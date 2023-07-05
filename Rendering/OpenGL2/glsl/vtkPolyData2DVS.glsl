//VTK::System::Dec

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// all variables that represent positions or directions have a suffix
// indicating the coordinate system they are in. The possible values are
// MC - Model Coordinates
// WC - WC world coordinates
// VC - View Coordinates
// DC - Display Coordinates

in vec4 vertexWC;

// material property values
//VTK::Color::Dec

// Texture coordinates
//VTK::TCoord::Dec

// Apple Bug
//VTK::PrimID::Dec

// PointSize on GLES 3.0
//VTK::PointSizeGLES30::Dec

// LineWidth on GLES 3.0
//VTK::LineWidthGLES30::Dec

uniform mat4 WCVCMatrix;  // World to view matrix

void main()
{
  //VTK::PointSizeGLES30::Impl

  // Apple Bug
  //VTK::PrimID::Impl

  gl_Position = WCVCMatrix*vertexWC;
  //VTK::LineWidthGLES30::Impl
  //VTK::TCoord::Impl
  //VTK::Color::Impl
}
