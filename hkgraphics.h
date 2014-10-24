/* HKGraphics
 * Copyright (C) 2014 Henry Kroll, www.thenerdshow.com
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

#ifndef _HKGRAPHICS_H_
#define _HKGRAPHICS_H_
// if not using gst.h #include glib-2.0/glib.h
#include <gst/gst.h>

typedef struct _hkVidLayout
{
  // 3 data areas (YUV or RGB)
  guint8 *data[3];
  guint stride[3], wscale[3], hscale[3];
  guint width, height, size;
  // tracking colors for getLength, getDiff, getBounds
  guint8 *bgcolor;
  guint8 *fgcolor0;
  guint8 *fgcolor1;
  guint threshold;
  // todo: use this struct to reduce number of func args
} hkVidLayout;

guint8* rgb2yuv (guint rgb, guint8 *yuv);
guint8 *getPixel(hkVidLayout *vl, int x, int y, guint8 layer);
void plotXY (hkVidLayout *vl, int x, int y, guint8 *color);
void crosshairs(hkVidLayout *vl, guint *point, guint8 *color);
void cloak(hkVidLayout *vl, guint *rect);
void decimate(hkVidLayout *vl, guint *rect, guint8 sz);
void colorize(hkVidLayout *vl, guint *rect, guint8* color);
void blur(hkVidLayout *vl, guint *rect, guint8 sz);
void edge(hkVidLayout *vl, guint *rect, guint8 *color);
void outline(hkVidLayout *vl, guint *rect, guint8 *color);
void box(hkVidLayout *vl, guint *rect, guint8 *color);
guint8* colorAt (hkVidLayout *vl, int x, int y, guint8 *color);
gboolean matchColor (hkVidLayout *vl, int x, int y, guint8 *color);
gboolean matchAny (hkVidLayout *vl, int x, int y);
guint* getLength(hkVidLayout *vl, int x, int y, int dx, int dy);
guint* getBounds(hkVidLayout *vl, int x, int y, guint *rect);
guint *rectCenter(guint *rect);

#endif