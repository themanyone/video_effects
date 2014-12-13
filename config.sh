#!/bin/bash

cd $HOME/rpmbuild/BUILD/gst-plugins-bad-0.10.*/

./autogen.sh

./configure --with-package-name="Fedora gstreamer-plugins-bad package" --with-package-origin="http://download.fedora.redhat.com/fedora" --enable-debug --disable-static --enable-gtk-doc --enable-experimental --disable-divx --disable-dts --disable-faac --disable-faad --disable-nas --disable-mimic --disable-libmms --disable-mpeg2enc --disable-mplex --disable-neon --disable-openal --disable-rtmp --disable-xvid --prefix=/usr