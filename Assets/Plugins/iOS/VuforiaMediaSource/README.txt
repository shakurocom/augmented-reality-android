#==============================================================================
#            Copyright (c) 2012-2015 Qualcomm Connected Experiences, Inc.
#            All Rights Reserved.
#==============================================================================

Pre-requisites
==============
You need to install:
- xcode
- iOS SDK

Configuration
=============
You need first to modify the file called build.sh
and set the paths and build parameters accordingly in this file.

Build
=====
To rebuild the VuforiaMedia library:
- cd to VuforiaMediaSource/src
- rename VideoPlayerHelper.m.txt to VideoPlayerHelper.m (i.e. remove the .txt extension)
- rename VideoPlayerWrapper.mm.txt to VideoPlayerWrapper.mm (i.e. remove the .txt extension)
- cd back to VuforiaMedia and run from the current working directory:
  build.sh

This will compile and install the binary in the correct directories.
After compilation the following file will be updated:

Plugins/iOS/libVuforiaMedia.a
