/* GStreamer
 * Copyright (C) 2014 Henry Kroll <nospam@thenerdshow.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstmotrack
 *
 * The motrack element motracks and optionally marks areas of color in a 
 * video stream. A threshold value may be adjusted to eliminate false 
 * signals. The algorithm gives more weight to color, especialy skin 
 * tones, and pays less attention to shading and bad lighting.
 * During each frame, if the #GstMotrack:message property is #TRUE,
 * motrack emits an element message for each video frame, named
 *
 * <classname>&quot;motrack&quot;</classname>
 *
 * The message's structure contains these fields:
 * <itemizedlist>
 * <listitem>
 *   <para>
 *   #GstValue of #guint
 *   <classname>&quot;count&quot;</classname>:
 *   Total number of detected objects on screen.
 *   </para>
 * </listitem>
 * <listitem>
 *   <para>
 *   #GstValue of #guint
 *   <classname>&quot;object&quot;</classname>:
 *   the temporary motracking number of each detected object.
 *   Note: Motracking numbers are initially assigned in top-down and
 *    left-to-right order. Motrack will attempt to assign the same
 *    motracking numbers to the same objects on each video frame,
 *    except if an object loses focus or leaves the frame.
 *   </para>
 * </listitem>
 * <listitem>
 *   <para>
 *   #GstValueList of #guint
 *   <classname>&quot;x1,y1,x2,y2&quot;</classname>:
 *   the x,y coordinates of the top-left and bottom-right corner
 *   of the bounding box for each detected object.
 *   </para>
 * </listitem>
 * <listitem>
 *   <para>
 *   #GstValueList of #guint
 *   <classname>&quot;xc,yc&quot;</classname>:
 *   the x,y coordinates of the center of each detected object.
 *   </para>
 * </listitem>
 * </itemizedlist>
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v v4l2src ! motrack ! autovideoconvert ! autovideosink
 * ]|
 * Insert motrack between video src and sink elements and set color 
 * properties. The motrackable objects' primary, or background color 
 * is most important. This is the first color that motrack finds. 
 * The foreground, text, and outline colors are optional. Motrack's 
 * algorithm can now motrack team players. Motrack can follow
 * blue players with black and orange markings, while ignoring orange 
 * players with black and blue markings on the field. Set the 
 * object's (player's) background color to blue and the foreground 
 * colors to black, and orange. Want to motrack the other team, too? 
 * No problem! Multiple instances of motrack may be added to the 
 * pipeline.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>
#include <math.h>
#include "gstmotrack.h"
#include "hkgraphics.h"

enum
{
  PROP_0,
  PROP_MESSAGE,
  PROP_MARK_METHOD,
  PROP_SPEED,
  PROP_MIN_SIZE,
  PROP_MAX_SIZE,
  PROP_COLOR0,
  PROP_COLOR1,
  PROP_COLOR2,
  PROP_MCOLOR,
  PROP_THRESHOLD,
  PROP_MAX_OBJECTS,
};

#define DEFAULT_MESSAGE TRUE
#define DEFAULT_THRESHOLD 88
#define DEFAULT_SPEED 20
#define DEFAULT_MIN_SIZE 20
#define DEFAULT_MAX_SIZE 100
#define DEFAULT_MAX_OBJECTS 1
#define DEFAULT_COLOR 0xFF0000

GST_DEBUG_CATEGORY_STATIC (gst_motrack_debug_category);
#define GST_CAT_DEFAULT gst_motrack_debug_category

/* prototypes */

static void gst_motrack_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_motrack_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_motrack_finalize (GObject * object);

static gboolean gst_motrack_start (GstBaseTransform * trans);
static gboolean gst_motrack_stop (GstBaseTransform * trans);

static GstFlowReturn
gst_motrack_prefilter (GstVideoFilter2 * videofilter2, GstBuffer * buf);

static GstVideoFilter2Functions gst_motrack_filter_functions[];

/* class initialization */

#define DEBUG_INIT(bla) \
  GST_DEBUG_CATEGORY_INIT (gst_motrack_debug_category, "motrack", 0, \
      "debug category for motrack element");

#define GST_TYPE_MOTRACK_MARK_METHOD (gst_motrack_mark_method_get_type())
#define DEFAULT_MARK_METHOD GST_MOTRACK_MARK_METHOD_BOTH

