/*===============================================================================
Copyright (c) 2015-2016 PTC Inc. All Rights Reserved.
 
Copyright (c) 2012-2014 Qualcomm Connected Experiences, Inc. All Rights Reserved.
 
Vuforia is a trademark of PTC Inc., registered in the United States and other 
countries.
==============================================================================*/

using UnityEngine;
using System.Collections;
using System.Runtime.InteropServices;
using System.IO;
using System;
using Vuforia;

/// <summary>
/// This is a utility class to actually play back videos on a texture or in full screen.
/// </summary>
public class VideoPlayerHelper
{
    #region NESTED

    /// <summary>
    /// Various states a video can be in
    /// </summary>
    public enum MediaState
    {
        REACHED_END,
        PAUSED,
        STOPPED,
        PLAYING,
        READY,
        NOT_READY,
        ERROR,
        PLAYING_FULLSCREEN // iOS-only
    }

    /// <summary>
    /// If the video can be played back on texture, in full screen on both.
    /// </summary>
    public enum MediaType
    {
        ON_TEXTURE,
        FULLSCREEN,
        ON_TEXTURE_FULLSCREEN
    }

    #endregion // NESTED



    #region PRIVATE_MEMBER_VARIABLES

    private string mFilename = null;
    private string mFullScreenFilename = null;

    #endregion // PRIVATE_MEMBER_VARIABLES



    #region PUBLIC_METHODS

    /// <summary>
    /// Get the native plugin render event function
    /// </summary>
    public static IntPtr GetNativeRenderEventFunc()
    {
#if UNITY_WSA_10_0 && !UNITY_EDITOR
        return GetRenderEventFunc();
#else
        return IntPtr.Zero;
#endif
    }

    /// <summary>
    /// Set the video filename
    /// </summary>
    public void SetFilename(string filename)
    {
#if UNITY_ANDROID

        mFilename = filename;

        videoPlayerSetActivity();

        if (videoPlayerIsFileInAssetsFolder(filename) || filename.Contains("://"))
        {
            mFullScreenFilename = filename;
        }
        else
        {
            mFullScreenFilename = "file://" + filename;
        }

#elif (UNITY_IPHONE || UNITY_IOS)

        mFilename = filename;

        if (!filename.Contains("://"))
        {
            // Not a remote file, assume this file is located in the StreamingAssets folder
            mFilename = "Data/Raw/" + filename;
        }

        mFullScreenFilename = filename;
		
#elif UNITY_WSA_10_0

        mFilename = filename;
		
        if (!filename.Contains("://"))
        {
            // Not a remote file, assume this file is located in the StreamingAssets folder
            mFilename = "ms-appx:///Data/StreamingAssets/" + filename;
        }
		
        mFullScreenFilename = mFilename;

#endif
    }


    /// <summary>
    /// Initializes the VideoPlayerHelper object
    /// </summary>
    public bool Init(VuforiaRenderer.RendererAPI rendererAPI)
    {
        return videoPlayerInit(rendererAPI);
    }


    /// <summary>
    /// Deinitializes the VideoPlayerHelper object
    /// </summary>
    /// <returns></returns>
    public bool Deinit()
    {
        return videoPlayerDeinit();
    }


    /// <summary>
    /// Loads a local or remote movie file
    /// </summary>
    public bool Load(string filename, MediaType requestedType, bool playOnTextureImmediately, float seekPosition)
    {
        SetFilename(filename);
        return videoPlayerLoad(mFilename, (int) requestedType, playOnTextureImmediately, seekPosition);
    }


    /// <summary>
    /// Unloads the currently loaded movie
    /// After this is called a new load() has to be invoked
    /// </summary>
    public bool Unload()
    {
        return videoPlayerUnload();
    }


    /// <summary>
    /// Indicates whether the movie can be played on a texture
    /// </summary>
    public bool IsPlayableOnTexture()
    {
        return videoPlayerIsPlayableOnTexture();
    }


    /// <summary>
    /// Indicates whether the movie can be played fullscreen
    /// </summary>
    public bool IsPlayableFullscreen()
    {
        return videoPlayerIsPlayableFullscreen();
    }


    /// <summary>
    /// Set the native texture object that the video frames will be copied to
    /// </summary>
    public bool SetVideoTexturePtr(IntPtr texturePtr)
    {
        return videoPlayerSetVideoTexturePtr(texturePtr);
    }


