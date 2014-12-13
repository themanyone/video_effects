#!/bin/bash

cp -l * $HOME/rpmbuild/BUILD/gst-plugins-bad-0.10.*/gst/hkfilters

patch -p0 $HOME/rpmbuild/BUILD/gst-plugins-bad-0.10.*/configure.ac < configure.ac.patch