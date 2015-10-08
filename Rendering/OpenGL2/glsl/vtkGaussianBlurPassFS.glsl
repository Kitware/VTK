//VTK::System::Dec

// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkGaussianBlurPassFS.glsl
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

// Fragment shader used by the gaussian blur filter render pass.

varying vec2 tcoordVC;
uniform sampler2D source;

uniform float coef[3];
uniform float offsetx;
uniform float offsety;

// the output of this shader
//VTK::Output::Dec

void main(void)
{
  vec2 offset=vec2(offsetx,offsety);

  gl_FragData[0] = coef[0]*texture2D(source,tcoordVC-offset)
    +coef[1]*texture2D(source,tcoordVC)
    +coef[2]*texture2D(source,tcoordVC+offset);
}
