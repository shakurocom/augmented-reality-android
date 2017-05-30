#==============================================================================
#            Copyright (c) 2012-2014 Qualcomm Connected Experiences, Inc.
#            All Rights Reserved.
#==============================================================================

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := libVuforiaMedia
LOCAL_ARM_MODE  := arm
LOCAL_SRC_FILES := VideoPlayerHelper.cpp SampleUtils.cpp
LOCAL_LDLIBS    := -llog -lGLESv3

include $(BUILD_SHARED_LIBRARY)
