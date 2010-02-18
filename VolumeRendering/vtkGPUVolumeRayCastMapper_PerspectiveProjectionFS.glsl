/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper_PerspectiveProjectionFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Perspective projection.

#version 110

// Entry position (global scope)
vec3 pos;
// Incremental vector in texture space (global scope)
vec3 rayDir;

// Camera position in texture space
uniform vec3 cameraPosition;
// Sample distance in world space
uniform float sampleDistance;
// Matrix coefficients: diagonal (a11,a22,a33)
uniform vec3 matrix1;
// Matrix coefficients: others (2a12,2a23,2a13)
uniform vec3 matrix2;

// Defined in the right projection method.
void incrementalRayDirection()
{
  // Direction of the ray in texture space, not normalized.
  rayDir=pos-cameraPosition;
  
  // x^2, y^2, z^2
  vec3 normDir=rayDir*rayDir;
  normDir.x=dot(normDir,matrix1);
  
  // xy,yz,zx
  vec3 coefs=rayDir*rayDir.yxz;
  coefs.x=dot(coefs,matrix2);

  // n^2
  normDir.x=normDir.x+coefs.x;
  
  // 1/n
  // normDir=1/sqrt(normDir)
  normDir.x=inversesqrt(normDir.x);
  
  // Final scale factor for the ray direction in texture space
  // normDir=normDir*sampleDistance
  normDir.x=normDir.x*sampleDistance;
  // Now, rayDir is the incremental direction in texture space
  rayDir=rayDir*normDir.x;
}