static const GEnumValue repace_methods[] = {
  {GST_MOTRACK_MARK_METHOD_NOTHING, "Do nothing", "nothing"},
  {GST_MOTRACK_MARK_METHOD_CROSSHAIRS, "Mark with crosshairs", "crosshairs"},
  {GST_MOTRACK_MARK_METHOD_BOX, "Draw box", "box"},
  {GST_MOTRACK_MARK_METHOD_BOTH, "Both 1 and 2", "both"},
  {GST_MOTRACK_MARK_METHOD_CLOAK, "Cloaking device", "cloak"},
  {GST_MOTRACK_MARK_METHOD_BLUR8, "Blur, 8 x 8 average", "blur"},
  {GST_MOTRACK_MARK_METHOD_BLUR, "Blur, size x size", "sizeblur"},
  {GST_MOTRACK_MARK_METHOD_DECIMATE, "Obscure (decimate, blocks)",
      "decimate"},
  {GST_MOTRACK_MARK_METHOD_EDGE, "Edge detect", "edge"},
  {GST_MOTRACK_MARK_METHOD_OUTLINE, "Color outlines", "outline"},
  {GST_MOTRACK_MARK_METHOD_COLORIZE, "Colorize to marker color",
      "colorize"},
  {0, NULL, NULL},
};

static GType
gst_motrack_mark_method_get_type (void)
{
  static GType repace_method_type = 0;
  if (!repace_method_type) {
    repace_method_type = g_enum_register_static ("GstMotrackMarkMethod",
        repace_methods);
  }
  return repace_method_type;
}

GST_BOILERPLATE_FULL (GstMotrack, gst_motrack, GstVideoFilter2,
    GST_TYPE_VIDEO_FILTER2, DEBUG_INIT);

static void
gst_motrack_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);
  gst_element_class_set_details_simple (element_class, "Object tracking / marking",
      "Filter/Tracking",
      "The motrack element tracks and optionally marks areas of color in a video stream.",
    "Henry Kroll III, www.thenerdshow.com");
}

