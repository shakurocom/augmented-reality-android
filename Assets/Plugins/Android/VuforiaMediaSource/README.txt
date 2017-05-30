#==============================================================================
#            Copyright (c) 2012-2014 Qualcomm Connected Experiences, Inc.
#            All Rights Reserved.
#==============================================================================

Pre-requisites
==============
You need to install:
- the Android SDK
- the Android NDK
- ant (from ant.apache.org)
- on Windows: cygwin with make installed


Configuration
=============
You need first to modify the file called local.properties
and set the paths accordingly in this file.
Currently the library is compiled for OpenGL 3. You can change this in Android.mk file and compile it for OpenGL 2.

Build
=====
To rebuild the VuforiaMedia libraries, just run from the current working directory (using cygwin on Windows):
ant

This will compile and install the binaries in the correct directories.
After compilation the following files will be updated:

Plugins/Android
+-- VuforiaMedia.jar
+-- libs
    +-- armeabi-v7a
        +-- libVuforiaMedia.so
