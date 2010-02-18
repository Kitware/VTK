/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper_CompositeCroppingFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Implementation of some function used by the composite method when cropping
// is on.

#version 110

// color buffer as an input
uniform sampler2D frameBufferTexture;
// 2D Texture fragment coordinates [0,1] from fragment coordinates
// the frame buffer texture has the size of the plain buffer but
// we use a fraction of it. The texture coordinates is less than 1 if
// the reduction factor is less than 1.
vec2 fragTexCoord;

vec4 initialColor()
{
  return texture2D(frameBufferTexture,fragTexCoord);
}
