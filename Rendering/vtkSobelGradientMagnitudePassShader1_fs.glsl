// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkSobelGradientMagnitudePassShader1_fs.glsl
//
//  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
//  All rights reserved.
//  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
//
//     This software is distributed WITHOUT ANY WARRANTY; without even
//     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//     PURPOSE.  See the above copyright notice for more information.
//
// ============================================================================

// Fragment shader used by the first pass of the Sobel filter render pass.

#version 110

// GLSL Spec 1.10 rev 59 30-April-2004 defines gl_FragData[] but implementation
// older than the spec only has it as an extension
// (nVidia Linux driver 100.14.13, OpenGL version 2.1.1,
// on Quadro FX 3500/PCI/SSE2)
#extension GL_ARB_draw_buffers : enable

uniform sampler2D source;
uniform float step; // 1/W

void main(void)
{
  vec2 tcoord=gl_TexCoord[0].st;
  vec2 offset=vec2(step,0.0);
  vec4 t1=texture2D(source,tcoord-offset);
  vec4 t2=texture2D(source,tcoord);
  vec4 t3=texture2D(source,tcoord+offset);
  
  // Gx
  
  // version with unclamped float textures t3-t1 will be in [-1,1]
//  gl_FragData[0]=t3-t1;
  
  // version with clamped unchar textures (t3-t1+1)/2 stays in [0,1]
  gl_FragData[0]=(t3-t1+1.0)/2.0;
  
  // Gy
  gl_FragData[1]=(t1+2.0*t2+t3)/4.0;
}