    /// <summary>
    /// Return the current status of the movie such as Playing, Paused or Not Ready
    /// </summary>
    public MediaState GetStatus()
    {
        return (MediaState) videoPlayerGetStatus();
    }


    /// <summary>
    /// Returns the width of the video frame
    /// </summary>
    public int GetVideoWidth()
    {
        return videoPlayerGetVideoWidth();
    }


    /// <summary>
    /// Returns the height of the video frame
    /// </summary>
    public int GetVideoHeight()
    {
        return videoPlayerGetVideoHeight();
    }


    /// <summary>
    /// Returns the length of the current movie
    /// </summary>
    public float GetLength()
    {
        return videoPlayerGetLength();
    }


    /// <summary>
    /// Request a movie to be played either full screen or on texture and at a given position
    /// </summary>
    public bool Play(bool fullScreen, float seekPosition)
    {
        // We use Unity's built-in full screen movie player
        if (fullScreen)
        {
            if (mFilename == null)
            {
                return false;
            }

            Handheld.PlayFullScreenMovie(mFullScreenFilename, Color.black, FullScreenMovieControlMode.Full, FullScreenMovieScalingMode.AspectFit);
            return true;
        }
        else
        {
            return videoPlayerPlay(fullScreen, seekPosition);
        }
    }


    /// <summary>
    /// Pauses the current movie being played
    /// </summary>
    public bool Pause()
    {
        return videoPlayerPause();
    }


    /// <summary>
    /// Stops the current movie being played
    /// </summary>
    public bool Stop()
    {
        return videoPlayerStop();
    }


    /// <summary>
    /// Tells the VideoPlayerHelper to update the data from the video feed
    /// </summary>
    public MediaState UpdateVideoData()
    {
        return (MediaState)videoPlayerUpdateVideoData();
    }


    /// <summary>
    /// Moves the movie to the requested seek position
    /// </summary>
    public bool SeekTo(float position)
    {
        return videoPlayerSeekTo(position);
    }


    /// <summary>
    /// Gets the current seek position
    /// </summary>
    public float GetCurrentPosition()
    {
        return videoPlayerGetCurrentPosition();
    }


    /// <summary>
    /// Sets the volume of the movie to the desired value
    /// </summary>
    public bool SetVolume(float value)
    {
        return videoPlayerSetVolume(value);
    }


    /// <summary>
    /// Gets the buffering percentage in case the movie is loaded from network
    /// Note this is not supported on iOS
    /// </summary>
    public int GetCurrentBufferingPercentage()
    {
        return videoPlayerGetCurrentBufferingPercentage();
    }


    /// <summary>
    /// Allows native player to do appropriate on pause cleanup
    /// </summary>
    public void OnPause()
    {
        videoPlayerOnPause();
    }

#endregion // PUBLIC_METHODS



#region NATIVE_FUNCTIONS

#if !UNITY_EDITOR

#if UNITY_ANDROID

    private AndroidJavaObject javaObj = null;

    private AndroidJavaObject GetJavaObject()
    {
        if (javaObj == null)
        {
            javaObj = new AndroidJavaObject("com.vuforia.VuforiaMedia.VideoPlayerHelper");
        }

        return javaObj;
    }

    private void videoPlayerSetActivity()
    {
        AndroidJavaClass jc = new AndroidJavaClass("com.unity3d.player.UnityPlayer");
        AndroidJavaObject jo = jc.GetStatic<AndroidJavaObject>("currentActivity");
        GetJavaObject().Call("setActivity", jo);
    }

    private bool videoPlayerIsFileInAssetsFolder(string filename)
    {
        return GetJavaObject().Call<bool>("isFileInAssetsFolder", filename);
    }

    private bool videoPlayerInit(VuforiaRenderer.RendererAPI rendererAPI)
    {
        int openGLVersion = -1;
        if(rendererAPI == VuforiaRenderer.RendererAPI.GL_30)
        {
            openGLVersion = 3;
        } else if(rendererAPI == VuforiaRenderer.RendererAPI.GL_20)
        {
            openGLVersion = 2;
        } else {
            Debug.LogError("Incorrect Renderer API, setting it to OpenGL 2");
            openGLVersion = 2;
        }
        return GetJavaObject().Call<bool>("init", openGLVersion);
    }

