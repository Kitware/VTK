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

// The following line handles system declarations such as
// default precisions, or defining precisions to null
//VTK::System::Dec

// Fragment shader used by the gaussian blur filter render pass.

varying vec2 tcoordVC;
uniform sampler2D source;

uniform float coef[3];
uniform float offsetx;
uniform float offsety;

void main(void)
{
  vec2 offset=vec2(offsetx,offsety);

  gl_FragColor = coef[0]*texture2D(source,tcoordVC-offset)
    +coef[1]*texture2D(source,tcoordVC)
    +coef[2]*texture2D(source,tcoordVC+offset);
}
