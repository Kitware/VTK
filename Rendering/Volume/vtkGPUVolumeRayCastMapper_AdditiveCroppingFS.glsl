/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper_AdditiveCroppingFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Implementation of some functions used by the Additive method when cropping
// is on.

#version 110

// GLSL Spec 1.10 rev 59 30-April-2004 defines gl_FragData[] but implementation
// older than the spec only has it as an extension
// (nVidia Linux driver 100.14.13, OpenGL version 2.1.1,
// on Quadro FX 3500/PCI/SSE2)
#extension GL_ARB_draw_buffers : enable

// sum scalar buffer as an input
uniform sampler2D scalarBufferTexture;
// 2D Texture fragment coordinates [0,1] from fragment coordinates
// the scalar frame buffer texture has the size of the plain buffer but
// we use a fraction of it. The texture coordinates is less than 1 if
// the reduction factor is less than 1.
vec2 fragTexCoord;

float initialValue()
{
  return texture2D(scalarBufferTexture,fragTexCoord).r;
}

void writeColorAndSumScalar(vec4 color,
                            float sumValue)
{
  // color framebuffer
  gl_FragData[0]=color;
  
  // sum scalar framebuffer.
  gl_FragData[1].r=sumValue;
  gl_FragData[1].g=0.0;
  gl_FragData[1].b=0.0;
  gl_FragData[1].a=0.0;
}
