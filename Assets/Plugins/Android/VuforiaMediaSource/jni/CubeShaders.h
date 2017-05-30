/*==============================================================================
Copyright (c) 2012-2014 Qualcomm Connected Experiences, Inc.
All Rights Reserved.
Confidential and Proprietary - Protected under copyright and other laws.
==============================================================================*/

#ifndef _QCAR_CUBE_SHADERS_H_
#define _QCAR_CUBE_SHADERS_H_

#ifndef USE_OPENGL_ES_1_1

static const char* cubeMeshVertexShader = " \
  \
attribute vec4 vertexPosition; \
attribute vec2 vertexTexCoord; \
 \
varying vec2 texCoord; \
 \
uniform mat4 modelViewProjectionMatrix; \
uniform mat4 textureMatrix; \
 \
void main() \
{ \
   gl_Position = modelViewProjectionMatrix * vertexPosition; \
   vec4 temp = vec4(1.0 - vertexTexCoord.x, 1.0 - vertexTexCoord.y, 1, 1); \
   texCoord = (textureMatrix * temp).xy; \
} \
";


static const char* cubeFragmentShader = " \
 \
#extension GL_OES_EGL_image_external : require \n \
 \
precision mediump float; \
 \
varying vec2 texCoord; \
 \
uniform samplerExternalOES texSampler2D; \
 \
void main() \
{ \
   gl_FragColor = texture2D(texSampler2D, texCoord); \
} \
";

#endif

#endif // _QCAR_CUBE_SHADERS_H_
