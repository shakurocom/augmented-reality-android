/*===============================================================================
Copyright (c) 2016 PTC Inc. All Rights Reserved.

Copyright (c) 2012-2015 Qualcomm Connected Experiences, Inc. All Rights Reserved.
 
Confidential and Proprietary - Protected under copyright and other laws.

Vuforia is a trademark of PTC Inc., registered in the United States and other 
countries.   
===============================================================================*/


package com.vuforia.VuforiaMedia;

import java.io.IOException;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.util.concurrent.locks.ReentrantLock;

import android.app.Activity;
import android.content.res.AssetFileDescriptor;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnBufferingUpdateListener;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnErrorListener;
import android.media.MediaPlayer.OnPreparedListener;
import android.os.Build;
import android.view.Surface;

/** The main class for the VideoPlayer plugin. */
public class VideoPlayerHelper implements OnPreparedListener, OnBufferingUpdateListener, OnCompletionListener, OnErrorListener
{
    public static final float       CURRENT_POSITION            = -1;
    private MediaPlayer             mMediaPlayer                = null;
    private MEDIA_TYPE              mVideoType                  = MEDIA_TYPE.UNKNOWN;
    private Object                  mSurfaceTexture             = null;
    private int                     mCurrentBufferingPercentage = 0;
    private String                  mMovieName                  = "";
    private int                     mTextureID                  = 0;
    //Intent                          mPlayerHelperActivityIntent = null;
    private Activity                mParentActivity             = null;
    private MEDIA_STATE             mCurrentState               = MEDIA_STATE.NOT_READY;
    private boolean                 mShouldPlayImmediately      = false;
    private float                   mSeekPosition               = CURRENT_POSITION;
    private ReentrantLock           mMediaPlayerLock            = null;
    private ReentrantLock           mSurfaceTextureLock         = null;

    private int mMediaTextureID                                 = -1;
    private int mDestTextureID                                  = -1;
    private int mFBO                                            = -1;


    private static Constructor<?> _surfaceTextureConstructor;
    private static Constructor<?> _surfaceConstructor;

    private static Method _updateTexImageFunc;
    private static Method _getTransformMatrixFunc;
    private static Method _releaseFunc;

    private final String CLASSNAME_SURFACETEXTURE               = "android.graphics.SurfaceTexture";
    private final String CLASSNAME_SURFACE                      = "android.view.Surface";


    // This enum declares the possible states a media can have
    public enum MEDIA_STATE {
        REACHED_END     (0),
        PAUSED          (1),
        STOPPED         (2),
        PLAYING         (3),
        READY           (4),
        NOT_READY       (5),
        ERROR           (6);

        private int type;
        MEDIA_STATE (int i)
        {
            this.type = i;
        }
        public int getNumericType()
        {
            return type;
        }
    }

    // This enum declares what type of playback we can do, share with the team
    public enum MEDIA_TYPE {
        ON_TEXTURE              (0),
        FULLSCREEN              (1),
        ON_TEXTURE_FULLSCREEN   (2),
        UNKNOWN                 (3);

        private int type;
        MEDIA_TYPE (int i)
        {
            this.type = i;
        }
        public int getNumericType()
        {
            return type;
        }
    }


    // Native methods
    public native void initNative(int openGLVersion);
    public native int initMediaTexture();
    public native void bindMediaTexture(int mediaTextureID);
    public native int initFBO(int destTextureID, int videoWidth, int videoHeight);
    public native void copyTexture(int mediaTextureID, int destTextureID, int fbo,
            float[] textureMat, int videoWidth, int videoHeight);


    /** Static initializer block to load native libraries on start-up. */
    static
    {
        loadLibrary("VuforiaMedia");
    }

    /** A helper for loading native libraries stored in "libs/armeabi*". */
    public static boolean loadLibrary(String nLibName)
    {
        try
        {
            System.loadLibrary(nLibName);
            DebugLog.LOGI("Native library lib" + nLibName + ".so loaded");
            return true;
        }
        catch (UnsatisfiedLinkError ulee)
        {
            DebugLog.LOGE("The library lib" + nLibName +
                            ".so could not be loaded");
        }
        catch (SecurityException se)
        {
            DebugLog.LOGE("The library lib" + nLibName +
                            ".so was not allowed to be loaded");
        }

        return false;
    }

