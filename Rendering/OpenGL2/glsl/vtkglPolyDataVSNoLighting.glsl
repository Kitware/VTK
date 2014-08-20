/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkglPolyDataVSNoLighting.glsl

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

// all variables that represent positions or directions have a suffix
// indicating the coordinate system they are in. The possible values are
// MC - Model Coordinates
// WC - WC world coordinates
// VC - View Coordinates
// DC - Display Coordinates
attribute vec4 vertexMC;

// material property values
//VTK::Color::Dec

// camera and actor matrix values
uniform mat4 MCVCMatrix;  // combined Model to View transform
uniform mat4 VCDCMatrix;  // the camera's projection matrix

// Texture coordinates
//VTK::TCoord::Dec

void main()
{
  //VTK::Color::Impl

  //VTK::TCoord::Impl

  gl_Position = VCDCMatrix * MCVCMatrix * vertexMC;
}
