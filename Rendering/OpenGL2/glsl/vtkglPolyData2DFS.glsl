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

// The following line handle system declarations such a
// default precisions, or defining precisions to null
//VTK::System::Dec

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
