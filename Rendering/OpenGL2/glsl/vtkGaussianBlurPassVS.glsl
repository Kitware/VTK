/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGaussianBlurPassVS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// The following line handles system declarations such as
// default precisions, or defining precisions to null
//VTK::System::Dec

attribute vec4 vertexMC;

attribute vec2 tcoordMC;
varying vec2 tcoordVC;

void main()
{
  tcoordVC = tcoordMC;
  gl_Position = vertexMC;
}
