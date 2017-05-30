/*===============================================================================
Copyright (c) 2016 PTC Inc. All Rights Reserved.

Vuforia is a trademark of PTC Inc., registered in the United States and other
countries.
===============================================================================*/
#include "VideoPlayerHelper.h"

using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::Storage::Streams;
using namespace Windows::System::Threading;
using namespace Concurrency;
using namespace VuforiaMedia;

#define MAX_FILENAME_LENGTH 256

// MediaEngineNotify: Implements the callback for Media Engine event notification.
class MediaEngineNotify : public IMFMediaEngineNotify
{
public:
    MediaEngineNotify() : m_refCount(1), m_callback(nullptr)
    {
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        if (__uuidof(IMFMediaEngineNotify) == riid)
        {
            *ppv = static_cast<IMFMediaEngineNotify*>(this);
        }
        else
        {
            *ppv = nullptr;
            return E_NOINTERFACE;
        }

        AddRef();

        return S_OK;
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&m_refCount);
    }

    STDMETHODIMP_(ULONG) Release()
    {
        LONG refCount = InterlockedDecrement(&m_refCount);
        if (refCount == 0)
        {
            delete this;
        }
        return refCount;
    }

    // EventNotify is called when the Media Engine sends an event.
    STDMETHODIMP EventNotify(DWORD mediaEngineEvent, DWORD_PTR param1, DWORD param2)
    {
        if (mediaEngineEvent == MF_MEDIA_ENGINE_EVENT_NOTIFYSTABLESTATE)
        {
            SetEvent(reinterpret_cast<HANDLE>(param1));
        }
        else
        {
            m_callback->OnMediaEngineEvent(mediaEngineEvent);
        }

        return S_OK;
    }

    void SetMediaEngineCallback(MediaEngineCallback* callback)
    {
        m_callback = callback;
    }

private:
    long m_refCount;
    MediaEngineCallback* m_callback;
};


bool VideoPlayerHelper::s_MediaFoundationStarted = false;


VideoPlayerHelper::VideoPlayerHelper(ID3D11Device *d3dDevice) :
    m_d3dDevice(d3dDevice),
    m_videoWidth(0),
    m_videoHeight(0),
    m_videoLengthSeconds(0),
    m_mediaState(NOT_READY),
    m_mediaEngine(nullptr),
    m_mediaEngineEx(nullptr),
    m_sourceUrl(nullptr),
    m_videoTexture(nullptr),
    m_frameTexture(nullptr),
    m_frameTextureInitialized(false),
    m_doUpdateVideoData(false)
{
    OutputDebugString(L"VideoPlayer: Initializing...\n");

    if (d3dDevice == nullptr) {
        OutputDebugString(L"VideoPlayer init: D3D device is null!\n");
    }

    memset(&m_bgColor, 0, sizeof(MFARGB));

    InitializeCriticalSectionEx(&m_criticalSection, 0, 0);

    Initialize();   
}

