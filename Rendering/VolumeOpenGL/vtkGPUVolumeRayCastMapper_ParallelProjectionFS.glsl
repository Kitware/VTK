/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper_ParallelProjectionFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Parallel projection.

#version 110

uniform vec3 parallelRayDirection;

// Incremental vector in texture space (global scope)
vec3 rayDir;

// Defined in the right projection method.
void incrementalRayDirection()
{
  rayDir=parallelRayDirection;
}
