filesrc location=/home/henry/dev/c/pop_can_tracking/paper.avi
 ! queue
 ! decodebin
 ! track bgcolor=0x7CA1D6 mark=0 size=4 erase=1 threshold=120
 ! queue
 ! videoscale
 ! video/x-raw-yuv,width=200, height=150
 ! autovideoconvert
 ! autovideosink