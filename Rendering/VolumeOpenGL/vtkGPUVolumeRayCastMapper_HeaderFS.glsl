/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper_HeaderFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#version 110

// Depth map of the polygonal geometry
uniform sampler2D depthTexture;

// 2D noise texture to jitter the starting point of the ray in order to
// remove patterns when the opacity transfer function make the data on the
// border of the dataset to be visible.
uniform sampler2D noiseTexture;

uniform vec2 windowLowerLeftCorner;
uniform vec2 invOriginalWindowSize;
uniform vec2 invWindowSize;

// Change-of-coordinate matrix from eye space to texture space
uniform mat4 textureToEye;

// Entry position (global scope)
vec3 pos;
// Incremental vector in texture space (global scope)
vec3 rayDir;

// Abscissa along the ray of the point on the depth map
// tracing stops when t>=tMax
float tMax;

// 2D Texture fragment coordinates [0,1] from fragment coordinates
// the frame buffer texture has the size of the plain buffer but
// we use a fraction of it. The texture coordinates is less than 1 if
// the reduction factor is less than 1.
vec2 fragTexCoord;

// Defined in the right projection method.
// May use pos in global scope as input.
// Use rayDir in global scope as output.
void incrementalRayDirection();
void trace();

void main()
{

  // device coordinates are between -1 and 1
  // we need texture coordinates between 0 and 1
  // the depth buffer has the original size buffer.
  fragTexCoord=(gl_FragCoord.xy-windowLowerLeftCorner)*invWindowSize;
  vec4 depth=texture2D(depthTexture,fragTexCoord);
  if(gl_FragCoord.z>=depth.x) // depth test
    {
    discard;
    }

  // color buffer or max scalar buffer have a reduced size.
  fragTexCoord=(gl_FragCoord.xy-windowLowerLeftCorner)*invOriginalWindowSize;
  // Abscissa of the point on the depth buffer along the ray.
  // point in texture coordinates
  vec4 maxPoint;

  // from window coordinates to normalized device coordinates
  maxPoint.x=(gl_FragCoord.x-windowLowerLeftCorner.x)*2.0*invWindowSize.x-1.0;
  maxPoint.y=(gl_FragCoord.y-windowLowerLeftCorner.y)*2.0*invWindowSize.y-1.0;
  maxPoint.z=(2.0*depth.x-(gl_DepthRange.near+gl_DepthRange.far))/gl_DepthRange.diff;
  maxPoint.w=1.0;

  // from normalized device coordinates to eye coordinates
  maxPoint=gl_ProjectionMatrixInverse*maxPoint;

  // from eye coordinates to texture coordinates
  maxPoint=textureToEye*maxPoint;
  // homogeneous to cartesian coordinates
  maxPoint/=maxPoint.w;

  // Entry position. divide by q.
  // pos=gl_TexCoord[0].xyz/gl_TexCoord[0].w;

  pos.x=gl_TexCoord[0].x/gl_TexCoord[0].w;
  pos.y=gl_TexCoord[0].y/gl_TexCoord[0].w;
  pos.z=gl_TexCoord[0].z/gl_TexCoord[0].w;

  // Incremental vector in texture space. Computation depends on the
  // type of projection (parallel or perspective)
  incrementalRayDirection();

  vec4 noiseValue=texture2D(noiseTexture,pos.xy*100.0); // with repeat/tiling mode on the noise texture.

  pos+=(noiseValue.x)*rayDir;

  tMax=length(maxPoint.xyz-pos.xyz) /length(rayDir);


  // Tracing method. Set the final fragment color.
  trace();
}