    private bool videoPlayerDeinit()
    {
        return GetJavaObject().Call<bool>("deinit");
    }

    private bool videoPlayerLoad(string filename, int requestType, bool playOnTextureImmediately, float seekPosition)
    {
        return GetJavaObject().Call<bool>("load", filename, requestType, playOnTextureImmediately, seekPosition);
    }

    private bool videoPlayerUnload()
    {
        return GetJavaObject().Call<bool>("unload");
    }

    private bool videoPlayerIsPlayableOnTexture()
    {
        return GetJavaObject().Call<bool>("isPlayableOnTexture");
    }

    private bool videoPlayerIsPlayableFullscreen()
    {
        return GetJavaObject().Call<bool>("isPlayableFullscreen");
    }

    private bool videoPlayerSetVideoTexturePtr(IntPtr texturePtr)
    {
        // Android OpenGL ES: cast texturePtr to OpenGL texture ID (32bit integer), as documented here:
        // http://docs.unity3d.com/ScriptReference/Texture.GetNativeTexturePtr.html
        return GetJavaObject().Call<bool>("setVideoTextureID", texturePtr.ToInt32() );
    }

    private int videoPlayerGetStatus()
    {
        return GetJavaObject().Call<int>("getStatus");
    }

    private int videoPlayerGetVideoWidth()
    {
        return GetJavaObject().Call<int>("getVideoWidth");
    }

    private int videoPlayerGetVideoHeight()
    {
        return GetJavaObject().Call<int>("getVideoHeight");
    }

    private float videoPlayerGetLength()
    {
        return GetJavaObject().Call<float>("getLength");
    }

    private bool videoPlayerPlay(bool fullScreen, float seekPosition)
    {
        return GetJavaObject().Call<bool>("play", fullScreen, seekPosition);
    }

    private bool videoPlayerPause()
    {
        return GetJavaObject().Call<bool>("pause");
    }

    private bool videoPlayerStop()
    {
        return GetJavaObject().Call<bool>("stop");
    }

    private int videoPlayerUpdateVideoData()
    {
        return GetJavaObject().Call<int>("updateVideoData");
    }

    private bool videoPlayerSeekTo(float position)
    {
        return GetJavaObject().Call<bool>("seekTo", position);
    }

    private float videoPlayerGetCurrentPosition()
    {
        return GetJavaObject().Call<float>("getCurrentPosition");
    }

    private bool videoPlayerSetVolume(float value)
    {
        return GetJavaObject().Call<bool>("setVolume", value);
    }

    private int videoPlayerGetCurrentBufferingPercentage()
    {
        return GetJavaObject().Call<int>("getCurrentBufferingPercentage");
    }

    private void videoPlayerOnPause()
    {
        // nothing to do for Android
    }

#elif (UNITY_IPHONE || UNITY_IOS)

    private IntPtr mVideoPlayerPtr = IntPtr.Zero;

    [DllImport("__Internal")]
    private static extern IntPtr videoPlayerInitIOS(bool isMetalRendering);

    [DllImport("__Internal")]
    private static extern bool videoPlayerDeinitIOS(IntPtr videoPlayerPtr);

    [DllImport("__Internal")]
    private static extern bool videoPlayerLoadIOS(IntPtr videoPlayerPtr, string filename, int requestType, bool playOnTextureImmediately, float seekPosition);

    [DllImport("__Internal")]
    private static extern bool videoPlayerUnloadIOS(IntPtr videoPlayerPtr);

    [DllImport("__Internal")]
    private static extern bool videoPlayerIsPlayableOnTextureIOS(IntPtr videoPlayerPtr);

    [DllImport("__Internal")]
    private static extern bool videoPlayerIsPlayableFullscreenIOS(IntPtr videoPlayerPtr);

    [DllImport("__Internal")]
    private static extern bool videoPlayerSetVideoTexturePtrIOS(IntPtr videoPlayerPtr, IntPtr texturePtr);

    [DllImport("__Internal")]
    private static extern int videoPlayerGetStatusIOS(IntPtr videoPlayerPtr);

    [DllImport("__Internal")]
    private static extern int videoPlayerGetVideoWidthIOS(IntPtr videoPlayerPtr);

    [DllImport("__Internal")]
    private static extern int videoPlayerGetVideoHeightIOS(IntPtr videoPlayerPtr);