void VideoPlayerHelper::Initialize()
{
    ComPtr<IMFMediaEngineClassFactory> mediaFactory;
    ComPtr<IMFAttributes> mediaEngineAttributes;
    ComPtr<MediaEngineNotify> mediaEngineNotify;

    if (!s_MediaFoundationStarted)
    {
        OutputDebugString(L"VideoPlayer: Initializing Media Foundation...\n");

        if (SUCCEEDED(MFStartup(MF_VERSION)))
        {
            s_MediaFoundationStarted = true;
            OutputDebugString(L"VideoPlayer: Successfully initialized Media Foundation.\n");
        }
        else
        {
            OutputDebugString(L"VideoPlayer Error: Media Foundation init error!\n");
            m_mediaState = MEDIA_ERROR;
            return;
        }
    }
    

    EnterCriticalSection(&m_criticalSection);

    // Create a DXGI Manager
    UINT resetToken;
    if (FAILED(MFCreateDXGIDeviceManager(&resetToken, &m_DXGIManager))) {
        OutputDebugString(L"VideoPlayer Error: Failed to create DXGI Manager!\n");
        m_mediaState = MEDIA_ERROR;
        LeaveCriticalSection(&m_criticalSection);
        return;
    }

    if (FAILED(m_DXGIManager->ResetDevice(m_d3dDevice, resetToken))) {
        OutputDebugString(L"VideoPlayer Error: Failed to reset D3D device!\n");
        LeaveCriticalSection(&m_criticalSection);
        return;
    }

    // Create Media Engine Factory
    if (FAILED(CoCreateInstance(
        CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&mediaFactory)))) {
        OutputDebugString(L"VideoPlayer Error: Failed to create Media Engine Factory!\n");
        m_mediaState = MEDIA_ERROR;
        LeaveCriticalSection(&m_criticalSection);
        return;
    }

    // Create Media Engine event notifier and set the callback.
    mediaEngineNotify = new MediaEngineNotify();
    mediaEngineNotify->SetMediaEngineCallback(this);

   
    // Create the Media Engine attributes
    if (FAILED(MFCreateAttributes(&mediaEngineAttributes, 1))) {
        OutputDebugString(L"VideoPlayer Error: Failed to create Media Engine Attributes!\n");
        m_mediaState = MEDIA_ERROR;
        LeaveCriticalSection(&m_criticalSection);
        return;
    }
    
    if (FAILED(mediaEngineAttributes->SetUnknown(MF_MEDIA_ENGINE_DXGI_MANAGER, (IUnknown*)m_DXGIManager.Get()))) {
        OutputDebugString(L"VideoPlayer Error: Failed to set Media Engine Attribute!\n");
        m_mediaState = MEDIA_ERROR;
        LeaveCriticalSection(&m_criticalSection);
        return;
    }
    
    if (FAILED(mediaEngineAttributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, (IUnknown*)mediaEngineNotify.Get()))) {
        OutputDebugString(L"VideoPlayer: Failed to set Media Engine Attribute!\n");
        m_mediaState = MEDIA_ERROR;
        LeaveCriticalSection(&m_criticalSection);
        return;
    }

    if (FAILED(mediaEngineAttributes->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT_B8G8R8A8_UNORM))) {
        OutputDebugString(L"VideoPlayer Error: Failed to set Media Engine Attribute!\n");
        m_mediaState = MEDIA_ERROR;
        LeaveCriticalSection(&m_criticalSection);
        return;
    }

    // Create the Media Engine
    const DWORD flags = MF_MEDIA_ENGINE_WAITFORSTABLE_STATE;
    if (FAILED(mediaFactory->CreateInstance(flags, mediaEngineAttributes.Get(), &m_mediaEngine))) {
        OutputDebugString(L"VideoPlayer Error: Failed to create Media Engine instance!\n");
        m_mediaState = MEDIA_ERROR;
        LeaveCriticalSection(&m_criticalSection);
        return;
    }

    if (FAILED(m_mediaEngine.Get()->QueryInterface(__uuidof(IMFMediaEngine), (void**)&m_mediaEngineEx))) {
        OutputDebugString(L"VideoPlayer Error: Failed to query Media Engine interface!\n");
        m_mediaState = MEDIA_ERROR;
        LeaveCriticalSection(&m_criticalSection);
        return;
    }

    LeaveCriticalSection(&m_criticalSection);
}

VideoPlayerHelper::~VideoPlayerHelper()
{
    EnterCriticalSection(&m_criticalSection);

    OutputDebugString(L"VideoPlayer: Shutting down Media Engine...\n");
    m_mediaEngine->Shutdown();
    m_mediaEngineEx->Shutdown();

    m_mediaEngine.Reset();
    m_mediaEngineEx.Reset();
    m_DXGIManager.Reset();

    LeaveCriticalSection(&m_criticalSection);
    DeleteCriticalSection(&m_criticalSection);

    m_frameTexture.Reset();
    m_frameTextureInitialized = false;
    m_videoTexture = nullptr;
    m_d3dDevice = nullptr;
    m_doUpdateVideoData = false;

    if (s_MediaFoundationStarted)
    {
        s_MediaFoundationStarted = false;
        OutputDebugString(L"VideoPlayer: Shutting down Media Foundation...\n");
        MFShutdown();
    }
}

