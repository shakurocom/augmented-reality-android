/*==============================================================================
Copyright (c) 2012-2014 QUALCOMM Austria Research Center GmbH.
All Rights Reserved.
 
This  Vuforia(TM) sample application in source code form ("Sample Code") for the
Vuforia Software Development Kit and/or Vuforia Extension for Unity
(collectively, the "Vuforia SDK") may in all cases only be used in conjunction
with use of the Vuforia SDK, and is subject in all respects to all of the terms
and conditions of the Vuforia SDK License Agreement, which may be found at
<a href="https://developer.vuforia.com/legal/license">https://developer.vuforia.com/legal/license</a>.
 
By retaining or using the Sample Code in any manner, you confirm your agreement
to all the terms and conditions of the Vuforia SDK License Agreement.  If you do
not agree to all the terms and conditions of the Vuforia SDK License Agreement,
then you may not retain or use any of the Sample Code in any manner.
==============================================================================*/

using UnityEngine;
using System.Collections;

/// <summary>
/// This class contains the logic to handle taps on VideoPlaybackBehaviour game objects
/// and starts playing the according video. It also pauses other videos when a new one is
/// started.
/// </summary>
public class VideoPlaybackController : MonoBehaviour
{
    #region PRIVATE_MEMBER_VARIABLES

    private Vector2 mTouchStartPos;
    private bool mTouchMoved = false;
    private float mTimeElapsed = 0.0f;

    private bool mTapped = false;
    private float mTimeElapsedSinceTap = 0.0f;

    private bool mWentToFullScreen = false;

    #endregion // PRIVATE_MEMBER_VARIABLES



    #region UNITY_MONOBEHAVIOUR_METHODS

    void Update()
    {
        // Determine the number of taps
        // Note: Input.tapCount doesn't work on Android

        if (Input.touchCount > 0)
        {
            Touch touch = Input.touches[0];
            if (touch.phase == TouchPhase.Began)
            {
                mTouchStartPos = touch.position;
                mTouchMoved = false;
                mTimeElapsed = 0.0f;
            }
            else
            {
                mTimeElapsed += Time.deltaTime;
            }

            if (touch.phase == TouchPhase.Moved)
            {
                if (Vector2.Distance(mTouchStartPos, touch.position) > 40)
                {
                    // Touch moved too far
                    mTouchMoved = true;
                }
            }
            else if (touch.phase == TouchPhase.Ended)
            {
                if (!mTouchMoved && mTimeElapsed < 1.0)
                {
                    if (mTapped)
                    {
                        // Second tap
                        HandleDoubleTap();
                        mTapped = false;
                    }
                    else
                    {
                        // Wait to see if this is a double tap
                        mTapped = true;
                        mTimeElapsedSinceTap = 0.0f;
                    }
                }
            }
        }

        if (mTapped)
        {
            if (mTimeElapsedSinceTap >= 0.5f)
            {
                // Not a double tap
                HandleTap();
                mTapped = false;
            }
            else
            {
                mTimeElapsedSinceTap += Time.deltaTime;
            }
        }

        // special handling in play mode:
        /*if (VuforiaRuntimeUtilities.IsPlayMode())
        {
            if (Input.GetMouseButtonUp(0))
            {
                if (PickVideo(Input.mousePosition) != null)
                    Debug.LogWarning("Playing videos is currently not supported in Play Mode.");
            }
        }*/
    }

    #endregion // UNITY_MONOBEHAVIOUR_METHODS



    #region PRIVATE_METHODS

    /// <summary>
    /// Handle single tap event
    /// </summary>
    private void HandleTap()
    {
        // Find out which video was tapped, if any
        VideoPlaybackBehaviour video = PickVideo(mTouchStartPos);

        if (video != null)
        {
            if (video.VideoPlayer.IsPlayableOnTexture())
            {
                // This video is playable on a texture, toggle playing/paused

                VideoPlayerHelper.MediaState state = video.VideoPlayer.GetStatus();
                if (state == VideoPlayerHelper.MediaState.PAUSED ||
                    state == VideoPlayerHelper.MediaState.READY ||
                    state == VideoPlayerHelper.MediaState.STOPPED)
                {
                    // Pause other videos before playing this one
                    PauseOtherVideos(video);

                    // Play this video on texture where it left off
                    video.VideoPlayer.Play(false, video.VideoPlayer.GetCurrentPosition());
                }
                else if (state == VideoPlayerHelper.MediaState.REACHED_END)
                {
                    // Pause other videos before playing this one
                    PauseOtherVideos(video);

                    // Play this video from the beginning
                    video.VideoPlayer.Play(false, 0);
                }
                else if (state == VideoPlayerHelper.MediaState.PLAYING)
                {
                    // Video is already playing, pause it
                    video.VideoPlayer.Pause();
                }
            }
            else
            {
                // Display the busy icon
                video.ShowBusyIcon();

                // This video cannot be played on a texture, play it full screen
                video.VideoPlayer.Play(true, 0);
                mWentToFullScreen = true;
            }
        }
    }


    /// <summary>
    /// Handle double tap event
    /// </summary>
    private void HandleDoubleTap()
    {
        // Find out which video was tapped, if any
        VideoPlaybackBehaviour video = PickVideo(mTouchStartPos);

        if (video != null)
        {
            if (video.VideoPlayer.IsPlayableFullscreen())
            {
                // Pause the video if it is currently playing
                video.VideoPlayer.Pause();

                // Seek the video to the beginning();
                video.VideoPlayer.SeekTo(0.0f);

                // Display the busy icon
                video.ShowBusyIcon();

                // Play the video full screen
                video.VideoPlayer.Play(true, 0);
                mWentToFullScreen = true;
            }
        }
    }


    /// <summary>
    /// Find the video object under the screen point
    /// </summary>
    private VideoPlaybackBehaviour PickVideo(Vector3 screenPoint)
    {
        VideoPlaybackBehaviour[] videos = (VideoPlaybackBehaviour[])
                FindObjectsOfType(typeof(VideoPlaybackBehaviour));

        Ray ray = Camera.main.ScreenPointToRay(screenPoint);
        RaycastHit hit = new RaycastHit();

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
        VideoPlaybackBehaviour[] videos = (VideoPlaybackBehaviour[])
                FindObjectsOfType(typeof(VideoPlaybackBehaviour));

        foreach (VideoPlaybackBehaviour video in videos)
        {
            if (video != currentVideo)
            {
                if (video.CurrentState == VideoPlayerHelper.MediaState.PLAYING)
                {
                    video.VideoPlayer.Pause();
                }
            }
        }
    }

    #endregion // PRIVATE_METHODS



    #region PUBLIC_METHODS

    /// <summary>
    /// One-time check for the Instructional Screen
    /// </summary>
    public bool CheckWentToFullScreen()
    {
        bool result = mWentToFullScreen;
        mWentToFullScreen = false;
        return result;
    }

    #endregion // PUBLIC_METHODS
}