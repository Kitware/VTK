/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereMapperVS.glsl

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
attribute vec3 orientMC;
attribute vec4 offsetMC;
attribute float radiusMC;

// optional normal declaration
//VTK::Normal::Dec

//VTK::Picking::Dec

// Texture coordinates
//VTK::TCoord::Dec

uniform mat3 normalMatrix; // transform model coordinate directions to view coordinates

// material property values
//VTK::Color::Dec

// clipping plane vars
//VTK::Clip::Dec

// camera and actor matrix values
//VTK::Camera::Dec

varying vec4 vertexVCClose;
varying float radiusVC;
varying float lengthVC;
varying vec3 centerVC;
varying vec3 orientVC;

uniform int cameraParallel;

void main()
{
  //VTK::Picking::Impl

  //VTK::Color::Impl

  //VTK::Normal::Impl

  //VTK::TCoord::Impl

  //VTK::Clip::Impl

  vertexVCClose = MCVCMatrix * vertexMC;
  centerVC = vertexVCClose.xyz;
  radiusVC = radiusMC;
  lengthVC = length(orientMC);
  orientVC = normalMatrix * normalize(orientMC);

  // make sure it is pointing out of the screen
  if (orientVC.z < 0.0)
    {
    orientVC = -orientVC;
    }

  // make the basis
  vec3 xbase;
  vec3 ybase;
  vec3 dir = vec3(0.0,0.0,1.0);
  if (cameraParallel == 0)
    {
    dir = normalize(-vertexVCClose.xyz);
    }
  if (abs(dot(dir,orientVC)) == 1.0)
    {
    xbase = normalize(cross(vec3(0.0,1.0,0.0),orientVC));
    ybase = cross(xbase,orientVC);
    }
  else
    {
    xbase = normalize(cross(orientVC,dir));
    ybase = cross(orientVC,xbase);
    }

  vec3 offsets = offsetMC.xyz*2.0-1.0;
  vertexVCClose.xyz = vertexVCClose.xyz +
    radiusVC*offsets.x*xbase +
    radiusVC*offsets.y*ybase +
    0.5*lengthVC*offsets.z*orientVC;

  gl_Position = VCDCMatrix * vertexVCClose;
}
