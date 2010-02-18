/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper_MinIPFourDependentNoCroppingFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Implementation of some functions used by the 4-component Minimum Intensity
// Projection (MinIP) method when cropping is off.

#version 110

float initialMinValue()
{
  return 1.0;
}

vec4 initialColor()
{
  return vec4(0.0,0.0,0.0,0.0);
}

void writeColorAndMinScalar(vec4 color,
                            vec4 opacity,
                            float minValue)
{
  // minValue is not used
  
  // color framebuffer
  gl_FragColor.r = color.r*opacity.a;
  gl_FragColor.g = color.g*opacity.a;
  gl_FragColor.b = color.b*opacity.a;
  gl_FragColor.a=opacity.a;
}
