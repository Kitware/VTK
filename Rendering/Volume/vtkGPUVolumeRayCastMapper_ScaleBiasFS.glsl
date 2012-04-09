/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper_ScaleBiasFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This fragment shader scales and biases a framebuffer passed as a texture.
// Incoming color from the texture is pre-multiplied by alpha.
// It does not affect the alpha component.
// Passing the framebuffer as a texture allows the use of a reduction factor
// compared to the size of the final image.

#version 110

// Framebuffer to scale.
uniform sampler2D frameBufferTexture;
uniform float scale;
uniform float bias;

void main()
{
  vec4 color=texture2D(frameBufferTexture,gl_TexCoord[0].xy);
  if(color.a==0.0)
    {
    discard;
    }
  // As incoming color is pre-multiplied by alpha, the bias has to be
  // multiplied by alpha before adding it.
  gl_FragColor.r=color.r*scale+bias*color.a;
  gl_FragColor.g=color.g*scale+bias*color.a;
  gl_FragColor.b=color.b*scale+bias*color.a;
  gl_FragColor.a=color.a;
}
