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

varying vec4 vertexVCVSOutput;
varying float radiusVCVSOutput;
varying float lengthVCVSOutput;
varying vec3 centerVCVSOutput;
varying vec3 orientVCVSOutput;

uniform int cameraParallel;

void main()
{
  //VTK::Picking::Impl

  //VTK::Color::Impl

  //VTK::Normal::Impl

  //VTK::TCoord::Impl

  //VTK::Clip::Impl

  vertexVCVSOutput = MCVCMatrix * vertexMC;
  centerVCVSOutput = vertexVCVSOutput.xyz;
  radiusVCVSOutput = radiusMC;
  lengthVCVSOutput = length(orientMC);
  orientVCVSOutput = normalMatrix * normalize(orientMC);

  // make sure it is pointing out of the screen
  if (orientVCVSOutput.z < 0.0)
    {
    orientVCVSOutput = -orientVCVSOutput;
    }

  // make the basis
  vec3 xbase;
  vec3 ybase;
  vec3 dir = vec3(0.0,0.0,1.0);
  if (cameraParallel == 0)
    {
    dir = normalize(-vertexVCVSOutput.xyz);
    }
  if (abs(dot(dir,orientVCVSOutput)) == 1.0)
    {
    xbase = normalize(cross(vec3(0.0,1.0,0.0),orientVCVSOutput));
    ybase = cross(xbase,orientVCVSOutput);
    }
  else
    {
    xbase = normalize(cross(orientVCVSOutput,dir));
    ybase = cross(orientVCVSOutput,xbase);
    }

  vec3 offsets = offsetMC.xyz*2.0-1.0;
  vertexVCVSOutput.xyz = vertexVCVSOutput.xyz +
    radiusVCVSOutput*offsets.x*xbase +
    radiusVCVSOutput*offsets.y*ybase +
    0.5*lengthVCVSOutput*offsets.z*orientVCVSOutput;

  gl_Position = VCDCMatrix * vertexVCVSOutput;
}