    [DllImport("__Internal")]
    private static extern float videoPlayerGetLengthIOS(IntPtr videoPlayerPtr);

    [DllImport("__Internal")]
    private static extern bool videoPlayerPlayIOS(IntPtr videoPlayerPtr, bool fullScreen, float seekPosition);

    [DllImport("__Internal")]
    private static extern bool videoPlayerPauseIOS(IntPtr videoPlayerPtr);

    [DllImport("__Internal")]
    private static extern bool videoPlayerStopIOS(IntPtr videoPlayerPtr);

    [DllImport("__Internal")]
    private static extern int videoPlayerUpdateVideoDataIOS(IntPtr videoPlayerPtr);

    [DllImport("__Internal")]
    private static extern bool videoPlayerSeekToIOS(IntPtr videoPlayerPtr, float position);

    [DllImport("__Internal")]
    private static extern float videoPlayerGetCurrentPositionIOS(IntPtr videoPlayerPtr);

    [DllImport("__Internal")]
    private static extern bool videoPlayerSetVolumeIOS(IntPtr videoPlayerPtr, float value);

    [DllImport("__Internal")]
    private static extern int videoPlayerGetCurrentBufferingPercentageIOS(IntPtr videoPlayerPtr);

    [DllImport("__Internal")]
    private static extern void videoPlayerOnPauseIOS(IntPtr videoPlayerPtr);


    private bool videoPlayerInit(VuforiaRenderer.RendererAPI rendererAPI)
    {
        mVideoPlayerPtr = videoPlayerInitIOS(rendererAPI == VuforiaRenderer.RendererAPI.METAL);
        return mVideoPlayerPtr != IntPtr.Zero;
    }

    private bool videoPlayerDeinit()
    {
        bool result = videoPlayerDeinitIOS(mVideoPlayerPtr);
        mVideoPlayerPtr = IntPtr.Zero;
        return result;
    }

    private bool videoPlayerLoad(string filename, int requestType, bool playOnTextureImmediately, float seekPosition)
    {
        return videoPlayerLoadIOS(mVideoPlayerPtr, filename, requestType, playOnTextureImmediately, seekPosition);
    }

    private bool videoPlayerUnload()
    {
        return videoPlayerUnloadIOS(mVideoPlayerPtr);
    }

    private bool videoPlayerIsPlayableOnTexture()
    {
        return videoPlayerIsPlayableOnTextureIOS(mVideoPlayerPtr);
    }

    private bool videoPlayerIsPlayableFullscreen()
    {
        return videoPlayerIsPlayableFullscreenIOS(mVideoPlayerPtr);
    }

    private bool videoPlayerSetVideoTexturePtr(IntPtr texturePtr)
    {
        return videoPlayerSetVideoTexturePtrIOS(mVideoPlayerPtr, texturePtr);
    }

    private int videoPlayerGetStatus()
    {
        return videoPlayerGetStatusIOS(mVideoPlayerPtr);
    }

    private int videoPlayerGetVideoWidth()
    {
        return videoPlayerGetVideoWidthIOS(mVideoPlayerPtr);
    }

    private int videoPlayerGetVideoHeight()
    {
        return videoPlayerGetVideoHeightIOS(mVideoPlayerPtr);
    }

    private float videoPlayerGetLength()
    {
        return videoPlayerGetLengthIOS(mVideoPlayerPtr);
    }

    private bool videoPlayerPlay(bool fullScreen, float seekPosition)
    {
        return videoPlayerPlayIOS(mVideoPlayerPtr, fullScreen, seekPosition);
    }

    private bool videoPlayerPause()
    {
        return videoPlayerPauseIOS(mVideoPlayerPtr);
    }

    private bool videoPlayerStop()
    {
        return videoPlayerStopIOS(mVideoPlayerPtr);
    }

    private int videoPlayerUpdateVideoData()
    {
        return videoPlayerUpdateVideoDataIOS(mVideoPlayerPtr);
    }

    private bool videoPlayerSeekTo(float position)
    {
        return videoPlayerSeekToIOS(mVideoPlayerPtr, position);
    }

    private float videoPlayerGetCurrentPosition()
    {
        return videoPlayerGetCurrentPositionIOS(mVideoPlayerPtr);
    }

