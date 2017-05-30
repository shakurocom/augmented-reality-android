/*===============================================================================
Copyright (c) 2016 PTC Inc. All Rights Reserved.

Vuforia is a trademark of PTC Inc., registered in the United States and other
countries.
===============================================================================*/
#ifndef _VUFORIA_MEDIA_WSA_VIDEO_PLAYER_HELPER_H_
#define _VUFORIA_MEDIA_WSA_VIDEO_PLAYER_HELPER_H_

#include <wrl.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>
#include <agile.h>
#include <concrt.h>
#include <collection.h>
#include <mfmediaengine.h>
#include <Mfapi.h>
#include <ppltasks.h>
#include <Strsafe.h>

namespace VuforiaMedia
{
    enum MediaState {
        REACHED_END = 0,
        PAUSED = 1,
        STOPPED = 2,
        PLAYING = 3,
        READY = 4,
        NOT_READY = 5,
        MEDIA_ERROR = 6
    };

    class MediaEngineCallback
    {
    public:
        virtual void OnMediaEngineEvent(ULONG32 mediaEngineEvent) = 0;
    };

    class VideoPlayerHelper : public MediaEngineCallback
    {
    public:
        static bool s_MediaFoundationStarted;

        VideoPlayerHelper(ID3D11Device* d3dDevice);
        virtual ~VideoPlayerHelper();
        
        bool SetVideoTexturePtr(ID3D11Texture2D* texturePtr);
        bool Load(const char* filename, int requestType, bool playOnTextureImmediately, float seekPosition);
        bool Unload();
        bool Pause();
        bool Play(bool fullScreen, float seekPosition);
        bool Stop();
        void OnPause();
        MediaState GetStatus() { return m_mediaState; }
        bool IsPlayableOnTexture();
        bool IsPlayableFullscreen() { return false; }
        int GetVideoWidth();
        int GetVideoHeight();
        float GetLength();
        int GetCurrentBufferingPercentage();
        float GetCurrentPosition();
        bool SeekTo(float pos);
        bool SetVolume(float volume);
        MediaState UpdateVideoData();
        void CopyVideoTexture();
        
        // Media Engine notify callback interface
        virtual void OnMediaEngineEvent(ULONG32 mediaEngineEvent) override;

    private:
        void Initialize();
        void SetSourceStream(Windows::Storage::Streams::IRandomAccessStream^ stream);

        inline void ThrowIfFailed(HRESULT hres)
        {
            if (FAILED(hres))
                throw Platform::Exception::CreateException(hres);
        }

        MediaState m_mediaState;
        CRITICAL_SECTION m_criticalSection;
        BSTR   m_sourceUrl;
        MFARGB m_bgColor;
        RECT   m_targetRect;

        int m_videoWidth;
        int m_videoHeight;
        float m_videoLengthSeconds;

        ID3D11Device* m_d3dDevice;
        ID3D11Texture2D* m_videoTexture;
        D3D11_TEXTURE2D_DESC m_frameTexDesc;
        bool m_frameTextureInitialized;
        volatile bool m_doUpdateVideoData;
       
        Microsoft::WRL::ComPtr<ID3D11Texture2D>       m_frameTexture;
        Microsoft::WRL::ComPtr<IMFMediaEngine>        m_mediaEngine;
        Microsoft::WRL::ComPtr<IMFMediaEngineEx>      m_mediaEngineEx;
        Microsoft::WRL::ComPtr<IMFDXGIDeviceManager>  m_DXGIManager;
        
        Windows::System::Threading::ThreadPoolTimer^  m_updateTimer;
    };
}

#endif // _VUFORIA_MEDIA_WSA_VIDEO_PLAYER_HELPER_H_