    /** Initializes the VideoPlayerHelper object. */
    public boolean init(int openGLVersion)
    {
        mMediaPlayerLock = new ReentrantLock();
        mSurfaceTextureLock = new ReentrantLock();
        initNative(openGLVersion);
        
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH)
        {
            try
            {
                Class<?> surfaceTextureClass = Class.forName(CLASSNAME_SURFACETEXTURE);
                Class<?> surfaceClass = Class.forName(CLASSNAME_SURFACE);

                // Retrieve constructor for SurfaceTexture
                _surfaceTextureConstructor = surfaceTextureClass.getConstructor(Integer.TYPE);
                if (_surfaceTextureConstructor == null)
                {
                    DebugLog.LOGE("Couldn't find SurfaceTexture(int) constructor");
                    return false;
                }

                // Retrieve constructor for Surface(SurfaceTexture)
                _surfaceConstructor = surfaceClass.getConstructor(surfaceTextureClass);
                if (_surfaceConstructor == null)
                {
                    DebugLog.LOGE("Couldn't find Surface(SurfaceTexture) constructor");
                    return false;
                }

                // Retrieve SurfaceTexture.updateTexImage function
                _updateTexImageFunc = retrieveClassMethod(surfaceTextureClass, "updateTexImage");
                if (_updateTexImageFunc == null)
                {
                    DebugLog.LOGE("Couldn't find SurfaceTexture.updateTexImage() method");
                    return false;
                }

                // Retrieve SurfaceTexture.getTransformMatrix function
                _getTransformMatrixFunc = retrieveClassMethod(surfaceTextureClass, "getTransformMatrix", float[].class);
                if (_getTransformMatrixFunc == null)
                {
                    DebugLog.LOGE("Couldn't find SurfaceTexture.getTransformMatrix(float[]) method");
                    return false;
                }

                // Retrieve SurfaceTexture.release function
                _releaseFunc = retrieveClassMethod(surfaceTextureClass, "release");
                if (_releaseFunc == null)
                {
                    DebugLog.LOGE("Couldn't find SurfaceTexture.release() method");
                    return false;
                }
            }
            catch (Exception e)
            {
                DebugLog.LOGE("Exception in init: " + e.getMessage());
                return false;
            }
        }