    private bool videoPlayerSetVolume(float value)
    {
        return videoPlayerSetVolumeIOS(mVideoPlayerPtr, value);
    }

    private int videoPlayerGetCurrentBufferingPercentage()
    {
        return videoPlayerGetCurrentBufferingPercentageIOS(mVideoPlayerPtr);
    }

    private void videoPlayerOnPause()
    {
        videoPlayerOnPauseIOS(mVideoPlayerPtr);
    }

#elif (UNITY_WSA_10_0)

    [DllImport("VuforiaMedia")]
    private static extern IntPtr GetRenderEventFunc();


    private IntPtr mVideoPlayerPtr = IntPtr.Zero;
    
    [DllImport("VuforiaMedia")]
    private static extern IntPtr VideoPlayerInitWSA();

    [DllImport("VuforiaMedia")]
    private static extern bool VideoPlayerDeinitWSA(IntPtr videoPlayerPtr);

    [DllImport("VuforiaMedia")]
    private static extern bool VideoPlayerLoadWSA(IntPtr videoPlayerPtr, [MarshalAs(UnmanagedType.LPStr)] string filename, int requestType, bool playOnTextureImmediately, float seekPosition);

    [DllImport("VuforiaMedia")]
    private static extern bool VideoPlayerUnloadWSA(IntPtr videoPlayerPtr);

    [DllImport("VuforiaMedia")]
    private static extern bool VideoPlayerIsPlayableOnTextureWSA(IntPtr videoPlayerPtr);

    [DllImport("VuforiaMedia")]
    private static extern bool VideoPlayerIsPlayableFullscreenWSA(IntPtr videoPlayerPtr);

    [DllImport("VuforiaMedia")]
    private static extern bool VideoPlayerSetVideoTexturePtrWSA(IntPtr videoPlayerPtr, IntPtr texturePtr);

    [DllImport("VuforiaMedia")]
    private static extern int VideoPlayerGetStatusWSA(IntPtr videoPlayerPtr);

    [DllImport("VuforiaMedia")]
    private static extern int VideoPlayerGetVideoWidthWSA(IntPtr videoPlayerPtr);

    [DllImport("VuforiaMedia")]
    private static extern int VideoPlayerGetVideoHeightWSA(IntPtr videoPlayerPtr);

    [DllImport("VuforiaMedia")]
    private static extern float VideoPlayerGetLengthWSA(IntPtr videoPlayerPtr);

    [DllImport("VuforiaMedia")]
    private static extern bool VideoPlayerPlayWSA(IntPtr videoPlayerPtr, bool fullScreen, float seekPosition);

    [DllImport("VuforiaMedia")]
    private static extern bool VideoPlayerPauseWSA(IntPtr videoPlayerPtr);

    [DllImport("VuforiaMedia")]
    private static extern bool VideoPlayerStopWSA(IntPtr videoPlayerPtr);

    [DllImport("VuforiaMedia")]
    private static extern int VideoPlayerUpdateVideoDataWSA(IntPtr videoPlayerPtr);

    [DllImport("VuforiaMedia")]
    private static extern bool VideoPlayerSeekToWSA(IntPtr videoPlayerPtr, float position);

    [DllImport("VuforiaMedia")]
    private static extern float VideoPlayerGetCurrentPositionWSA(IntPtr videoPlayerPtr);

    [DllImport("VuforiaMedia")]
    private static extern bool VideoPlayerSetVolumeWSA(IntPtr videoPlayerPtr, float value);

    [DllImport("VuforiaMedia")]
    private static extern int VideoPlayerGetCurrentBufferingPercentageWSA(IntPtr videoPlayerPtr);

    [DllImport("VuforiaMedia")]
    private static extern void VideoPlayerOnPauseWSA(IntPtr videoPlayerPtr);


    private bool videoPlayerInit(VuforiaRenderer.RendererAPI unused_rendererAPI)
    {
        mVideoPlayerPtr = VideoPlayerInitWSA();
        return mVideoPlayerPtr != IntPtr.Zero;
    }

    private bool videoPlayerDeinit()
    {
        bool result = VideoPlayerDeinitWSA(mVideoPlayerPtr);
        mVideoPlayerPtr = IntPtr.Zero;
        return result;
    }

