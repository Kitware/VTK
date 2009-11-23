// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkGaussianBlurPassShader_fs.glsl
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

#version 110

uniform sampler2D source;
uniform float coef[3];
uniform float offsetx;
uniform float offsety;

void main(void)
{
  vec2 tcoord=gl_TexCoord[0].st;
  vec2 offset=vec2(offsetx,offsety);
  
  gl_FragColor=coef[0]*texture2D(source,tcoord-offset)
    +coef[1]*texture2D(source,tcoord)
    +coef[2]*texture2D(source,tcoord+offset);
}
