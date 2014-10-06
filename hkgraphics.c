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

guint getDiff (hkVidLayout *vl, int x, int y, guint8 *color)
/* get diff between color at x,y and supplied color */
{
  guint8 *pixel;
  vl->diff = 0;
  for (int k=3;k--;){
    pixel = getPixel(vl, x, y, k);
    // k+1 favors color over shade
    vl->diff += (k+1) * abs(*pixel - color[k]);
  }
  return vl->diff;
}

guint getDiff2 (hkVidLayout *vl, int x, int y)
/* get diff between color at x,y and any of vl's fg and bg colors */
{
  getDiff(vl, x, y, vl->bgcolor);
  if (vl->diff > vl->threshold){
    getDiff(vl, x, y, vl->fgcolor0);
  }
  if (vl->diff > vl->threshold){
    getDiff(vl, x, y, vl->fgcolor1);
  }
  return vl->diff;
}

guint* getLength(hkVidLayout *vl, int x, int y, int dx, int dy)
/* stretch the measuring tape across a color patch */
{
  static guint endpoint[2];
  while(1){
    x += dx, y += dy;
    if (y >= 0 && y < vl->height && x >= 0 && x < vl->width){
      getDiff2(vl, x, y);
      if (vl->diff > vl->threshold){
        endpoint[0] = x-dx, endpoint[1] = y-dy;
        break;
      }
    } else {
      endpoint[0] = CLAMP (x-dx, 0, vl->width - 1);
      endpoint[1] = CLAMP (y-dy, 0, vl->height - 1);
      break;
    }
  }
  return endpoint;
}

void markBounds(hkVidLayout *vl, guint *rect, guint size)
/* mark area of color patch, so it isn't detected again */
{
  guint8 *pixel;
  guint sx = rect[0] - rect[0] % size + size;
  guint sy = rect[1] - rect[1] % size + size;
  for (guint y=sy; y<rect[3]; y+=size){
    for (guint x=sx; x<rect[2]; x+=size){
      pixel = getPixel(vl, x, y, 2);
      *pixel ^= 0x80;
    }
  }
}

guint* getBounds(hkVidLayout *vl, int x, int y, guint *rect)
/* survey area of color patch */
{
  guint *extent;
  if (y >= 0 && y < vl->height && x >= 0 && x < vl->width){
    if (!rect[3]) {
      rect[0] = rect[2] = x;
      rect[1] = rect[3] = y;    
    }
    // stretch the tape out in different directions
    // right
    extent = getLength(vl, x, y, 4, 0);
    rect[2] = (extent[0] > rect[2]) ? extent[0] : rect[2];
    extent = getLength(vl, extent[0], y, 1, 0);
    rect[2] = (extent[0] > rect[2]) ? extent[0] : rect[2];
    // right, down
    extent = getLength(vl, x, y, 4, 4);
    rect[2] = (extent[0] > rect[2]) ? extent[0] : rect[2];
    rect[3] = (extent[1] > rect[3]) ? extent[1] : rect[3];
    extent = getLength(vl, rect[2], rect[3], 1, 1);
    rect[2] = (extent[0] > rect[2]) ? extent[0] : rect[2];
    rect[3] = (extent[1] > rect[3]) ? extent[1] : rect[3];
    // down
    extent = getLength(vl, x, y, 0, 4);
    rect[3] = (extent[1] > rect[3]) ? extent[1] : rect[3];
    extent = getLength(vl, x, rect[3], 0, 1);
    rect[3] = (extent[1] > rect[3]) ? extent[1] : rect[3];
    // left, down
    extent = getLength(vl, x, y, -4, 4);
    rect[0] = (extent[0] < rect[0]) ? extent[0] : rect[0];
    rect[3] = (extent[1] > rect[3]) ? extent[1] : rect[3];
    extent = getLength(vl, rect[0], rect[3], -1, 1);
    rect[0] = (extent[0] < rect[0]) ? extent[0] : rect[0];
    rect[3] = (extent[1] > rect[3]) ? extent[1] : rect[3];
    // left
    extent = getLength(vl, x, y, -4, 0);
    rect[0] = (extent[0] < rect[0]) ? extent[0] : rect[0];
    extent = getLength(vl, rect[0], y, -1, 0);
    rect[0] = (extent[0] < rect[0]) ? extent[0] : rect[0];
    // left, up
    extent = getLength(vl, x, y, -4, -4);
    rect[0] = (extent[0] < rect[0]) ? extent[0] : rect[0];
    rect[1] = (extent[1] < rect[1]) ? extent[1] : rect[1];
    extent = getLength(vl, rect[0], rect[1], -1, -1);
    rect[0] = (extent[0] < rect[0]) ? extent[0] : rect[0];
    rect[1] = (extent[1] < rect[1]) ? extent[1] : rect[1];
    // up
    extent = getLength(vl, x, y, 0, -4);
    rect[1] = (extent[1] < rect[1]) ? extent[1] : rect[1];
    extent = getLength(vl, x, rect[1], 0, -1);
    rect[1] = (extent[1] < rect[1]) ? extent[1] : rect[1];
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