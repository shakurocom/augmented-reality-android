#==============================================================================
#            Copyright (c) 2012-2014 Qualcomm Connected Experiences, Inc.
#            All Rights Reserved.
#==============================================================================

# This script builds the libVuforiaMedia library from source and copies 
# it into the Plugin folder

# Ensure any error stops the script and fails the build
set -e

export DEVELOPER_DIR=/Applications/Xcode_61.app/Contents/Developer
export CONST_IOS_SDK=iphoneos8.1
export CONST_CLEAN_BUILD=clean

# Build VuforiaMedia and copy to Plugin folder
xcodebuild -sdk $CONST_IOS_SDK $CONST_CLEAN_BUILD build
cp ./build/Release-iphoneos/libVuforiaMedia.a ../
