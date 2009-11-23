// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkSobelGradientMagnitudePassShader2_fs.glsl
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

// Fragment shader used by the second pass of the Sobel filter render pass.

#version 110

uniform sampler2D gx1;
uniform sampler2D gy1;
uniform float step; // 1/H

void main(void)
{
  vec2 tcoord=gl_TexCoord[0].st;
  vec2 offset=vec2(0.0,step);
  
  // Gx
  
  vec4 tx1=texture2D(gx1,tcoord-offset);
  vec4 tx2=texture2D(gx1,tcoord);
  vec4 tx3=texture2D(gx1,tcoord+offset);
  
  // if clamped textures, rescale values from [0,1] to [-1,1]
  tx1=tx1*2.0-1.0;
  tx2=tx2*2.0-1.0;
  tx3=tx3*2.0-1.0;
  
  vec4 gx=(tx1+2.0*tx2+tx3)/4.0;
  
  // Gy
  
  vec4 ty1=texture2D(gy1,tcoord-offset);
  vec4 ty3=texture2D(gy1,tcoord+offset);
  
  vec4 gy=ty3-ty1;
  
  // the maximum gradient magnitude is sqrt(2.0) when for example gx=1 and
  // gy=1
//  gl_FragColor=sqrt((gx*gx+gy*gy)/2.0);
  gl_FragColor.rgb=sqrt((gx.rgb*gx.rgb+gy.rgb*gy.rgb)/2.0);
  gl_FragColor.a=1.0; // arbitrary choice.
}
