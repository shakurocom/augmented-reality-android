/*===============================================================================
Copyright (c) 2015 PTC Inc. All Rights Reserved.
 
Copyright (c) 2010-2015 Qualcomm Connected Experiences, Inc. All Rights Reserved.
 
Vuforia is a trademark of PTC Inc., registered in the United States and other 
countries.
==============================================================================*/

#ifndef _VUFORIA_MEDIA_VIDEO_PLAYER_WRAPPER_H_
#define _VUFORIA_MEDIA_VIDEO_PLAYER_WRAPPER_H_

#ifdef __cplusplus
extern "C"
{
#endif

    void* videoPlayerInitIOS(bool isMetalRendering);
    bool videoPlayerDeinitIOS(void* dataSetPtr);
    bool videoPlayerLoadIOS(void* dataSetPtr, const char* filename, int requestType, bool playOnTextureImmediately, float seekPosition);
    bool videoPlayerUnloadIOS(void* dataSetPtr);
    bool videoPlayerIsPlayableOnTextureIOS(void* dataSetPtr);
    bool videoPlayerIsPlayableFullscreenIOS(void* dataSetPtr);
    bool videoPlayerSetVideoTexturePtrIOS(void* dataSetPtr, void* texturePtr);
    int videoPlayerGetStatusIOS(void* dataSetPtr);
    int videoPlayerGetVideoWidthIOS(void* dataSetPtr);
    int videoPlayerGetVideoHeightIOS(void* dataSetPtr);
    float videoPlayerGetLengthIOS(void* dataSetPtr);
    bool videoPlayerPlayIOS(void* dataSetPtr, bool fullScreen, float seekPosition);
    bool videoPlayerPauseIOS(void* dataSetPtr);
    bool videoPlayerStopIOS(void* dataSetPtr);
    int videoPlayerUpdateVideoDataIOS(void* dataSetPtr);
    bool videoPlayerSeekToIOS(void* dataSetPtr, float position);
    float videoPlayerGetCurrentPositionIOS(void* dataSetPtr);
    bool videoPlayerSetVolumeIOS(void* dataSetPtr, float value);
    int videoPlayerGetCurrentBufferingPercentageIOS(void* dataSetPtr);
    void videoPlayerOnPauseIOS(void* dataSetPtr);
    
#ifdef __cplusplus
}
#endif

#endif // _VUFORIA_MEDIA_VIDEO_PLAYER_WRAPPER_H_
