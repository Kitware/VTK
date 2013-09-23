//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkLineIntegralConvolution2D_VT.glsl
//
//  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
//  All rights reserved.
//  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
//
//     This software is distributed WITHOUT ANY WARRANTY; without even
//     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//     PURPOSE.  See the above copyright notice for more information.
//
//=========================================================================

// move vector field to normalized image space
// pre-procesing for vtkLineIntegralConvolution2D

#version 110

uniform sampler2D texVectors; // input texture
uniform vec2      uTexSize;   // size of texture

// select vector components.
// see vtkLineIntegralConvolution2D.cxx for implementation
vec2 getSelectedComponents(vec4 V);

void main(void)
{
  vec2 V = getSelectedComponents(texture2D(texVectors, gl_TexCoord[0].st));
  V = V/uTexSize;
  gl_FragData[0] = vec4(V, 0.0, 1.0);
}
