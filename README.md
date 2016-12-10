# Video Effects Plugin for Gstreamer

Free and open source live video motion tracking, message capture, colorizing, stabilization, and more.

* realtime HD video stream editing, object tracking
* marker, markerless motion capture (mocap)
* flock, team tracking, multiple teams
* position reporting through application messages
* counting, measuring, 3D positioning, scene

## Uses

* video production
* live rotoscoping
* scientific research
* wildlife studies
* search and rescue
* gesture recognition
* mobile gaming
* augmented reality
* surveillance
* robotics

## FastTrack Team Tracker

The track element tracks and optionally marks areas of color in a video stream. A threshold value may be adjusted to eliminate false signals. The algorithm gives more weight to color, especialy skin tones, and pays less attention to shading and bad lighting. During each frame, if the #GstTrack:message property is #TRUE, track emits an element message for each video frame.

## FaceBlur

FaceBlur is included in the git version available below. Redact, obscure or remove faces, license plates, and other sensitive information in large batches of video, and even live video feeds!

But wait, there's more! During each frame, if the message property is TRUE, track emits an element message with the count, position, and dimensions of each detected object. GStreamer C projects, Python applications, such as Master Control, or Blender scripts, may intercept these messages and do things with them.

## Get FastTrack from github

```
git clone https://github.com/themanyone/video_effects
```

## Installation

Linux build instructions follow. On Fedora, for example, install these dependencies.

```
yumdownloader --source gstreamer-plugins-bad-free
su -c 'yum-builddep gstreamer-plugins-bad-free'
```

While the build dependencies are downloading, install the source RPM and unpack the sources.

```
rpm -ivh gstreamer-plugins-bad-free*
cd rpmbuild/SPECS/
rpmbuild -bp gstreamer-plugins-bad-free.spec
```

Rename the folder created by git, the video_effects folder, to hkeffects.

```
mv video_effects hkeffects
```

Copy the newly made hkeffects folder to rpmbuild/BUILD/gst-plugins-bad*

```
cp -r hkeffects rpmbuild/BUILD/gst-plugins-bad*/gst/
Patch configure.ac and run autogen.sh to generate Makefiles
```

```
cp hkeffects/configure.ac.patch rpmbuild/BUILD/gst-plugins-bad*/
cd rpmbuild/BUILD/gst-plugins-bad*/
patch -p0 < configure.ac.patch
./autogen.sh
./configure
cd gst/hkeffects
make
```

Those who ran configure with the correct options for their system may safely run su -c 'make install'. Otherwise, manually copy the plugins, .libs/libgsthkeffects.so to wherever the gstreamer-plugins go, usually /usr/lib64/gstreamer-0.10/ on a 64-bit Fedora system or $GST_PLUGIN_SYSTEM_PATH. Can optionally `export GST_PLUGIN_SYSTEM_PATH=` to the location of the plugins you want to use for that particular session. Whatever works.

See updated docs at http://thenerdshow.com/motion%20tracking.html