    private bool videoPlayerLoad(string filename, int requestType, bool playOnTextureImmediately, float seekPosition)
    {
        return VideoPlayerLoadWSA(mVideoPlayerPtr, filename, requestType, playOnTextureImmediately, seekPosition);
    }

    private bool videoPlayerUnload()
    {
        return VideoPlayerUnloadWSA(mVideoPlayerPtr);
    }

    private bool videoPlayerIsPlayableOnTexture()
    {
        return VideoPlayerIsPlayableOnTextureWSA(mVideoPlayerPtr);
    }

    private bool videoPlayerIsPlayableFullscreen()
    {
        return VideoPlayerIsPlayableFullscreenWSA(mVideoPlayerPtr);
    }

    private bool videoPlayerSetVideoTexturePtr(IntPtr texturePtr)
    {
        return VideoPlayerSetVideoTexturePtrWSA(mVideoPlayerPtr, texturePtr);
    }

    private int videoPlayerGetStatus()
    {
        return VideoPlayerGetStatusWSA(mVideoPlayerPtr);
    }

    private int videoPlayerGetVideoWidth()
    {
        return VideoPlayerGetVideoWidthWSA(mVideoPlayerPtr);
    }

    private int videoPlayerGetVideoHeight()
    {
        return VideoPlayerGetVideoHeightWSA(mVideoPlayerPtr);
    }

    private float videoPlayerGetLength()
    {
        return VideoPlayerGetLengthWSA(mVideoPlayerPtr);
    }

    private bool videoPlayerPlay(bool fullScreen, float seekPosition)
    {
        return VideoPlayerPlayWSA(mVideoPlayerPtr, fullScreen, seekPosition);
    }

    private bool videoPlayerPause()
    {
        return VideoPlayerPauseWSA(mVideoPlayerPtr);
    }

    private bool videoPlayerStop()
    {
        return VideoPlayerStopWSA(mVideoPlayerPtr);
    }

    private int videoPlayerUpdateVideoData()
    {
        return VideoPlayerUpdateVideoDataWSA(mVideoPlayerPtr);
    }

    private bool videoPlayerSeekTo(float position)
    {
        return VideoPlayerSeekToWSA(mVideoPlayerPtr, position);
    }

    private float videoPlayerGetCurrentPosition()
    {
        return VideoPlayerGetCurrentPositionWSA(mVideoPlayerPtr);
    }

    private bool videoPlayerSetVolume(float value)
    {
        return VideoPlayerSetVolumeWSA(mVideoPlayerPtr, value);
    }

    private int videoPlayerGetCurrentBufferingPercentage()
    {
        return VideoPlayerGetCurrentBufferingPercentageWSA(mVideoPlayerPtr);
    }

    private void videoPlayerOnPause()
    {
        VideoPlayerOnPauseWSA(mVideoPlayerPtr);
    }

#endif

#else // !UNITY_EDITOR

    void videoPlayerSetActivity() { }

    bool videoPlayerIsFileInAssetsFolder(string filename) { return false; }

    bool videoPlayerInit(VuforiaRenderer.RendererAPI rendererAPI) { return false; }

    bool videoPlayerDeinit() { return false; }

    bool videoPlayerLoad(string filename, int requestType, bool playOnTextureImmediately, float seekPosition) { return false; }

    bool videoPlayerUnload() { return false; }

    bool videoPlayerIsPlayableOnTexture() { return false; }

    bool videoPlayerIsPlayableFullscreen() { return false; }

    bool videoPlayerSetVideoTexturePtr(IntPtr texturePtr) { return false; }

    int videoPlayerGetStatus() { return 0; }

    int videoPlayerGetVideoWidth() { return 0; }

    int videoPlayerGetVideoHeight() { return 0; }

    float videoPlayerGetLength() { return 0; }

    bool videoPlayerPlay(bool fullScreen, float seekPosition) { return false; }

    bool videoPlayerPause() { return false; }

    bool videoPlayerStop() { return false; }

    int videoPlayerUpdateVideoData() { return 0; }

    bool videoPlayerSeekTo(float position) { return false; }

    float videoPlayerGetCurrentPosition() { return 0; }

    bool videoPlayerSetVolume(float value) { return false; }

    int videoPlayerGetCurrentBufferingPercentage() { return 0; }

    void videoPlayerOnPause() { }

#endif // !UNITY_EDITOR

#endregion // NATIVE_FUNCTIONS
}
