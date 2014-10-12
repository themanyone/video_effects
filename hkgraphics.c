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
//{
#include "hkgraphics.h"
#include "gsttrack.h"

guint8* rgb2yuv (guint rgb, guint8 *yuv)
/* convert from rgbint, store in supplied yuv array (pointer) */
{
  guint8 R = rgb>>16;
  guint8 G = rgb>>8;
  guint8 B = rgb;
  gint YUV[3];
  YUV[0] = 0.299 * R + 0.587 * G + 0.114 * B;
  YUV[1] = -.169 * R - 0.331 * G + 0.499 * B + 128;
  YUV[2] = 0.499 * R - 0.418 * G - 0.081 * B + 128;
  for(int k=3;k--;){
    yuv[k] = CLAMP(YUV[k],0,255);
  }
  return yuv;
}

guint8 *getPixel(hkVidLayout *vl, int x, int y, guint8 layer)
/* returns pixel data location at x, y, layer */
{
  guint8 *pixel;
  pixel = vl->data[layer] + y/vl->hscale[layer] * vl->stride[layer]
    + x/vl->wscale[layer];
  return pixel;
}

void plotXY (hkVidLayout *vl, int x, int y, guint8 *color)
/* mark a pixel at x,y with color */
/* caution: no bounds checking */
{
  guint8 *pixel;
  for (int k=3;k--;){
    pixel = getPixel(vl, x, y, k);
    *pixel = color[k];
  }
}

void crosshairs(hkVidLayout *vl, guint *point, guint8 *color)
/* draw crosshairs at point with color */
{
  guint x=point[0], y=point[1];
  // left crosshair
  for (int i=x-4; i>0 && i > x-14 ;i--){
    plotXY(vl, i, y, color);
  }
  // right crosshair
  for (int i=x+4; i<vl->width && i < x+14 ;i++){
    plotXY(vl, i, y, color);
  }
  // top crosshair
  for (int i=y-4; i>0 && i > y-14 ;i--){
    plotXY(vl, x, i, color);
  }
  // bottom crosshair
  for (int i=y+4; i<vl->height && i < y+14 ;i++){
    plotXY(vl, x, i, color);
  }
}

void erase(hkVidLayout *vl, guint *rect)
/* attempt to erase rect from video */
{
  guint8 *from;
  guint height = rect[3]-rect[1], h2 = height / 2;
  for(int y=rect[3]; y >= rect[1] + h2; y--){
    for(int x=rect[2]; x > rect[0]; x--){
      for (int k=3; k--;){
        if (y > height)
          from = getPixel(vl, x, y - height, k);
        else
          from = getPixel(vl, x, y + h2, k);
        *(getPixel(vl, x, y, k)) = *from;
        *(getPixel(vl, x, y - h2, k)) = *from;
      }
    }
  }
}

void box(hkVidLayout *vl, guint *rect, guint8 *color)
/* draw box at rect[4] with color */
{
  guint x1 = rect[0], y1 = rect[1], x2 = rect[2], y2 = rect[3];
  // box top
  for (int i=x2;i>x1;i--){
    plotXY(vl, i, y1, color);
  }
  // box bottom
  for (int i=x2;i>x1;i--){
    plotXY(vl, i, y2, color);
  }
  // box left
  for (int i=y2;i>y1;i--){
    plotXY(vl, x1, i, color);
  }
  // box right
  for (int i=y2;i>y1;i--){
    plotXY(vl, x2, i, color);
  }
}

guint8* colorAt (hkVidLayout *vl, int x, int y, guint8 *color)
/* copy color at x,y into supplied array (pointer) */
{
  guint8 *pixel;
  for (int k=3;k--;){
    pixel = getPixel(vl, x, y, k);
    color[k] = *pixel;
  }
  // color is now modified; we don't need to return it,
  // but sometimes a return value is handy
  return color;
}

gboolean matchColor (hkVidLayout *vl, int x, int y, guint8 *color)
/* check if supplied color matches color at x,y and vl->threshold */
{
  guint8 *pixel;
  guint diff = 0;
  for (int k=3;k--;){
    pixel = getPixel(vl, x, y, k);
    // k+1 favors color over shade
    diff += (k+1) * abs(*pixel - color[k]);
  }
  return diff < vl->threshold;
}

gboolean matchAny (hkVidLayout *vl, int x, int y)
/* check if any color matches that at x,y and vl->threshold */
{
  return matchColor(vl, x, y, vl->bgcolor) || 
        matchColor(vl, x, y, vl->fgcolor0) ||
        matchColor(vl, x, y, vl->fgcolor1);
}

guint* getLength(hkVidLayout *vl, int x, int y, int dx, int dy)
/* stretch the measuring tape across a color patch */
{
  static guint endpoint[2];
  while (1) {
    x += dx, y += dy;
    if ( x < 0 
      || x >= vl->width
      || y < 0
      || y >= vl->height
      || ! matchAny(vl, x, y)) {
      x-=dx, y-=dy;
      if (abs(dx) > 1 || abs(dy) > 1) {
        if (dx) dx /= abs(dx);
        if (dy) dy /= abs(dy);
      } else break;
    }
  }
  endpoint[0] = CLAMP (x, 0, vl->width - 1);
  endpoint[1] = CLAMP (y, 0, vl->height - 1);
  return endpoint;
}

guint* getBounds(hkVidLayout *vl, int x, int y, guint *rect)
/* survey area of color patch */
{
  #define STEP 8
  gboolean expanded;
  guint *extent;
  if (y >= 0 && y < vl->height && x >= 0 && x < vl->width){
    if (!rect[3]) {
      rect[0] = rect[2] = x;
      rect[1] = rect[3] = y;    
    }
    // stretch the box out in different directions
    do {
      expanded = FALSE;
      // right
      for (int v = rect[1]; v<=rect[3]; v+=1){
        extent = getLength(vl, rect[2], v, STEP, 0);
        if (extent[0] > rect[2]){
          rect[2] = extent[0];
          expanded = TRUE;
        }
      }
      // down
      for (int h = rect[0]; h<=rect[2]; h+=1){
        extent = getLength(vl, h, rect[3], 0, STEP);
        if (extent[1] > rect[3]){
          rect[3] = extent[1];
          expanded = TRUE;
        }
      }
      // left
      for (int v = rect[1]; v<=rect[3]; v+=1){
        extent = getLength(vl, rect[0], v, -STEP, 0);
        if (extent[0] < rect[0]){
          rect[0] = extent[0];
          expanded = TRUE;
        }
      }
      // up
      for (int h = rect[0]; h<=rect[2]; h+=1){
        extent = getLength(vl, h, rect[1], 0, -STEP);
        if (extent[1] < rect[1]){
          rect[1] = extent[1];
          expanded = TRUE;
        }
      }
    } while (expanded);
  }
  return rect;
}

guint *rectCenter(guint *rect)
/* return center of rect */
{
  static guint point[2];
  point[0] = (rect[0] + rect[2]) / 2;
  point[1] = (rect[1] + rect[3]) / 2;
  return point;
}
//}