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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _GST_MOTRACK_H_
#define _GST_MOTRACK_H_

#include <gst/videofilters/gstvideofilter2.h>
#include <gst/video/video.h>

typedef enum {
  GST_MOTRACK_MARK_METHOD_NOTHING,
  GST_MOTRACK_MARK_METHOD_CROSSHAIRS,
  GST_MOTRACK_MARK_METHOD_BOX,
  GST_MOTRACK_MARK_METHOD_BOTH,
  GST_MOTRACK_MARK_METHOD_CLOAK,
  GST_MOTRACK_MARK_METHOD_BLUR,
  GST_MOTRACK_MARK_METHOD_BLUR8,
  GST_MOTRACK_MARK_METHOD_DECIMATE,
  GST_MOTRACK_MARK_METHOD_EDGE,
  GST_MOTRACK_MARK_METHOD_OUTLINE,
  GST_MOTRACK_MARK_METHOD_COLORIZE,
} GstMotrackMarkMethod;

#define RED   0xff0000
#define GREEN 0x00ff00
#define BLUE  0x0000ff
#define WHITE 0x0ffffff
#define MAX_OBJECTS 1024

G_BEGIN_DECLS

#define GST_TYPE_MOTRACK   (gst_motrack_get_type())
#define GST_MOTRACK(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MOTRACK,GstMotrack))
#define GST_MOTRACK_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MOTRACK,GstMotrackClass))
#define GST_IS_MOTRACK(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MOTRACK))
#define GST_IS_MOTRACK_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MOTRACK))

typedef struct _GstMotrack
{
  GstVideoFilter2 video_filter2;

  /* properties */
  gboolean message;             /* whether to post messages */
  guint speed;                   /* minimum detection size */
  guint minsize;                   /* minimum detection size */
  guint maxsize;                   /* minimum detection size */
  guint color0;                /* object color to motrack */
  guint color1;               /* object highlight or text */
  guint color2;               /* object spot or outline */
  guint mcolor;                 /* marker color */
  guint threshold;              /* color motracking threshold */
  guint max_objects;            /* number of objects to motrack */
  guint mark_method;            /* mark method */

  /* state */
  guint *rect;                  /* bounding box of motracked object */
  guint8 yuv0[3];              /* background color YUV */
  guint8 yuv1[3];
  guint8 yuv2[3];
  guint8 mcyuv[3];
  guint obj_found[MAX_OBJECTS][6]; /* array of found objects */
  guint obj_count;
} GstMotrack;

typedef struct _GstMotrackClass
{
  GstVideoFilter2Class video_filter2_class;
} GstMotrackClass;

GType gst_motrack_get_type (void);

G_END_DECLS

#endif