void VideoPlayerHelper::OnMediaEngineEvent(ULONG32 mediaEngineEvent)
{
    switch (mediaEngineEvent)
    {
    case MF_MEDIA_ENGINE_EVENT_LOADEDMETADATA:
    {
        OutputDebugString(L"VideoPlayer: loaded metadata.\n");
        
        EnterCriticalSection(&m_criticalSection);

        // Metadata have been loaded, 
        // so now we can get video width, height and length
        unsigned long videoWidth = 0;
        unsigned long videoHeight = 0;
        HRESULT hres = m_mediaEngine->GetNativeVideoSize(&videoWidth, &videoHeight);
        if (FAILED(hres))
        {
            OutputDebugString(L"VideoPlayer Error: Failed to get native video size!\n");
            break;
        }

        m_videoWidth = (int)videoWidth;
        m_videoHeight = (int)videoHeight;
        m_videoLengthSeconds = (float)m_mediaEngine->GetDuration();
        
        m_targetRect.left = 0;
        m_targetRect.top = 0;
        m_targetRect.right = m_videoWidth;
        m_targetRect.bottom = m_videoHeight;

        // Create the internal video frame texture
        int rowPitch = m_videoWidth * 4;
        int frameSize = rowPitch * m_videoHeight;
        uint8_t *frameBytes = new (std::nothrow) uint8_t[frameSize];
        memset(frameBytes, 0, frameSize);

        D3D11_SUBRESOURCE_DATA initData;
        initData.pSysMem = frameBytes;
        initData.SysMemPitch = static_cast<UINT>(rowPitch);
        initData.SysMemSlicePitch = static_cast<UINT>(frameSize);

        ZeroMemory(&m_frameTexDesc, sizeof(D3D11_TEXTURE2D_DESC));
        m_frameTexDesc.Width = m_videoWidth;
        m_frameTexDesc.Height = m_videoHeight;
        m_frameTexDesc.MipLevels = 0;
        m_frameTexDesc.ArraySize = 1;
        m_frameTexDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        m_frameTexDesc.SampleDesc.Count = 1;
        m_frameTexDesc.SampleDesc.Quality = 0;
        m_frameTexDesc.Usage = D3D11_USAGE_DEFAULT;
        m_frameTexDesc.CPUAccessFlags = 0;
        m_frameTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        m_frameTexDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

        LeaveCriticalSection(&m_criticalSection);
    }
    break;
    case MF_MEDIA_ENGINE_EVENT_CANPLAY:
    {
        m_mediaState = READY;
        OutputDebugString(L"VideoPlayer: Media ready to play.\n");
    }
    break;
    case MF_MEDIA_ENGINE_EVENT_PLAY:
    {
        OutputDebugString(L"VideoPlayer: Media playing.\n");
        m_mediaState = PLAYING;
    }
    break;
    case MF_MEDIA_ENGINE_EVENT_PAUSE:
    {
        OutputDebugString(L"VideoPlayer: Media paused.\n");
        m_mediaState = PAUSED;
    }
    break;
    case MF_MEDIA_ENGINE_EVENT_ENDED:
    {
        OutputDebugString(L"VideoPlayer: Media reached end.\n");
        m_mediaState = REACHED_END;
    }
    break;
    case MF_MEDIA_ENGINE_EVENT_ERROR:
    {
        OutputDebugString(L"VideoPlayer: Media Error.\n");
        m_mediaState = MEDIA_ERROR;
    }
    break;
    case MF_MEDIA_ENGINE_EVENT_STALLED:
    {
        OutputDebugString(L"VideoPlayer: Media Stalled.\n");
        m_mediaState = MEDIA_ERROR;
    }
    break;
    }
}

bool VideoPlayerHelper::SetVideoTexturePtr(ID3D11Texture2D* texturePtr)
{
    EnterCriticalSection(&m_criticalSection);

    m_videoTexture = texturePtr;

    LeaveCriticalSection(&m_criticalSection);

    return true;
}

bool VideoPlayerHelper::IsPlayableOnTexture()
{
    return true;
}

