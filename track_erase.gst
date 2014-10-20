filesrc location=/home/henry/dev/c/pop_can_tracking/paper.avi
 ! queue
 ! decodebin
 ! videoscale
 ! video/x-raw-yuv,width=200, height=150
 ! track bgcolor=0x7CA1D6 size=4 mark=4 threshold=120
 ! queue
 ! autovideoconvert
 ! autovideosink