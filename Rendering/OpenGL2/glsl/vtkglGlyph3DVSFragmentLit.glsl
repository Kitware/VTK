/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkglPolyDataVSFragmentLit.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// this shader is used to implement lighting in the fragment shader
// it handles setting up the basic varying variables for the fragment shader

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

// optional normal declaration
//VTK::Normal::Dec

// Texture coordinates
//VTK::TCoord::Dec

uniform mat3 normalMatrix; // transform model coordinate directions to view coordinates

// material property values
//VTK::Color::Dec

// camera and actor matrix values
uniform mat4 MCVCMatrix;  // combined Model to View transform
uniform mat4 VCDCMatrix;  // the camera's projection matrix

//VTK::Glyph::Dec

// clipping plane vars
//VTK::Clip::Dec

varying vec4 vertexVC;

void main()
{
  //VTK::Glyph::Impl

  //VTK::Clip::Impl

  //VTK::Color::Impl

  //VTK::Normal::Impl

  //VTK::TCoord::Impl

  // compute the projected vertex position
  vertexVC = MCVCMatrix * vertex;
  gl_Position = VCDCMatrix * vertexVC;
}
