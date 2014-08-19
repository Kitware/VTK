/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkglPolyData2DFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

//VTK:Precision

#ifdef GL_ES
#extension GL_OES_standard_derivatives : enable
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
#endif

// Texture coordinates
//VTK::TCoord::Dec

varying vec4 fcolor;

// Depth Peeling
//VTK::DepthPeeling::Dec

void main()
{
  gl_FragColor = fcolor;
  //VTK::TCoord::Impl
  //VTK::DepthPeeling::Impl
}
