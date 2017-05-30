/*===============================================================================
Copyright (c) 2015 PTC Inc. All Rights Reserved.
 
Copyright (c) 2012-2015 Qualcomm Connected Experiences, Inc. All Rights Reserved.
 
Vuforia is a trademark of PTC Inc., registered in the United States and other 
countries.
==============================================================================*/

#import <AVFoundation/AVFoundation.h>
#import <MediaPlayer/MediaPlayer.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <Metal/Metal.h>


// Media types
typedef enum tagMEDIA_TYPE { 
    ON_TEXTURE,
    FULLSCREEN,
    ON_TEXTURE_FULLSCREEN
} MEDIA_TYPE;


// Media states
typedef enum tagMEDIA_STATE {
    REACHED_END         = 0,
    PAUSED              = 1,
    STOPPED             = 2,
    PLAYING             = 3,
    READY               = 4,
    NOT_READY           = 5,
    ERROR               = 6,
    PLAYING_FULLSCREEN  = 7
} MEDIA_STATE;


// Used to specify that playback should start from the current position when
// calling the load and play methods
static const float VIDEO_PLAYBACK_CURRENT_POSITION = -1.0f;


@interface VideoPlayerHelper : NSObject {
@private
    BOOL useMetal;
    NSBundle *metalBundle;
    
    // AVPlayer
    AVPlayer* player;
    CMTime playerCursorStartPosition;
    
    // Native playback
    MPMoviePlayerController* moviePlayer;
    BOOL resumeOnTexturePlayback;
    
    // Timing
    CFTimeInterval mediaStartTime;
    CFTimeInterval playerCursorPosition;
    NSTimer* frameTimer;
    BOOL stopFrameTimer;
    
    // Asset
    NSURL* mediaURL;
    AVAssetReader* assetReader;
    AVAssetReaderTrackOutput* assetReaderTrackOutputVideo;
    AVURLAsset* asset;
    BOOL seekRequested;
    float requestedCursorPosition;
    BOOL localFile;
    BOOL playImmediately;
    
    // Requested video type
    MEDIA_TYPE videoType;
    
    // Playback status
    MEDIA_STATE mediaState;
    
    // Class data lock
    NSLock* dataLock;
    
    // Sample and pixel buffers for video frames
    CMSampleBufferRef latestSampleBuffer;
    CMSampleBufferRef currentSampleBuffer;
    NSLock* latestSampleBufferLock;
    
    // Video properties
    CGSize videoSize;
    Float64 videoLengthSeconds;
    float videoFrameRate;
    
    // Audio properties
    float currentVolume;
    BOOL playAudio;
    
    // OpenGL / Metal data
    GLuint videoTextureIdGL;
    id<MTLTexture> videoTextureMetal;
    id<MTLDevice> metalDevice;
    Class MTLTextureDescriptorClass;
    
    // Audio/video synchronisation state
    enum tagSyncState {
        SYNC_DEFAULT,
        SYNC_READY,
        SYNC_AHEAD,
        SYNC_BEHIND
    } syncStatus;
    
    // Media player type
    enum tagPLAYER_TYPE {
        PLAYER_TYPE_ON_TEXTURE,
        PLAYER_TYPE_NATIVE
    } playerType;
}

- (id)initWithMetalRendering:(BOOL)isMetalRendering;
- (void)deinit;
- (BOOL)load:(NSString*)filename mediaType:(MEDIA_TYPE)requestedType playImmediately:(BOOL)playOnTextureImmediately fromPosition:(float)seekPosition;
- (BOOL)unload;
- (BOOL)isPlayableOnTexture;
- (BOOL)isPlayableFullscreen;
- (MEDIA_STATE)getStatus;
- (int)getVideoHeight;
- (int)getVideoWidth;
- (float)getLength;
- (BOOL)play:(BOOL)fullscreen fromPosition:(float)seekPosition;
- (BOOL)pause;
- (BOOL)stop;
- (MEDIA_STATE)updateVideoData;
- (BOOL)seekTo:(float)position;
- (float)getCurrentPosition;
- (BOOL)setVolume:(float)volume;
- (BOOL)setVideoTexturePtr:(void*)texturePtr;
- (void)onPause;

@end
