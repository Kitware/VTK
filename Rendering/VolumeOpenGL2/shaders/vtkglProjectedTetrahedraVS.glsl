//VTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkglProjectedTetrahedra.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


// all variables that represent positions or directions have a suffix
// indicating the coordinate system they are in. The possible values are
// MC - Model Coordinates
// WC - WC world coordinates
// VC - View Coordinates
// DC - Display Coordinates
attribute vec4 vertexDC;
attribute vec3 scalarColor;
attribute float depthArray;
attribute float attenuationArray;

varying float fdepth;
varying float fattenuation;
varying vec3 fcolor;

void main()
{
  fcolor = scalarColor;
  fdepth = depthArray;
  fattenuation = attenuationArray;
  gl_Position = vertexDC;
}
