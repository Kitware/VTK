//VTK::System::Dec

// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkSobelGradientMagnitudePass1FS.glsl
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

varying vec2 tcoordVC;
uniform sampler2D source;
uniform float stepSize; // 1/W

// the output of this shader
//VTK::Output::Dec

void main(void)
{
  vec2 offset=vec2(stepSize,0.0);
  vec4 t1=texture2D(source,tcoordVC-offset);
  vec4 t2=texture2D(source,tcoordVC);
  vec4 t3=texture2D(source,tcoordVC+offset);

  // Gx

  // version with unclamped float textures t3-t1 will be in [-1,1]
//  gl_FragData[0]=t3-t1;

  // version with clamped unchar textures (t3-t1+1)/2 stays in [0,1]
  gl_FragData[0]=(t3-t1+1.0)/2.0;

  // Gy
  gl_FragData[1]=(t1+2.0*t2+t3)/4.0;
}