bool VideoPlayerHelper::Load(const char *filename, int requestType, bool playOnTextureImmediately, float seekPosition)
{
    OutputDebugString(L"VideoPlayer: Loading file...\n");
    
    size_t strLen = strlen(filename) + 1;
    if (strLen > MAX_FILENAME_LENGTH) strLen = MAX_FILENAME_LENGTH;
    wchar_t filenameWChar[MAX_FILENAME_LENGTH];
    size_t numChars;
    mbstowcs_s(&numChars, filenameWChar, filename, strLen);

    OutputDebugString(filenameWChar);
    OutputDebugString(L"\n");
    
    Uri^ uri = ref new Uri(ref new Platform::String(filenameWChar));
    task<StorageFile^> getFileTask(StorageFile::GetFileFromApplicationUriAsync(uri));
    
    getFileTask.then([this](StorageFile^ file)
    {
        if (!file) {
            OutputDebugString(L"VideoPlayer: file loading failed!");
            throw ref new Platform::Exception(E_FAIL);
        }

        task<IRandomAccessStream^> openFileTask(file->OpenAsync(Windows::Storage::FileAccessMode::Read));

        // Stor the file URL
        Platform::String^ filePath = file->Path;
        if (m_sourceUrl != nullptr)
        {
            ::CoTaskMemFree(m_sourceUrl);
            m_sourceUrl = nullptr;
        }
        size_t filePathStrLen = 1 + ::wcslen(filePath->Data());
        m_sourceUrl = (LPWSTR)::CoTaskMemAlloc(sizeof(WCHAR) * filePathStrLen);
        StringCchCopyW(m_sourceUrl, filePathStrLen, filePath->Data());

        openFileTask.then([this](IRandomAccessStream^ stream)
        {
            this->SetSourceStream(stream);
        });
    });

    return true;
}

void VideoPlayerHelper::SetSourceStream(IRandomAccessStream^ stream)
{
    if (!stream) {
        OutputDebugString(L"VideoPlayer Error: NULL stream!\n");
        return;
    }

    ComPtr<IMFByteStream> byteStream = nullptr;

    EnterCriticalSection(&m_criticalSection);

    HRESULT hres;
    hres = MFCreateMFByteStreamOnStreamEx((IUnknown*)stream, &byteStream);    
    if (FAILED(hres)) {
        OutputDebugString(L"VideoPlayer Error: Failed to create Media byte stream!\n");
        m_mediaState = MEDIA_ERROR;
        LeaveCriticalSection(&m_criticalSection);
        return;
    }

    hres = m_mediaEngineEx->SetSourceFromByteStream(byteStream.Get(), m_sourceUrl);
    if (FAILED(hres)) {
        OutputDebugString(L"VideoPlayer Error: Failed to set source from byte stream!\n");
        m_mediaState = MEDIA_ERROR;
        LeaveCriticalSection(&m_criticalSection);
        return;
    }

    LeaveCriticalSection(&m_criticalSection);
}

bool VideoPlayerHelper::Play(bool fullScreen, float seekPosition)
{
    if (m_mediaState == PLAYING)
    {
        // already playing, skip
        return true;
    }

    if (!m_mediaEngine || !m_mediaEngine->HasVideo())
    {
        OutputDebugString(L"VideoPlayer: Play skipped: Media Engine not ready!\n");
        return false;
    }

    if (m_mediaState == REACHED_END)
    {
        SeekTo(0);
        m_mediaState = READY;
    }

    EnterCriticalSection(&m_criticalSection);
    HRESULT hres = m_mediaEngine->Play();
    LeaveCriticalSection(&m_criticalSection);

    if (SUCCEEDED(hres))
    {
        m_mediaState = PLAYING;
        return true;
    }
    
    return false;
}

bool VideoPlayerHelper::Pause()
{
    EnterCriticalSection(&m_criticalSection);

    bool success = false;
    if (m_mediaEngine && m_mediaEngine->HasVideo())
    {
        success = SUCCEEDED(m_mediaEngine->Pause());
    }

    LeaveCriticalSection(&m_criticalSection);
    
    return success;
}

bool VideoPlayerHelper::Stop()
{
    if (m_mediaState == PLAYING) {
        Pause();
    }
    m_mediaState = STOPPED;
    return true;
}

void VideoPlayerHelper::OnPause()
{
    Pause();
}

bool VideoPlayerHelper::Unload()
{
    if (m_mediaEngine)
    {
        if (m_mediaState == PLAYING) {
            Pause();
        }
        
        EnterCriticalSection(&m_criticalSection);
        HRESULT hres = m_mediaEngine->Shutdown();
        LeaveCriticalSection(&m_criticalSection);
        return SUCCEEDED(hres);
    }
    return false;
}
 
