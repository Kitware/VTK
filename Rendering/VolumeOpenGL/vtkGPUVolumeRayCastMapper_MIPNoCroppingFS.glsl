/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper_MIPNoCroppingFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Implementation of some functions used by the Maximum Intensity projection
// (MIP) method when cropping is off.

#version 110

float initialMaxValue()
{
  return 0.0;
}

void writeColorAndMaxScalar(vec4 sample,
                            vec4 opacity,
                            float maxValue)
{
  // we don't need to write maxValue to a buffer when there is no cropping.
  // color framebuffer
  gl_FragColor.r =sample.r * opacity.a;
  gl_FragColor.g =sample.g * opacity.a;
  gl_FragColor.b =sample.b * opacity.a;
  gl_FragColor.a=opacity.a;
}
