#!/usr/bin/env python
# -*- Mode: Python -*-
# vi:si:et:sw=4:sts=4:ts=4

# gst-python
# Copyright (C) 2014 Henry Kroll <nospam@thenerdshow.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#

import sys

import pygst
pygst.require('0.10')

import gst

class test():

   def track_cb(self, sender, *args):
        msg = args[0]
        if msg.type == gst.MESSAGE_ELEMENT\
            and msg.structure.get_name()=="track":
            if msg.structure["object"] == 0:
                mix = self.bin.get_by_name("mix")
                pads = list(mix.pads())
                pad = pads[0]
                xc = msg.structure["xc"]
                yc = msg.structure["yc"]
                x = (320 - xc)
                y = (180 - yc)
                pad.set_property("xpos",x)
                pad.set_property("ypos",y)
        return gst.BUS_PASS

   def __init__(self, args):

    self.bin = gst.parse_launch("filesrc location=paper.avi\
 ! queue\
 ! decodebin\
 ! tee name=tee\
 ! videoscale\
 ! video/x-raw-yuv,width=240,height=200\
 ! cairotextoverlay text=\"unstable camera?\"\
 ! videomixer2 name=mix2\
 sink_0::ypos=200\
 ! videocrop top=50\
 ! autovideoconvert\
 ! autovideosink\
 tee.\
 ! queue\
 ! track mark=0 objects=1 bgcolor=0x7CA0D5 size=5 name=trk\
 ! videomixer2 name=mix\
 ! videocrop left=200 right=200 top=120 bottom=100\
 ! cairotextoverlay text=\"zoom to objects\"\
 ! mix2.")
    bus = self.bin.get_bus()
    bus.add_signal_watch()
    bus.connect("message::element", self.track_cb)
    res = self.bin.set_state(gst.STATE_PLAYING);
    assert res

    while 1:
      msg = bus.poll(gst.MESSAGE_EOS | gst.MESSAGE_ERROR, gst.SECOND)
      if msg:
          break

    res = self.bin.set_state(gst.STATE_NULL)
    assert res

if __name__ == '__main__':
    test(sys.argv)
