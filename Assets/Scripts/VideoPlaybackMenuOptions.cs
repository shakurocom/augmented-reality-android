/*===============================================================================
Copyright (c) 2015-2016 PTC Inc. All Rights Reserved.
 
Copyright (c) 2012-2015 Qualcomm Connected Experiences, Inc. All Rights Reserved.
 
Vuforia is a trademark of PTC Inc., registered in the United States and other 
countries.
===============================================================================*/
using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Vuforia;

/// <summary>
/// Class to handle Video Playback menu options.
/// </summary>
public class VideoPlaybackMenuOptions : MenuOptions
{
    #region PRIVATE_MEMBERS
    private PlayVideo mPlayVideo;
    #endregion PRIVATE_MEMBERS


    #region MONOBEHAVIOUR_METHODS
    protected override void Start()
    {
        base.Start();
        mPlayVideo = FindObjectOfType<PlayVideo>();
    }
    #endregion //MONOBEHAVIOUR_METHODS


    #region PUBLIC_METHODS
    /// <summary>
    /// If the video is already playing on texture, enabling it will bring the video to full-screen
    /// otherwise, the video will play on fullscreen the next time user taps on the texture.
    /// </summary>
    /// <param name="tf"></param>
    public void ToggleFullscreenMode()
    {
        Toggle fullscreenToggle = base.FindUISelectableWithText<Toggle>("Fullscreen");
        mPlayVideo.playFullscreen = fullscreenToggle ? fullscreenToggle.isOn : false;
        if (mPlayVideo.playFullscreen)
        {
            VideoPlaybackBehaviour video = PickVideo();
            if (video != null)
            {
                if (video.VideoPlayer.IsPlayableFullscreen())
                {
                    //On Android, we use Unity's built in player, so Unity application pauses before going to fullscreen. 
                    //So we have to handle the orientation from within Unity. 
#if UNITY_ANDROID
					Screen.orientation = ScreenOrientation.LandscapeLeft;
#endif
                    // Pause the video if it is currently playing
                    video.VideoPlayer.Pause();

                    // Seek the video to the beginning();
                    video.VideoPlayer.SeekTo(0.0f);

                    // Display the busy icon
                    video.ShowBusyIcon();

                    // Play the video full screen
                    StartCoroutine ( PlayVideo.PlayFullscreenVideoAtEndOfFrame(video) );
                }
            }
        }

        CloseMenu();
    }
    #endregion //PUBLIC_METHODS


    #region PRIVATE_METHODS
    private VideoPlaybackBehaviour PickVideo()
    {
        VideoPlaybackBehaviour[] behaviours = GameObject.FindObjectsOfType<VideoPlaybackBehaviour>();
        foreach (VideoPlaybackBehaviour vb in behaviours)
        {
            if (vb.CurrentState == VideoPlayerHelper.MediaState.PLAYING)
                return vb;
        }
        return null;
    }
    #endregion //PRIVATE_METHODS
}