static void
gst_motrack_class_init (GstMotrackClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstVideoFilter2Class *video_filter2_class = GST_VIDEO_FILTER2_CLASS (klass);
  GstBaseTransformClass *base_transform_class =
      GST_BASE_TRANSFORM_CLASS (klass);

  gobject_class->set_property = gst_motrack_set_property;
  gobject_class->get_property = gst_motrack_get_property;
  gobject_class->finalize = gst_motrack_finalize;
  base_transform_class->start = GST_DEBUG_FUNCPTR (gst_motrack_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_motrack_stop);

  video_filter2_class->prefilter =
      GST_DEBUG_FUNCPTR (gst_motrack_prefilter);

  g_object_class_install_property (gobject_class, PROP_MESSAGE,
      g_param_spec_boolean ("message", "message",
          "Post a message for each motracked object",
        DEFAULT_MESSAGE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MARK_METHOD,
      g_param_spec_enum ("mark", "Mark Method", "Method for marking motracked objects",
          GST_TYPE_MOTRACK_MARK_METHOD, DEFAULT_MARK_METHOD,
          GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_THRESHOLD,
      g_param_spec_uint ("threshold", "Threshold",
          "Motracking color difference threshold", 0, 600,
          DEFAULT_THRESHOLD,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MAX_OBJECTS,
      g_param_spec_uint ("objects", "Objects",
          "Max number of objects to motrack", 1, MAX_OBJECTS,
          DEFAULT_MAX_OBJECTS,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_SPEED,
      g_param_spec_uint ("speed", "Speed",
          "Speed (lossy checking, skips n pixels)", 1, 100,
          DEFAULT_SPEED,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MIN_SIZE,
      g_param_spec_uint ("min-size", "Minimum size",
          "Minimum size of objects", 1, 100,
          DEFAULT_SPEED,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MAX_SIZE,
      g_param_spec_uint ("max-size", "Maximum size",
          "Maximum size of objects)", 1, 500,
          DEFAULT_MAX_SIZE,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_COLOR0,
      g_param_spec_uint ("color0", "Background Color",
          "Object's Main or Background Color RGB red=0xff0000", 0, 2000,
          DEFAULT_MIN_SIZE,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_COLOR1,
      g_param_spec_uint ("color1", "Foreground Color 0",
          "Object's Highlight or Text Color RGB green=0x00ff00", 0, G_MAXUINT,
          DEFAULT_COLOR,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_COLOR2,
      g_param_spec_uint ("color2", "Foreground Color 1",
          "Object's Spot or Outline Color RGB blue=0x0000ff", 0, G_MAXUINT,
          DEFAULT_COLOR,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MCOLOR,
      g_param_spec_uint ("mcolor", "Marker Color",
          "Marker color RGB white=0xffffff", 0, G_MAXUINT,
          GREEN,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  gst_video_filter2_class_add_functions (video_filter2_class,
      gst_motrack_filter_functions);
}

static void
gst_motrack_init (GstMotrack * motrack,
    GstMotrackClass * motrack_class)
{
  motrack->message = DEFAULT_MESSAGE;
  motrack->color0 = DEFAULT_COLOR;
  motrack->color1 = DEFAULT_COLOR;
  motrack->color2 = DEFAULT_COLOR;
  motrack->mcolor = GREEN;
  motrack->threshold = DEFAULT_THRESHOLD;
  motrack->max_objects = DEFAULT_MAX_OBJECTS;
  motrack->mark_method = DEFAULT_MARK_METHOD;
  motrack->obj_count = 0;
  for (int obj=MAX_OBJECTS; obj--;)
    motrack->obj_found[obj][3] = 0;
}

void
gst_motrack_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstMotrack *motrack;
  g_return_if_fail (GST_IS_MOTRACK (object));
  motrack = GST_MOTRACK (object);

  switch (property_id) {
    case PROP_MESSAGE:
      motrack->message = g_value_get_boolean(value);
      break;
    case PROP_MARK_METHOD:
      motrack->mark_method = g_value_get_enum(value);
      break;
    case PROP_SPEED:
      motrack->speed = g_value_get_uint(value);
      break;
    case PROP_MIN_SIZE:
      motrack->minsize = g_value_get_uint(value);
      break;
    case PROP_MAX_SIZE:
      motrack->maxsize = g_value_get_uint(value);
      break;
    case PROP_COLOR0:
      motrack->color0 = g_value_get_uint(value);
      rgb2yuv(motrack->color0, motrack->yuv0);
      break;
    case PROP_COLOR1:
      motrack->color1 = g_value_get_uint(value);
      rgb2yuv(motrack->color1, motrack->yuv1);
      break;
    case PROP_COLOR2:
      motrack->color2 = g_value_get_uint(value);
      rgb2yuv(motrack->color2, motrack->yuv2);
      break;
    case PROP_MCOLOR:
      motrack->mcolor = g_value_get_uint(value);
      rgb2yuv(motrack->mcolor, motrack->mcyuv);
      break;
    case PROP_THRESHOLD:
      motrack->threshold = g_value_get_uint(value);
      break;
    case PROP_MAX_OBJECTS:
      motrack->max_objects = g_value_get_uint(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_motrack_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstMotrack *motrack;

  g_return_if_fail (GST_IS_MOTRACK (object));
  motrack = GST_MOTRACK (object);

  switch (property_id) {
    case PROP_MESSAGE:
      g_value_set_boolean (value, motrack->message);
      break;
    case PROP_MARK_METHOD:
      g_value_set_enum (value, motrack->mark_method);
      break;
    case PROP_SPEED:
      g_value_set_uint (value, motrack->speed);
      break;
    case PROP_MIN_SIZE:
      g_value_set_uint (value, motrack->minsize);
      break;
    case PROP_MAX_SIZE:
      g_value_set_uint (value, motrack->maxsize);
      break;
    case PROP_COLOR0:
      g_value_set_uint (value, motrack->color0);
      break;
    case PROP_COLOR1:
      g_value_set_uint (value, motrack->color1);
      break;
    case PROP_COLOR2:
      g_value_set_uint (value, motrack->color2);
      break;
    case PROP_MCOLOR:
      g_value_set_uint (value, motrack->mcolor);
      break;
    case PROP_THRESHOLD:
      g_value_set_uint (value, motrack->threshold);
      break;
    case PROP_MAX_OBJECTS:
      g_value_set_uint (value, motrack->max_objects);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_motrack_finalize (GObject * object)
{
  g_return_if_fail (GST_IS_MOTRACK (object));

  /* clean up object here */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static gboolean
gst_motrack_start (GstBaseTransform * trans)
{
  return TRUE;
}

static gboolean
gst_motrack_stop (GstBaseTransform * trans)
{
  return TRUE;
}

static GstFlowReturn
gst_motrack_prefilter (GstVideoFilter2 * videofilter2, GstBuffer * buf)
{
  return GST_FLOW_OK;
}

static void hkgraphics_init (GstMotrack *motrack, hkVidLayout *vl, GstBuffer *buf)
/* populate hkVidLayout struct for hkgraphics library */
/* called upon each video frame */
{
  GstVideoFormat format = GST_VIDEO_FILTER2_FORMAT (motrack);
  guint8 *gdata = GST_BUFFER_DATA (buf);
  vl->width = GST_VIDEO_FILTER2_WIDTH (motrack),
  vl->height = GST_VIDEO_FILTER2_HEIGHT (motrack),
  vl->threshold = motrack->threshold,
  vl->color0 = motrack->yuv0,
  vl->color1 = motrack->yuv1;
  vl->color2 = motrack->yuv2;
  for (int i=3;i--;){
    vl->data[i] = gdata + gst_video_format_get_component_offset
      (format, i, vl->width, vl->height);
    vl->stride[i] = gst_video_format_get_row_stride
      (format, i, vl->width);
    vl->hscale[i] = vl->height /
      gst_video_format_get_component_height (format, i, vl->height);
    vl->wscale[i] = vl->width /
      gst_video_format_get_component_width (format, i, vl->width);
  }
}

static gboolean is_reject(GstMotrack *motrack, guint *rect, guint obj)
{
  gint sz = motrack->speed;
  gboolean reject = FALSE;
  // too small?
  if (rect[2]-rect[0] < motrack->speed
    || rect[3]-rect[1] < motrack->speed){
    reject = TRUE;
  }
  // already detected?
  for (int o=MAX_OBJECTS; o--;){
    if (o==obj || !motrack->obj_found[o][3]) continue;
    if (motrack->obj_found[o][0]>=rect[0]-sz
      && motrack->obj_found[o][1]>=rect[1]-sz
      && motrack->obj_found[o][2]<=rect[2]+sz
      && motrack->obj_found[o][3]<=rect[3]+sz
    ){ 
        reject = TRUE;
        break;
    }
  }
  return reject;
}

static void scan_for_objects(GstMotrack *motrack, hkVidLayout *vl)
/* search video frame and count any colored objects */
{
  guint size = motrack->speed,
    max = motrack->max_objects,
    rect[4]={0}, *center,
    available = 0;
  for (int i=0; i<vl->height && motrack->obj_count < max; i+=size){
    for (int j=0;j<vl->width && motrack->obj_count < max; j+=size){
      if (matchColor(vl, j, i, motrack->yuv0)){
        // measure bounds of detected object
        getBounds(vl, j, i, rect);
        if (is_reject(motrack, rect, MAX_OBJECTS)){
          rect[3] = 0; continue;
        }
        center = rectCenter(rect);
        // find an available obj_found storage location
        for (int i=0; i<max; i++){
          if (!motrack->obj_found[i][3]){
            available = i;
            break;
          }
        }
        for (int r=4; r--;){
          motrack->obj_found[available][r] = rect[r];
          rect[r] = 0;
        }
        motrack->obj_found[available][4] = center[0];
        motrack->obj_found[available][5] = center[1];
        motrack->obj_count++;
      }
    }
  }
}

static void motrack_objects(GstMotrack *motrack, hkVidLayout *vl)
/* Follows existing objects as they move about. */
/* Attempts to keep persistent motracking numbers assigned. */
{
  guint *rect, *center;
  for (int obj = 0; obj < MAX_OBJECTS; obj++){
    rect = motrack->obj_found[obj];
    if (!rect[3]) continue; // next
    // clear old rect
    for (int i=4; i--;) rect[i] = 0;
    // get new bounds
    getBounds(vl, rect[4], rect[5], rect);
    // check validity
    if (is_reject(motrack, rect, obj)){
      // reject; wipe it
      motrack->obj_count--;
      rect[3] = 0; continue; // next
    }
    // get new center
    center = rectCenter(rect);
    rect[4] = center[0];
    rect[5] = center[1];
  }
  scan_for_objects(motrack, vl);
}

static void report_objects(GstMotrack *motrack, hkVidLayout *vl)
/* report object count, locations, optionally mark */
{
  GstStructure *s;
  guint8 *mcolor = motrack->mcyuv;
  guint *prect, *center, obj = 0;
  for (int c=motrack->obj_count; c--;){
    do {
      if (motrack->obj_found[obj][3]) break;
      obj++;
    } while (1);
    prect = motrack->obj_found[obj], center = &motrack->obj_found[obj][4];
    switch (motrack->mark_method){
      case GST_MOTRACK_MARK_METHOD_BOX:
        box(vl, prect, mcolor);
        break;
      case GST_MOTRACK_MARK_METHOD_BOTH:
        box(vl, prect, mcolor);
      case GST_MOTRACK_MARK_METHOD_CROSSHAIRS:
        crosshairs(vl, center, mcolor);
        break;
      case GST_MOTRACK_MARK_METHOD_CLOAK:
        cloak(vl, prect);
        break;
      case GST_MOTRACK_MARK_METHOD_BLUR:
        blur(vl, prect, motrack->speed);
        break;
      case GST_MOTRACK_MARK_METHOD_BLUR8:
        blur(vl, prect, 8);
        break;
      case GST_MOTRACK_MARK_METHOD_EDGE:
        edge(vl, prect, mcolor);
        break;
      case GST_MOTRACK_MARK_METHOD_OUTLINE:
        outline(vl, prect, mcolor);
        break;
      case GST_MOTRACK_MARK_METHOD_DECIMATE:
        decimate(vl, prect, motrack->speed);
        break;
      case GST_MOTRACK_MARK_METHOD_COLORIZE:
        colorize(vl, prect, mcolor);
        break;
      default:
        break;
    }
    if (motrack->message){
      s = gst_structure_new ("motrack",
      "count", G_TYPE_UINT, motrack->obj_count,
      "object", G_TYPE_UINT, obj,
      "x1", G_TYPE_UINT, prect[0],
      "y1", G_TYPE_UINT, prect[1],
      "x2", G_TYPE_UINT, prect[2],
      "y2", G_TYPE_UINT, prect[3],
      "xc", G_TYPE_UINT, center[0],
      "yc", G_TYPE_UINT, center[1],
        NULL);
      gst_element_post_message (GST_ELEMENT_CAST (motrack),
        gst_message_new_element (GST_OBJECT_CAST (motrack), s));
    }
    obj++;
  }
}

static GstFlowReturn
gst_motrack_filter_ip_planarY (GstVideoFilter2 * videofilter2,
    GstBuffer * buf, int start, int end)
{
  GstMotrack *motrack = GST_MOTRACK (videofilter2);
  hkVidLayout vl; hkgraphics_init(motrack, &vl, buf);
  motrack_objects(motrack, &vl);
  report_objects(motrack, &vl);
  return GST_FLOW_OK;
}

static GstFlowReturn
gst_motrack_filter_ip_YxYy (GstVideoFilter2 * videofilter2,
    GstBuffer * buf, int start, int end)
{
  g_print("Fixme: YxYy\n");
  return GST_FLOW_OK;
}

static GstFlowReturn
gst_motrack_filter_ip_AYUV (GstVideoFilter2 * videofilter2,
    GstBuffer * buf, int start, int end)
{
  g_print("Fixme: AYUV\n");
  return GST_FLOW_OK;
}

static GstVideoFilter2Functions gst_motrack_filter_functions[] = {
  {GST_VIDEO_FORMAT_I420, NULL, gst_motrack_filter_ip_planarY},
  {GST_VIDEO_FORMAT_YV12, NULL, gst_motrack_filter_ip_planarY},
  {GST_VIDEO_FORMAT_Y41B, NULL, gst_motrack_filter_ip_planarY},
  {GST_VIDEO_FORMAT_Y42B, NULL, gst_motrack_filter_ip_planarY},
  {GST_VIDEO_FORMAT_NV12, NULL, gst_motrack_filter_ip_planarY},
  {GST_VIDEO_FORMAT_NV21, NULL, gst_motrack_filter_ip_planarY},
  {GST_VIDEO_FORMAT_YUV9, NULL, gst_motrack_filter_ip_planarY},
  {GST_VIDEO_FORMAT_YVU9, NULL, gst_motrack_filter_ip_planarY},
  {GST_VIDEO_FORMAT_Y444, NULL, gst_motrack_filter_ip_planarY},
  {GST_VIDEO_FORMAT_UYVY, NULL, gst_motrack_filter_ip_YxYy},
  {GST_VIDEO_FORMAT_YUY2, NULL, gst_motrack_filter_ip_YxYy},
  {GST_VIDEO_FORMAT_YVYU, NULL, gst_motrack_filter_ip_YxYy},
  {GST_VIDEO_FORMAT_AYUV, NULL, gst_motrack_filter_ip_AYUV},
  {GST_VIDEO_FORMAT_UNKNOWN}
};
