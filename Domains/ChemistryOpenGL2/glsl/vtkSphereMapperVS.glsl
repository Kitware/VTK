//VTK::System::Dec

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

attribute vec4 vertexMC;
attribute vec2 offsetMC;

// optional normal declaration
//VTK::Normal::Dec

// Texture coordinates
//VTK::TCoord::Dec

uniform mat3 normalMatrix; // transform model coordinate directions to view coordinates

// material property values
//VTK::Color::Dec

// clipping plane vars
//VTK::Clip::Dec

// camera and actor matrix values
//VTK::Camera::Dec

varying vec4 vertexVCVSOutput;
varying float radiusVCVSOutput;
varying vec3 centerVCVSOutput;

uniform int cameraParallel;

void main()
{
  //VTK::Color::Impl

  //VTK::Normal::Impl

  //VTK::TCoord::Impl

  //VTK::Clip::Impl

  // compute the projected vertex position
  vertexVCVSOutput = MCVCMatrix * vertexMC;
  centerVCVSOutput = vertexVCVSOutput.xyz;
  radiusVCVSOutput = length(offsetMC)*0.5;

  // make the triangle face the camera
  if (cameraParallel == 0)
    {
    vec3 dir = normalize(-vertexVCVSOutput.xyz);
    vec3 base2 = normalize(cross(dir,vec3(1.0,0.0,0.0)));
    vec3 base1 = cross(base2,dir);
    vertexVCVSOutput.xyz = vertexVCVSOutput.xyz + offsetMC.x*base1 + offsetMC.y*base2;
    }
  else
    {
    // add in the offset
    vertexVCVSOutput.xy = vertexVCVSOutput.xy + offsetMC;
    }

  gl_Position = VCDCMatrix * vertexVCVSOutput;
}
