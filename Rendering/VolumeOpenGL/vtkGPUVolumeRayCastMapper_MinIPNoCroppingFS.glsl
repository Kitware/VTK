/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper_MinIPNoCroppingFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Implementation of some functions used by the Minimum Intensity projection
// (MinIP) method when cropping is off.

#version 110

float initialMinValue()
{
  return 1.0;
}

void writeColorAndMinScalar(vec4 sample,
                            vec4 opacity,
                            float minValue)
{
  // we don't need to write minValue to a buffer when there is no cropping.
  // color framebuffer
  gl_FragColor.r =sample.r * opacity.a;
  gl_FragColor.g =sample.g * opacity.a;
  gl_FragColor.b =sample.b * opacity.a;
  gl_FragColor.a=opacity.a;
}
