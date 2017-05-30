/*==============================================================================
Copyright (c) 2016 PTC Inc. All Rights Reserved.

Copyright (c) 2012-2014 Qualcomm Connected Experiences, Inc. All Rights Reserved.

Confidential and Proprietary - Protected under copyright and other laws.
==============================================================================*/

#include <jni.h>
#include <stdlib.h>

//Superset of OGL2
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

//Needed for GL_TEXTURE_EXTERNAL_OES
#include <GLES2/gl2ext.h>

#include "SampleUtils.h"
#include "CubeShaders.h"


#ifdef __cplusplus
extern "C"
{
#endif


unsigned int shaderProgramID    = 0;
GLint vertexHandle              = 0;
GLint textureCoordHandle        = 0;
GLint mvpMatrixHandle           = 0;
GLint textureMatrixHandle       = 0;

// Used to know which version of OpenGL we are using and render video accordingly
int _glVersion = -1;

// Ortho-quad geometry:
float orthoProjMatrix[16];
float orthoQuadVertices[] =
{
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f
};
// Note this intentionally flips the texture horizontally:
float orthoQuadTexCoords[] =
{
    1.0f, 0.0f,
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f
};
unsigned char orthoQuadIndices[]=
{
    0, 1, 2, 2, 3, 0
};


bool
setOrthographicProjectionMatrix(float orthoMatrix[16])
{
    // Set projection matrix for orthographic projection:
    for (int i = 0; i < 16; i++) orthoMatrix[i] = 0.0f;
    float nLeft   = -1.0;
    float nRight  =  1.0;
    float nBottom = -1.0;
    float nTop    =  1.0;
    float nNear   = -1.0;
    float nFar    =  1.0;
    
    orthoMatrix[0]  =  2.0f / (nRight - nLeft);
    orthoMatrix[5]  =  2.0f / (nTop - nBottom);
    orthoMatrix[10] =  2.0f / (nNear - nFar);
    orthoMatrix[12] = -(nRight + nLeft) / (nRight - nLeft);
    orthoMatrix[13] = -(nTop + nBottom) / (nTop - nBottom);
    orthoMatrix[14] =  (nFar + nNear) / (nFar - nNear);
    orthoMatrix[15] =  1.0f;
    
    return true;
}


JNIEXPORT int JNICALL
Java_com_vuforia_VuforiaMedia_VideoPlayerHelper_initMediaTexture(JNIEnv* env, jobject obj)
{
    // Generate OpenGL texture objects for SurfaceTexture:
    GLuint mediaTextureID;
    glGenTextures(1, &mediaTextureID);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mediaTextureID);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    return mediaTextureID;
}


JNIEXPORT void JNICALL
Java_com_vuforia_VuforiaMedia_VideoPlayerHelper_bindMediaTexture(JNIEnv *, jobject, jint mediaTextureID)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mediaTextureID);
}


JNIEXPORT void JNICALL
Java_com_vuforia_VuforiaMedia_VideoPlayerHelper_initNative(JNIEnv *, jobject, jint openGLVersion)
{
    _glVersion = openGLVersion;
}


JNIEXPORT int JNICALL
Java_com_vuforia_VuforiaMedia_VideoPlayerHelper_initFBO(JNIEnv* env, jobject obj, jint destTextureID, int videoWidth, int videoHeight)
{
    //LOG("VuforiaMedia initFBO, destTextureID: %d, size: %d, %d", destTextureID, videoWidth, videoHeight);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, destTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, videoWidth, videoHeight, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 0);
    
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, destTextureID, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    shaderProgramID     = SampleUtils::createProgramFromBuffer(cubeMeshVertexShader,
                                                               cubeFragmentShader);
    vertexHandle        = glGetAttribLocation(shaderProgramID,
                                              "vertexPosition");
    textureCoordHandle  = glGetAttribLocation(shaderProgramID,
                                              "vertexTexCoord");
    mvpMatrixHandle     = glGetUniformLocation(shaderProgramID,
                                               "modelViewProjectionMatrix");
    textureMatrixHandle = glGetUniformLocation(shaderProgramID,
                                               "textureMatrix");
    
    setOrthographicProjectionMatrix(orthoProjMatrix);
    
    SampleUtils::checkGlError("VuforiaMedia initFBO");
    
    return fbo;
}


JNIEXPORT void JNICALL
Java_com_vuforia_VuforiaMedia_VideoPlayerHelper_copyTexture(JNIEnv* env, jobject obj, jint mediaTextureID, jint destTextureID, jint fbo,
                                              jfloatArray textureMat, jint videoWidth, jint videoHeight)
{
	GLint saved_viewport[4];
	glGetIntegerv(GL_VIEWPORT, saved_viewport);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mediaTextureID);
    
    // OpenGL state changes:
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    
    // Unity does not unbind these buffers before we are called, so we have to do so.
    // This shouldn't have an effect on other rendering solutions as long as
    // the programmer is binding these buffers properly.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    if(_glVersion > 2)
      glBindVertexArray(0);

    glUseProgram(shaderProgramID);
    
    glVertexAttribPointer(vertexHandle, 3, GL_FLOAT, GL_FALSE, 0,
                          (const GLvoid*) &orthoQuadVertices[0]);
    glVertexAttribPointer(textureCoordHandle, 2, GL_FLOAT, GL_FALSE, 0,
                          (const GLvoid*) &orthoQuadTexCoords[0]);
    
    glViewport(0, 0, videoWidth, videoHeight);
    
    glEnableVertexAttribArray(vertexHandle);
    glEnableVertexAttribArray(textureCoordHandle);
    
    float *textureMatArray = env->GetFloatArrayElements(textureMat, 0);
    
    glUniformMatrix4fv(mvpMatrixHandle, 1, GL_FALSE,
                       (GLfloat*) &orthoProjMatrix[0]);
    glUniformMatrix4fv(textureMatrixHandle, 1, GL_FALSE,
                       (GLfloat*) textureMatArray);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE,
                   (const GLvoid*) &orthoQuadIndices[0]);
    
    env->ReleaseFloatArrayElements(textureMat, textureMatArray, 0);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    SampleUtils::checkGlError("VuforiaMedia copyTexture");

    glViewport(saved_viewport[0], saved_viewport[1], saved_viewport[2], saved_viewport[3]);
}


#ifdef __cplusplus
}
#endif
