/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkglPolyData2DVS.glsl

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

attribute vec4 vertexWC;

// material property values
//VTK::Color::Dec
uniform vec3 ambientColor; // intensity weighted color

// Texture coordinates
//VTK::TCoord::Dec

uniform mat4 WCVCMatrix;  // World to view matrix

varying vec4 fcolor;

void main()
{
  gl_Position = WCVCMatrix*vertexWC;
  //VTK::TCoord::Impl
  fcolor = vec4(ambientColor + diffuseColor.rgb, diffuseColor.a);
}