int VideoPlayerHelper::GetVideoWidth()
{
    return m_videoWidth;
}

int VideoPlayerHelper::GetVideoHeight()
{
    return m_videoHeight;
}

float VideoPlayerHelper::GetLength()
{
    return m_videoLengthSeconds;
}

float VideoPlayerHelper::GetCurrentPosition()
{
    EnterCriticalSection(&m_criticalSection);

    float currentTime = 0;
    if (m_mediaEngine && m_mediaEngine->HasVideo())
    {
        currentTime = (float)m_mediaEngine->GetCurrentTime();
    }
    
    LeaveCriticalSection(&m_criticalSection);

    return currentTime;
}

bool VideoPlayerHelper::SeekTo(float pos)
{
    EnterCriticalSection(&m_criticalSection);

    bool success = false;
    if (m_mediaEngine && m_mediaEngine->HasVideo())
    {
        success = SUCCEEDED(m_mediaEngine->SetCurrentTime(pos));
    }

    LeaveCriticalSection(&m_criticalSection);

    return success;
}

bool VideoPlayerHelper::SetVolume(float volume)
{
    EnterCriticalSection(&m_criticalSection);

    bool success = false;
    if (m_mediaEngine && m_mediaEngine->HasVideo())
    {
        success = SUCCEEDED(m_mediaEngine->SetVolume(volume));
    }

    LeaveCriticalSection(&m_criticalSection);

    return success;
}

// This method we are called from the Unity rendering thread
// so access to D3D device is guaranteed to be safe here
void VideoPlayerHelper::CopyVideoTexture()
{
    EnterCriticalSection(&m_criticalSection);

    if ((m_mediaState == PLAYING) && 
        m_d3dDevice && m_videoTexture)
    {
        LONGLONG frameTime;
        if (m_doUpdateVideoData && 
            SUCCEEDED(m_mediaEngine->OnVideoStreamTick(&frameTime)))
        {
            // Init frame texture if not yet initialized
            if (!m_frameTextureInitialized) 
            {
                HRESULT hres = m_d3dDevice->CreateTexture2D(&m_frameTexDesc, nullptr, m_frameTexture.GetAddressOf());
                if (SUCCEEDED(hres)) {
                    m_frameTextureInitialized = true;
                }
                else {
                    OutputDebugString(L"VideoPlayer Error: Failed to create internal frame texture!\n");
                }
            }

            // Transfer the video frame to the frame texture
            if (m_frameTextureInitialized) 
            {
                HRESULT hres = m_mediaEngine->TransferVideoFrame(
                    m_frameTexture.Get(), nullptr, &m_targetRect, &m_bgColor);

                if (FAILED(hres)) {
                    OutputDebugString(L"VideoPlayer Error: video texture update error!\n");
                }
            }

            // Copy the frame textiure to the target video texture
            ID3D11DeviceContext *context;
            m_d3dDevice->GetImmediateContext(&context);
            if (context != nullptr)
            {
                context->CopySubresourceRegion(
                    m_videoTexture, 0, 0, 0, 0,
                    m_frameTexture.Get(), 0, nullptr
                );
            }

            m_doUpdateVideoData = false;
        }
    }

    LeaveCriticalSection(&m_criticalSection);
}

MediaState VideoPlayerHelper::UpdateVideoData()
{
    if (m_mediaEngine && m_mediaEngine->HasVideo() && 
        m_mediaState == PLAYING)
    {
        m_doUpdateVideoData = true;
    }
    
    return m_mediaState;
}

int VideoPlayerHelper::GetCurrentBufferingPercentage()
{
    EnterCriticalSection(&m_criticalSection);

    double ratio = 0;
    if (m_mediaEngine && m_mediaEngine->HasVideo())
    {
        IMFMediaTimeRange *seekableRange;
        m_mediaEngine->GetSeekable(&seekableRange);
        if (seekableRange->GetLength() > 0) {
            double end = 0;
            seekableRange->GetEnd(seekableRange->GetLength() - 1, &end);
            ratio = end / m_mediaEngine->GetDuration();
            if (ratio < 0) ratio = 0;
            if (ratio > 1) ratio = 1;
        }
    }

    LeaveCriticalSection(&m_criticalSection);

    return (int)(100 * ratio);
}
