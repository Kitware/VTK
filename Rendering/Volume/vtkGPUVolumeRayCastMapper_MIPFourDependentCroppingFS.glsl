/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper_MIPFourDependentCroppingFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Implementation of some functions used by the 4-component Maximum Intensity
// Projection (MIP) method when cropping is on.

#version 110

// GLSL Spec 1.10 rev 59 30-April-2004 defines gl_FragData[] but implementation
// older than the spec only has it as an extension
// (nVidia Linux driver 100.14.13, OpenGL version 2.1.1,
// on Quadro FX 3500/PCI/SSE2)
#extension GL_ARB_draw_buffers : enable

// max scalar buffer as an input
uniform sampler2D scalarBufferTexture;

// color buffer as an input
uniform sampler2D frameBufferTexture;

// 2D Texture fragment coordinates [0,1] from fragment coordinates
// the scalar frame buffer texture has the size of the plain buffer but
// we use a fraction of it. The texture coordinates is less than 1 if
// the reduction factor is less than 1.
vec2 fragTexCoord;

float initialMaxValue()
{
  return texture2D(scalarBufferTexture,fragTexCoord).r;
}

vec4 initialColor()
{
  return texture2D(frameBufferTexture,fragTexCoord);
}

void writeColorAndMaxScalar(vec4 color,
                            vec4 opacity,
                            float maxValue)
{
  // color framebuffer
  gl_FragData[0].r = color.r*opacity.a;
  gl_FragData[0].g = color.g*opacity.a;
  gl_FragData[0].b = color.b*opacity.a;
  gl_FragData[0].a=opacity.a;
  
  // max scalar framebuffer
  gl_FragData[1].r=maxValue;
  gl_FragData[1].g=0.0;
  gl_FragData[1].b=0.0;
  gl_FragData[1].a=0.0;
}
