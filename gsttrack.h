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

#ifndef _GST_TRACK_H_
#define _GST_TRACK_H_

#include <gst/videofilters/gstvideofilter2.h>
#include <gst/video/video.h>

enum
{
  PROP_0,
  PROP_MESSAGE,
  PROP_MARK,
  PROP_SIZE,
  PROP_BGCOLOR,
  PROP_FGCOLOR0,
  PROP_FGCOLOR1,
  PROP_THRESHOLD,
  PROP_MAX_OBJECTS
};

#define DEFAULT_MESSAGE TRUE
#define DEFAULT_MARK TRUE
#define DEFAULT_THRESHOLD 75
#define DEFAULT_SIZE 20
#define DEFAULT_MAX_OBJECTS 5
#define DEFAULT_COLOR 0xFF0000
#define MAX_OBJECTS 25

G_BEGIN_DECLS

#define GST_TYPE_TRACK   (gst_track_get_type())
#define GST_TRACK(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_TRACK,GstTrack))
#define GST_TRACK_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_TRACK,GstTrackClass))
#define GST_IS_TRACK(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_TRACK))
#define GST_IS_TRACK_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_TRACK))

typedef struct _GstTrack
{
  GstVideoFilter2 video_filter2;

  /* properties */
  gboolean message;             /* whether to post messages */
  gboolean mark;                /* whether to mark video */
  guint size;                   /* minimum detection size */
  guint bgcolor;                /* object color to track */
  guint fgcolor0;               /* object highlight or text */
  guint fgcolor1;               /* object spot or outline */
  guint threshold;              /* color tracking threshold */
  guint max_objects;            /* number of objects to track */

  /* state */
  guint *rect;                  /* bounding box of traced object */
  guint8 bgyuv[3];
  guint8 fgyuv0[3];
  guint8 fgyuv1[3];
  guint found[MAX_OBJECTS][4];

} GstTrack;

typedef struct _GstTrackClass
{
  GstVideoFilter2Class video_filter2_class;
} GstTrackClass;

GType gst_track_get_type (void);

G_END_DECLS

#endif
