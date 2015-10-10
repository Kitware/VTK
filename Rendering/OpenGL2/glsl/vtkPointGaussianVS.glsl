//VTK::System::Dec

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

attribute vec4 vertexMC;
attribute vec2 offsetMC;

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

varying vec2 offsetVCVSOutput;
uniform int cameraParallel;

uniform float triangleScale;

void main()
{
  //VTK::Color::Impl

  //VTK::Normal::Impl

  //VTK::TCoord::Impl

  //VTK::Clip::Impl

  // compute the projected vertex position
  vec4 vertexVC = MCVCMatrix * vertexMC;

  // the offsets sent down are positioned
  // at 2.0*radius*triangleScale from the center of the
  // gaussian.  This has to be consistent
  // with the offsets we build into the VBO
  float radius = 0.5*sqrt(dot(offsetMC,offsetMC))/triangleScale;

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

  offsetVCVSOutput = offsetMC/radius;
  gl_Position = VCDCMatrix * vertexVC;
}
