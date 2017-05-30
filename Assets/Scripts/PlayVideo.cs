/*============================================================================== 
Copyright (c) 2016 PTC Inc. All Rights Reserved.

 * Copyright (c) 2012-2015 Qualcomm Connected Experiences, Inc. All Rights Reserved. 
 * ==============================================================================*/

using UnityEngine;
using System.Collections;
using Vuforia;

/// <summary>
/// Demonstrates how to play the video on texture and full-screen mode.
/// Single tapping on texture will play the video on texture (if the 'Play FullScreen' Mode in the UIMenu is turned off)
/// or play full screen (if the option is enabled in the UIMenu)
/// At any time during the video playback, it can be brought to full-screen by enabling the options from the UIMenu.
/// </summary>
public class PlayVideo : MonoBehaviour
{
    #region PUBLIC_METHODS
    public bool playFullscreen = false;
    #endregion //PUBLIC_METHODS


    #region PRIVATE_MEMBERS
    private VideoPlaybackBehaviour currentVideo = null;
    #endregion //PRIVATE_MEMBERS


    #region PUBLIC_METHODS
    /// <summary>
    /// Try to pick video at tap screen position 
    /// </summary>
    public void TryPickingVideo()
    {
        if (VuforiaRuntimeUtilities.IsPlayMode())
        {
            if (PickVideo(Input.mousePosition) != null)
                Debug.LogWarning("Playing videos is currently not supported in Play Mode.");
        }

        // Find out which video was tapped, if any
        currentVideo = PickVideo(Input.mousePosition);

        if (currentVideo != null)
        {
            if (playFullscreen)
            {
                if (currentVideo.VideoPlayer.IsPlayableFullscreen())
                {
                    // Pause the video if it is currently playing
                    currentVideo.VideoPlayer.Pause();

                    // Seek the video to the beginning();
                    currentVideo.VideoPlayer.SeekTo(0.0f);

                    // Display the busy icon
                    currentVideo.ShowBusyIcon();

                    // Play the video full screen
                    StartCoroutine( PlayFullscreenVideoAtEndOfFrame(currentVideo) );
                }
            }
            else
            {
                if (currentVideo.VideoPlayer.IsPlayableOnTexture())
                {
                    // This video is playable on a texture, toggle playing/paused
                    VideoPlayerHelper.MediaState state = currentVideo.VideoPlayer.GetStatus();
                    if (state == VideoPlayerHelper.MediaState.PAUSED ||
                        state == VideoPlayerHelper.MediaState.READY ||
                        state == VideoPlayerHelper.MediaState.STOPPED)
                    {
                        // Pause other videos before playing this one
                        PauseOtherVideos(currentVideo);

                        // Play this video on texture where it left off
                        currentVideo.VideoPlayer.Play(false, currentVideo.VideoPlayer.GetCurrentPosition());
                    }
                    else if (state == VideoPlayerHelper.MediaState.REACHED_END)
                    {
                        // Pause other videos before playing this one
                        PauseOtherVideos(currentVideo);

                        // Play this video from the beginning
                        currentVideo.VideoPlayer.Play(false, 0);
                    }
                    else if (state == VideoPlayerHelper.MediaState.PLAYING)
                    {
                        // Video is already playing, pause it
                        currentVideo.VideoPlayer.Pause();
                    }
                }
                else
                {
                    // Display the busy icon
                    currentVideo.ShowBusyIcon();

                    // This video cannot be played on a texture, play it full screen
                    StartCoroutine( PlayFullscreenVideoAtEndOfFrame(currentVideo) );
                }
            }
        }
    }
    
    public static IEnumerator PlayFullscreenVideoAtEndOfFrame(VideoPlaybackBehaviour video)
    {
#if !UNITY_EDITOR
    #if UNITY_ANDROID
        Screen.orientation = ScreenOrientation.LandscapeLeft;
    #else // iOS or UWP
        Screen.orientation = ScreenOrientation.AutoRotation;
        Screen.autorotateToPortrait = true;
        Screen.autorotateToPortraitUpsideDown = true;
        Screen.autorotateToLandscapeLeft = true;
        Screen.autorotateToLandscapeRight = true;
    #endif
#endif //!UNITY_EDITOR

        yield return new WaitForEndOfFrame ();

        // we wait a bit to allow the ScreenOrientation.AutoRotation to become effective
        yield return new WaitForSeconds (0.3f);
        
        video.VideoPlayer.Play(true, 0);

        // We call WaitForEndOfFrame twice, so to ensure that
        // we intercept the time when the fullscreen video player stops.
        yield return new WaitForEndOfFrame();
        yield return new WaitForEndOfFrame();

        // When we reach this point, we know that the fullscreen player terminated.
        Debug.Log("Fullscreen playback exited.");

        // We restore the Play icon
        video.ShowPlayIcon();

#if !UNITY_EDITOR
        // We now restore the Portrait orientation
        // as the sample UI requires so.
        Screen.autorotateToPortrait = true;
        Screen.autorotateToLandscapeLeft = false;
        Screen.autorotateToLandscapeRight = false;
        Screen.autorotateToPortraitUpsideDown = false;

        // We need to act in 2 steps, i.e. first we change to landscape 
        // and then to Portrait; this ensures that Vuforia can acknowlegde an orientation change.
        // First we set it temporarily to landscape
        Screen.orientation = ScreenOrientation.LandscapeLeft;

        // We wait for about half a second to be sure the 
        // screen orientation has switched to landscape
        yield return new WaitForSeconds(0.7f);

        // Finally we set to Portrait
        Screen.orientation = ScreenOrientation.Portrait;
#endif //!UNITY_EDITOR
    }
    #endregion //PUBLIC_METHODS


    #region PRIVATE_METHODS
    /// <summary>
    /// Find the video object under the screen point
    /// </summary>
    private VideoPlaybackBehaviour PickVideo(Vector3 screenPoint)
    {
        GameObject go = VuforiaManager.Instance.ARCameraTransform.gameObject;
        Camera[] cam = go.GetComponentsInChildren<Camera> ();
        Ray ray = cam[0].ScreenPointToRay(screenPoint);

        RaycastHit hit = new RaycastHit();
        VideoPlaybackBehaviour[] videos = FindObjectsOfType<VideoPlaybackBehaviour>();
        foreach (VideoPlaybackBehaviour video in videos)
        {
            if (video.GetComponent<Collider>().Raycast(ray, out hit, 10000))
            {
                return video;
            }
        }
        return null;
    }

    /// <summary>
    /// Pause all videos except this one
    /// </summary>
    private void PauseOtherVideos(VideoPlaybackBehaviour currentVideo)
    {
        VideoPlaybackBehaviour[] videos = FindObjectsOfType<VideoPlaybackBehaviour>();
        foreach (VideoPlaybackBehaviour video in videos)
        {
            if (video != currentVideo && 
                video.CurrentState == VideoPlayerHelper.MediaState.PLAYING)
            {
                video.VideoPlayer.Pause();
            }
        }
    }
    #endregion // PRIVATE_METHODS
}
