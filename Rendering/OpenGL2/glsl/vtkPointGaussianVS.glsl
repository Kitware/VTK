/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointGaussianVS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// this shader implements imposters in OpenGL for Spheres

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
attribute vec2 offsetMC;
//attribute float radiusMC;

// optional normal declaration
//VTK::Normal::Dec

// Texture coordinates
//VTK::TCoord::Dec

// material property values
//VTK::Color::Dec

// clipping plane vars
//VTK::Clip::Dec

// camera and actor matrix values
//VTK::Camera::Dec

varying vec2 offsetVC;
uniform int cameraParallel;

void main()
{
  //VTK::Color::Impl

  //VTK::Normal::Impl

  //VTK::TCoord::Impl

  //VTK::Clip::Impl

  // compute the projected vertex position
  vec4 vertexVC = MCVCMatrix * vertexMC;
  float radius2 = dot(offsetMC,offsetMC)*0.25;

  // make the triangle face the camera
  if (cameraParallel == 0)
    {
    vec3 dir = normalize(-vertexVC.xyz);
    vec3 base2 = normalize(cross(dir,vec3(1.0,0.0,0.0)));
    vec3 base1 = cross(base2,dir);
    vertexVC.xyz = vertexVC.xyz + offsetMC.x*base1 + offsetMC.y*base2;
    }
  else
    {
    // add in the offset
    vertexVC.xy = vertexVC.xy + offsetMC;
    }

  offsetVC = offsetMC/sqrt(radius2);
  gl_Position = VCDCMatrix * vertexVC;
}
