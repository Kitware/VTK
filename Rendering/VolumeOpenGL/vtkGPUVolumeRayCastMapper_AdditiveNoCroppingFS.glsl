/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper_AdditiveNoCroppingFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Implementation of some functions used by the Additive method when cropping
// is off.

#version 110

float initialValue()
{
  return 0.0;
}
void writeColorAndSumScalar(vec4 color,
                            float sumValue)
{
  // we don't need to write sumValue to a buffer when there is no cropping.
  // color framebuffer
  gl_FragColor=color;
}