        return true;
    }

    /** Deinitializes the VideoPlayerHelper object. */
    public boolean deinit()
    {
        unload();

        mSurfaceTextureLock.lock();
        if (mSurfaceTexture != null)
        {
            try
            {
                _releaseFunc.invoke(mSurfaceTexture);
            }
            catch (Exception e)
            {
                DebugLog.LOGE("Exception in deinit: " + e.getMessage());
            }
            mSurfaceTexture = null;
        }
        mSurfaceTextureLock.unlock();

        return true;
    }

    /** Loads a local or remote movie file */
    public boolean load(String filename, int type, boolean playOnTextureImmediately, float seekPosition)
    {
        // If the client requests that we should be able to play ON_TEXTURE then we need
        // to create a MediaPlayer

        MEDIA_TYPE requestedType = MEDIA_TYPE.values()[type];

        boolean canBeOnTexture = false;
        boolean canBeFullscreen = false;

        boolean result = false;
        mMediaPlayerLock.lock();
        mSurfaceTextureLock.lock();

            // If the media has already been loaded then exit.
            // The client must first call unload() before calling load again
            if ((mCurrentState == MEDIA_STATE.READY) || (mMediaPlayer != null))
            {
                DebugLog.LOGD("Already loaded");
            }
            else
            {
                if (((requestedType == MEDIA_TYPE.ON_TEXTURE) ||                        // If the client requests on texture only
                    (requestedType == MEDIA_TYPE.ON_TEXTURE_FULLSCREEN)) &&             // or on texture with full screen
                    (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH))  // and this is an ICS device
                {
                    // Create a GL_TEXTURE_EXTERNAL_OES texture for use with the surface texture
                    mMediaTextureID = initMediaTexture();
        
                    if (!setupSurfaceTexture(mMediaTextureID))
                    {
                        DebugLog.LOGD("Can't load file to ON_TEXTURE because the Surface Texture is not ready");
                    }
                    else
                    {
                        // Get the asset file descriptor, if it exists
                        AssetFileDescriptor afd = null;
                        try
                        {
                            afd = mParentActivity.getAssets().openFd(filename);
                        }
                        catch(IOException e)
                        {
                            afd = null;
                        }

                        try
                        {
                            mMediaPlayer = new MediaPlayer();
            
                            if (afd != null)
                            {
                                mMediaPlayer.setDataSource(afd.getFileDescriptor(),afd.getStartOffset(),afd.getLength());
                                afd.close();
                            }
                            else
                            {
                                mMediaPlayer.setDataSource(filename);
                            }

                            Object argList[] = new Object[1];
                            argList[0] = mSurfaceTexture;
                            Surface surface = (Surface) _surfaceConstructor.newInstance(argList);

                            mMediaPlayer.setOnPreparedListener(this);
                            mMediaPlayer.setOnBufferingUpdateListener(this);
                            mMediaPlayer.setOnCompletionListener(this);
                            mMediaPlayer.setOnErrorListener(this);
                            mMediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
                            mMediaPlayer.setSurface(surface);
                            canBeOnTexture = true;
                            mShouldPlayImmediately = playOnTextureImmediately;
                            mMediaPlayer.prepareAsync();
                        }
                        catch (Exception e)
                        {
                            DebugLog.LOGD("Could not create a Media Player");
                            mCurrentState = MEDIA_STATE.ERROR; 
                            mMediaPlayerLock.unlock();
                            mSurfaceTextureLock.unlock();
                            return false;
                        }
                    }
                }

                // If the client requests that we should be able to play FULLSCREEN
                // then we need to create a FullscreenPlaybackActivity
                if ((requestedType == MEDIA_TYPE.FULLSCREEN) || (requestedType == MEDIA_TYPE.ON_TEXTURE_FULLSCREEN))
                {
                    //mPlayerHelperActivityIntent = new Intent(mParentActivity, FullscreenPlayback.class);
                    //mPlayerHelperActivityIntent.setAction(android.content.Intent.ACTION_VIEW);
                    canBeFullscreen = true;
                }

                // We store the parameters for further use
                mMovieName = filename;
                mSeekPosition = seekPosition;

                if (canBeFullscreen && canBeOnTexture)  mVideoType = MEDIA_TYPE.ON_TEXTURE_FULLSCREEN;
                else if (canBeFullscreen) {             mVideoType = MEDIA_TYPE.FULLSCREEN; mCurrentState = MEDIA_STATE.READY; } // If it is pure fullscreen then we're ready otherwise we let the MediaPlayer load first
                else if (canBeOnTexture)                mVideoType = MEDIA_TYPE.ON_TEXTURE;
                else                                    mVideoType = MEDIA_TYPE.UNKNOWN;

                result = true;
            }

        mSurfaceTextureLock.unlock();
        mMediaPlayerLock.unlock();

        return result;
    }

    /** Unloads the currently loaded movie
        After this is called a new load() has to be invoked */
    public boolean unload()
    {
        mMediaPlayerLock.lock();
            if (mMediaPlayer != null) {
                try 
                {
                    mMediaPlayer.stop();
                } 
                catch (Exception e) 
                {
                    mMediaPlayerLock.unlock();
                    DebugLog.LOGE("Could not start playback");
                }
                        
                mMediaPlayer.release();
                mMediaPlayer = null;
            }
        mMediaPlayerLock.unlock();

        // TODO: unload native textures

        mCurrentState = MEDIA_STATE.NOT_READY;
        mVideoType = MEDIA_TYPE.UNKNOWN;
        return true;
    }

    /** Indicates whether the movie can be played on a texture */
    public boolean isPlayableOnTexture()
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH &&
                ((mVideoType == MEDIA_TYPE.ON_TEXTURE) || (mVideoType == MEDIA_TYPE.ON_TEXTURE_FULLSCREEN)))
            return true;

        return false;
    }

    /** Indicates whether the movie can be played fullscreen */
    public boolean isPlayableFullscreen()
    {
        if ((mVideoType == MEDIA_TYPE.FULLSCREEN) || (mVideoType == MEDIA_TYPE.ON_TEXTURE_FULLSCREEN))
            return true;

        return false;
    }

    /** Return the current status of the movie such as Playing, Paused or Not Ready */
    int getStatus()
    {
        return mCurrentState.type;
    }

    /** Returns the width of the video frame */
    public int getVideoWidth()
    {
        if (!isPlayableOnTexture())
        {
            // DebugLog.LOGD("Cannot get the video width if it is not playable on texture");
            return -1;
        }

        if ((mCurrentState == MEDIA_STATE.NOT_READY) || (mCurrentState == MEDIA_STATE.ERROR))
        {
            // DebugLog.LOGD("Cannot get the video width if it is not ready");
            return -1;
        }

        int result=-1;
        mMediaPlayerLock.lock();
            if (mMediaPlayer != null)
                result = mMediaPlayer.getVideoWidth();
        mMediaPlayerLock.unlock();

        return result;
    }

    /** Returns the height of the video frame */
    public int getVideoHeight()
    {
        if (!isPlayableOnTexture())
        {
            // DebugLog.LOGD("Cannot get the video height if it is not playable on texture");
            return -1;
        }

        if ((mCurrentState == MEDIA_STATE.NOT_READY) || (mCurrentState == MEDIA_STATE.ERROR))
        {
            // DebugLog.LOGD("Cannot get the video height if it is not ready");
            return -1;
        }

        int result=-1;
        mMediaPlayerLock.lock();
            if (mMediaPlayer != null)
                result = mMediaPlayer.getVideoHeight();
        mMediaPlayerLock.unlock();

        return result;
    }

    /** Returns the length of the current movie */
    public float getLength()
    {
        if (!isPlayableOnTexture())
        {
            // DebugLog.LOGD("Cannot get the video length if it is not playable on texture");
            return -1;
        }

        if ((mCurrentState == MEDIA_STATE.NOT_READY) || (mCurrentState == MEDIA_STATE.ERROR))
        {
            // DebugLog.LOGD("Cannot get the video length if it is not ready");
            return -1;
        }


        int result=-1;
        mMediaPlayerLock.lock();
            if (mMediaPlayer != null)
                result = mMediaPlayer.getDuration()/1000;
        mMediaPlayerLock.unlock();

        return result;
    }


    /** Request a movie to be played either full screen or on texture and at a given position */
    public boolean play(boolean fullScreen, float seekPosition)
    {
        if (fullScreen)
        {
            // If the play request was for fullscreen playback
            // We check first whether this was requested upon load time
            if (!isPlayableFullscreen())
            {
                DebugLog.LOGD("Cannot play this video fullscreen, it was not requested on load");
                return false;
            }

            // Unity handles full screen playback
            return true;
        }
        else
        {
            // If the client requested playback on texture
            // we must first verify that it is possible
            if (!isPlayableOnTexture())
            {
                DebugLog.LOGD("Cannot play this video on texture, it was either not requested on load or is not supported on this plattform");
                return false;
            }

            if ((mCurrentState == MEDIA_STATE.NOT_READY) || (mCurrentState == MEDIA_STATE.ERROR))
            {
                DebugLog.LOGD("Cannot play this video if it is not ready");
                return false;
            }

            mMediaPlayerLock.lock();
                // If the client requests a given position
                if (seekPosition != CURRENT_POSITION)
                {
                    try 
                    {
                        mMediaPlayer.seekTo((int)seekPosition*1000);
                    } 
                    catch (Exception e) 
                    {
                        mMediaPlayerLock.unlock();
                        DebugLog.LOGE("Could not seek to position");
                    }
                }
                else    // If it had reached the end loop it back
                    if (mCurrentState == MEDIA_STATE.REACHED_END)
                    {
                        try 
                        {
                            mMediaPlayer.seekTo(0);
                        } 
                        catch (Exception e) 
                        {
                            mMediaPlayerLock.unlock();
                            DebugLog.LOGE("Could not seek to position");
                        }
                    }

                // Then simply start playing
                try 
                {
                    mMediaPlayer.start();
                } 
                catch (Exception e) 
                {
                    mMediaPlayerLock.unlock();
                    DebugLog.LOGE("Could not start playback");
                }
                mCurrentState = MEDIA_STATE.PLAYING;

            mMediaPlayerLock.unlock();

            return true;
        }
    }

    /** Pauses the current movie being played */
    public boolean pause()
    {
        if (!isPlayableOnTexture())
        {
            // DebugLog.LOGD("Cannot pause this video since it is not on texture");
            return false;
        }

        if ((mCurrentState == MEDIA_STATE.NOT_READY) || (mCurrentState == MEDIA_STATE.ERROR))
        {
            // DebugLog.LOGD("Cannot pause this video if it is not ready");
            return false;
        }

        boolean result = false;

        mMediaPlayerLock.lock();
            if (mMediaPlayer != null)
            {
                if (mMediaPlayer.isPlaying())
                {
                    try 
                    {
                        mMediaPlayer.pause();
                    } 
                    catch (Exception e) 
                    {
                        mMediaPlayerLock.unlock();
                        DebugLog.LOGE("Could not pause playback");
                    }
                    mCurrentState = MEDIA_STATE.PAUSED;
                    result = true;
                }
            }
        mMediaPlayerLock.unlock();

        return result;
    }

    /** Stops the current movie being played */
    public boolean stop()
    {
        if (!isPlayableOnTexture())
        {
            // DebugLog.LOGD("Cannot stop this video since it is not on texture");
            return false;
        }

        if ((mCurrentState == MEDIA_STATE.NOT_READY) || (mCurrentState == MEDIA_STATE.ERROR))
        {
            // DebugLog.LOGD("Cannot stop this video if it is not ready");
            return false;
        }

        boolean result = false;

        mMediaPlayerLock.lock();
            if (mMediaPlayer != null)
            {
                mCurrentState = MEDIA_STATE.STOPPED;
                try 
                {
                    mMediaPlayer.stop();
                } 
                catch (Exception e) 
                {
                    mMediaPlayerLock.unlock();
                    DebugLog.LOGE("Could not stop playback");
                }
                
                result = true;
            }
        mMediaPlayerLock.unlock();

        return result;
    }

    /** Tells the VideoPlayerHelper to update the data from the video feed */
    public int updateVideoData()
    {
        if (!isPlayableOnTexture())
        {
            // DebugLog.LOGD("Cannot update the data of this video since it is not on texture");
            return MEDIA_STATE.NOT_READY.type;
        }

        int result = MEDIA_STATE.NOT_READY.type;

        mSurfaceTextureLock.lock();
            if (mSurfaceTexture != null)
            {
                // Only request an update if currently playing
                if (mCurrentState == MEDIA_STATE.PLAYING)
                {
                    try
                    {
                        _updateTexImageFunc.invoke(mSurfaceTexture);

                        float[] mtx = new float[16];

                        Object argList[] = new Object[1];
                        argList[0] = mtx;
                        _getTransformMatrixFunc.invoke(mSurfaceTexture, argList);

                        // Copy texture from GL_TEXTURE_EXTERNAL_OES to GL_TEXTURE_2D object
                        copyTexture(mMediaTextureID, mDestTextureID, mFBO, mtx,
                                mMediaPlayer.getVideoWidth(), mMediaPlayer.getVideoHeight());
                    }
                    catch (Exception e)
                    {
                        DebugLog.LOGE("Error in updateVideoData: " + e.getMessage());
                        result = MEDIA_STATE.ERROR.type;
                    }
                }

                result = mCurrentState.type;
            }
        mSurfaceTextureLock.unlock();

        return result;
    }

    /** Moves the movie to the requested seek position */
    public boolean seekTo(float position)
    {
        if (!isPlayableOnTexture())
        {
            // DebugLog.LOGD("Cannot seek-to on this video since it is not on texture");
            return false;
        }

        if ((mCurrentState == MEDIA_STATE.NOT_READY) || (mCurrentState == MEDIA_STATE.ERROR))
        {
            // DebugLog.LOGD("Cannot seek-to on this video if it is not ready");
            return false;
        }

        boolean result = false;
        mMediaPlayerLock.lock();
            if (mMediaPlayer != null)
            {
                try 
                {
                    mMediaPlayer.seekTo((int)position*1000);
                } 
                catch (Exception e) 
                {
                    mMediaPlayerLock.unlock();
                    DebugLog.LOGE("Could not seek to position");
                }
                result = true;
            }
        mMediaPlayerLock.unlock();

        return result;
    }

    /** Gets the current seek position */
    public float getCurrentPosition()
    {
        if (!isPlayableOnTexture())
        {
            // DebugLog.LOGD("Cannot get the current playback position of this video since it is not on texture");
            return -1;
        }

        if ((mCurrentState == MEDIA_STATE.NOT_READY) || (mCurrentState == MEDIA_STATE.ERROR))
        {
            // DebugLog.LOGD("Cannot get the current playback position of this video if it is not ready");
            return -1;
        }

        float result = -1;
        mMediaPlayerLock.lock();
            if (mMediaPlayer != null)
                result = mMediaPlayer.getCurrentPosition()/1000.0f;
        mMediaPlayerLock.unlock();

        return result;
    }

    /** Sets the volume of the movie to the desired value */
    public boolean setVolume(float value)
    {
        if (!isPlayableOnTexture())
        {
            // DebugLog.LOGD("Cannot set the volume of this video since it is not on texture");
            return false;
        }

        if ((mCurrentState == MEDIA_STATE.NOT_READY) || (mCurrentState == MEDIA_STATE.ERROR))
        {
            // DebugLog.LOGD("Cannot set the volume of this video if it is not ready");
            return false;
        }

        boolean result = false;
        mMediaPlayerLock.lock();
            if (mMediaPlayer != null)
            {
                mMediaPlayer.setVolume(value, value);
                result = true;
            }
        mMediaPlayerLock.unlock();

        return result;
    }

    /**
     *  The following functions are specific to Android
     *  and will likely not be implemented on other platforms
     */

    /** Gets the buffering percentage in case the movie is loaded from network */
    public int getCurrentBufferingPercentage()
    {
        return mCurrentBufferingPercentage;
    }

    /** Listener call for buffering */
    public void onBufferingUpdate(MediaPlayer arg0, int arg1)
    {
        mMediaPlayerLock.lock();
            if (mMediaPlayer != null)
            {
                if (arg0 == mMediaPlayer)
                    mCurrentBufferingPercentage = arg1;
            }
        mMediaPlayerLock.unlock();
    }

    /** With this we can set the parent activity */
    public void setActivity(Activity newActivity)
    {
        mParentActivity = newActivity;
    }

    /** To set a value upon completion */
    public void onCompletion(MediaPlayer arg0)
    {
        // Signal that the video finished playing
        mCurrentState = MEDIA_STATE.REACHED_END;
    }

    /** Used to set up the surface texture */
    public boolean setupSurfaceTexture(int nativeTextureID)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH)
        {
            // We create a surface texture where the video can be played
            // We have to give it a texture id of an already created (in native) // OpenGL texture
            //mSurfaceTextureLock.lock();  already called within a lock block

                Object argList[] = new Object[1];
                argList[0] = new Integer(nativeTextureID);

                try
                {
                    mSurfaceTexture = _surfaceTextureConstructor.newInstance(argList);
                }
                catch (Exception e)
                {
                    DebugLog.LOGE("Error in setupSurfaceTexture: " + e.getMessage());
                    return false;
                }

                mTextureID = nativeTextureID;

            //mSurfaceTextureLock.unlock();

            return true;
        }
        else
            return false;
    }
    
    /** This is called when the movie is ready for playback */
    public void onPrepared(MediaPlayer mediaplayer) 
    {
        mCurrentState = MEDIA_STATE.READY;

        // If requested an immediate play
        if (mShouldPlayImmediately)
            play(false, mSeekPosition);

        mSeekPosition = 0;
    }

    /** This is called if an error occurs while loading the video */
    public boolean onError(MediaPlayer mp, int what, int extra)
    {
        DebugLog.LOGE("Error while opening the file. Unloading the media player");
        unload();
        mCurrentState = MEDIA_STATE.ERROR;
        return true;
    }

    /** Set the GL_TEXTURE_2D object that the video frames will be copied to */
    public boolean setVideoTextureID(int textureID)
    {
        if (!isPlayableOnTexture() || mMediaPlayer == null)
        {
            DebugLog.LOGD("Cannot set the video texture ID if it is not playable on texture");
            return false;
        }

        mDestTextureID = textureID;

        int videoWidth = mMediaPlayer.getVideoWidth();
        int videoHeight = mMediaPlayer.getVideoHeight();

        if (videoWidth > 0 && videoHeight > 0)
        {
            mFBO = initFBO(mDestTextureID, videoWidth, videoHeight);
            return true;
        }

        return false;
    }
    
    /** Returns true if the file is located in the assets folder */
    public boolean isFileInAssetsFolder(String filename)
    {
        AssetFileDescriptor afd = null;

        try
        {
            afd = mParentActivity.getAssets().openFd(filename);
        }
        catch(IOException e)
        {
            afd = null;
        }

        return afd != null;
    }
    
    /** Attempt to retrieve a method from a given class */
    public static Method retrieveClassMethod(Class<?> cls, String name, Class<?>... parameterTypes)
    {
        Method classMethod = null;
        try
        {
            classMethod = cls.getMethod(name, parameterTypes);
        }
        catch (Exception e)
        {
            DebugLog.LOGE("Failed to retrieve method '" + name + "' at API level " + Build.VERSION.SDK_INT + ": " + e.toString());
        }

        return classMethod;
    }
}
