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


// Texture coordinates
//VTK::TCoord::Dec

varying vec4 fcolor;

void main()
{
  gl_FragColor = fcolor;
  //VTK::TCoord::Impl
}
