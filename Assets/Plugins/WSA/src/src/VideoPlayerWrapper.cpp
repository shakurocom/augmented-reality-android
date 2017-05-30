/*===============================================================================
Copyright (c) 2016 PTC Inc. All Rights Reserved.

Vuforia is a trademark of PTC Inc., registered in the United States and other
countries.
===============================================================================*/
#include "VideoPlayerWrapper.h"
#include "IUnityGraphics.h"

#include <d3d11.h>
#include "IUnityGraphicsD3D11.h"

#include <string>
#include <unordered_set>

using namespace VuforiaMedia;

static IUnityInterfaces* s_UnityInterfaces = NULL;
static IUnityGraphics* s_Graphics = NULL;
static UnityGfxRenderer s_DeviceType = kUnityGfxRendererNull;
static ID3D11Device* s_D3D11Device = NULL;

static std::unordered_set<VideoPlayerHelper*> s_VideoPlayers;

extern "C" int64_t UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerInitWSA()
{
    VideoPlayerHelper* videoPlayerHelper = new VideoPlayerHelper(s_D3D11Device);
    s_VideoPlayers.insert(videoPlayerHelper);
    return (int64_t)videoPlayerHelper;
}

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerDeinitWSA(void* dataSetPtr)
{
    if (dataSetPtr == nullptr)
    {
        return false;
    }
    VideoPlayerHelper* vidPlayer = (VideoPlayerHelper*)dataSetPtr;
    s_VideoPlayers.erase(vidPlayer);
    delete vidPlayer;
    return true;
}

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerLoadWSA(
    void* dataSetPtr, const char* filename, int requestType, bool playOnTextureImmediately, float seekPosition)
{
    if (dataSetPtr == nullptr)
    {
        return false;
    }

    VideoPlayerHelper* vidPlayerHelper = (VideoPlayerHelper*)dataSetPtr;
    return vidPlayerHelper->Load(filename, requestType, playOnTextureImmediately, seekPosition);
}

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerUnloadWSA(void* dataSetPtr)
{
    if (dataSetPtr == nullptr)
    {
        return false;
    }

    VideoPlayerHelper* vidPlayerHelper = (VideoPlayerHelper*)dataSetPtr;
    return vidPlayerHelper->Unload();
}

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerIsPlayableOnTextureWSA(void* dataSetPtr)
{
    if (dataSetPtr == nullptr)
    {
        return false;
    }

    VideoPlayerHelper* vidPlayerHelper = (VideoPlayerHelper*)dataSetPtr;
    return vidPlayerHelper->IsPlayableOnTexture();
}

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerIsPlayableFullscreenWSA(void* dataSetPtr)
{
    if (dataSetPtr == nullptr)
    {
        return false;
    }

    VideoPlayerHelper* vidPlayerHelper = (VideoPlayerHelper*)dataSetPtr;
    return vidPlayerHelper->IsPlayableFullscreen();
}

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerSetVideoTexturePtrWSA(
    void* dataSetPtr, void* texturePtr)
{
    if (dataSetPtr == nullptr)
    {
        return false;
    }

    VideoPlayerHelper* vidPlayerHelper = (VideoPlayerHelper*)dataSetPtr;
    return vidPlayerHelper->SetVideoTexturePtr((ID3D11Texture2D*)texturePtr);
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerGetStatusWSA(void* dataSetPtr)
{
    if (dataSetPtr == nullptr)
    {
        return false;
    }

    VideoPlayerHelper* vidPlayerHelper = (VideoPlayerHelper*)dataSetPtr;
    return (int)vidPlayerHelper->GetStatus();
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerGetVideoWidthWSA(void* dataSetPtr)
{
    if (dataSetPtr == nullptr)
    {
        return false;
    }

    VideoPlayerHelper* vidPlayerHelper = (VideoPlayerHelper*)dataSetPtr;
    return vidPlayerHelper->GetVideoWidth();
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerGetVideoHeightWSA(void* dataSetPtr)
{
    if (dataSetPtr == nullptr)
    {
        return false;
    }

    VideoPlayerHelper* vidPlayerHelper = (VideoPlayerHelper*)dataSetPtr;
    return vidPlayerHelper->GetVideoHeight();
}

extern "C" float UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerGetLengthWSA(void* dataSetPtr)
{
    if (dataSetPtr == nullptr)
    {
        return false;
    }

    VideoPlayerHelper* vidPlayerHelper = (VideoPlayerHelper*)dataSetPtr;
    return vidPlayerHelper->GetLength();
}

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerPlayWSA(
    void* dataSetPtr, bool fullScreen, float seekPosition)
{
    if (dataSetPtr == nullptr)
    {
        return false;
    }

    VideoPlayerHelper* vidPlayerHelper = (VideoPlayerHelper*)dataSetPtr;
    return vidPlayerHelper->Play(fullScreen, seekPosition);
}

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerPauseWSA(void* dataSetPtr)
{
    if (dataSetPtr == nullptr)
    {
        return false;
    }

    VideoPlayerHelper* vidPlayerHelper = (VideoPlayerHelper*)dataSetPtr;
    return vidPlayerHelper->Pause();
}

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerStopWSA(void* dataSetPtr)
{
    if (dataSetPtr == nullptr)
    {
        return false;
    }

    VideoPlayerHelper* vidPlayerHelper = (VideoPlayerHelper*)dataSetPtr;
    return vidPlayerHelper->Stop();
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerUpdateVideoDataWSA(void* dataSetPtr)
{
    if (dataSetPtr == nullptr)
    {
        return false;
    }

    VideoPlayerHelper* vidPlayerHelper = (VideoPlayerHelper*)dataSetPtr;
    return (int)vidPlayerHelper->UpdateVideoData();
}

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerSeekToWSA(void* dataSetPtr, float position)
{
    if (dataSetPtr == nullptr)
    {
        return false;
    }

    VideoPlayerHelper* vidPlayerHelper = (VideoPlayerHelper*)dataSetPtr;
    return vidPlayerHelper->SeekTo(position);
}

extern "C" float UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerGetCurrentPositionWSA(void* dataSetPtr)
{
    if (dataSetPtr == nullptr)
    {
        return false;
    }

    VideoPlayerHelper* vidPlayerHelper = (VideoPlayerHelper*)dataSetPtr;
    return vidPlayerHelper->GetCurrentPosition();
}

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerSetVolumeWSA(void* dataSetPtr, float value)
{
    if (dataSetPtr == nullptr)
    {
        return false;
    }

    VideoPlayerHelper* vidPlayerHelper = (VideoPlayerHelper*)dataSetPtr;
    return vidPlayerHelper->SetVolume(value);
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerGetCurrentBufferingPercentageWSA(void* dataSetPtr)
{
    if (dataSetPtr == nullptr)
    {
        return false;
    }

    VideoPlayerHelper* vidPlayerHelper = (VideoPlayerHelper*)dataSetPtr;
    return vidPlayerHelper->GetCurrentBufferingPercentage();
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API VideoPlayerOnPauseWSA(void* dataSetPtr)
{
    if (dataSetPtr == nullptr)
    {
        return;
    }

    VideoPlayerHelper* vidPlayerHelper = (VideoPlayerHelper*)dataSetPtr;
    vidPlayerHelper->OnPause();
}

// -------------------------------------------------------------------------

static void DoEventGraphicsDeviceD3D11(UnityGfxDeviceEventType eventType)
{
    if (eventType == kUnityGfxDeviceEventInitialize)
    {
        IUnityGraphicsD3D11* d3d11 = s_UnityInterfaces->Get<IUnityGraphicsD3D11>();
        s_D3D11Device = d3d11->GetDevice();
    }
    else if (eventType == kUnityGfxDeviceEventShutdown)
    {
        //Release D3D11 resources...
    }
}

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
	UnityGfxRenderer currentDeviceType = s_DeviceType;

	switch (eventType)
	{
	case kUnityGfxDeviceEventInitialize:
		{
			s_DeviceType = s_Graphics->GetRenderer();
			currentDeviceType = s_DeviceType;
			break;
		}

	case kUnityGfxDeviceEventShutdown:
		{
			s_DeviceType = kUnityGfxRendererNull;
			break;
		}

	case kUnityGfxDeviceEventBeforeReset:
		{
			break;
		}

	case kUnityGfxDeviceEventAfterReset:
		{
			break;
		}
	};

    if (currentDeviceType == kUnityGfxRendererD3D11) {
        DoEventGraphicsDeviceD3D11(eventType);
    }		
}

// --------------------------------------------------------------------------

// This is called on the rendering thread
// in response to a Unity GL.IssuePluginEvent() call.
static void UNITY_INTERFACE_API OnRenderEvent(int eventID)
{
	// Unknown graphics device type? Do nothing.
	if (s_DeviceType == kUnityGfxRendererNull)
		return;

	if (s_DeviceType == kUnityGfxRendererD3D11)
	{
        for (auto videoPlayer : s_VideoPlayers)
        {
            if (videoPlayer->GetStatus() == PLAYING) {
                videoPlayer->CopyVideoTexture();
            }
        }
	}
}

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc()
{
	return OnRenderEvent;
}

// --------------------------------------------------------------------------

extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
    OutputDebugString(L"VuforiaMedia: UnityPluginLoad...\n");

    s_UnityInterfaces = unityInterfaces;
    s_Graphics = s_UnityInterfaces->Get<IUnityGraphics>();
    s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);

    // Run OnGraphicsDeviceEvent(initialize) manually on plugin load
    // to not miss the event in case the graphics device is already initialized
    OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
    s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}
