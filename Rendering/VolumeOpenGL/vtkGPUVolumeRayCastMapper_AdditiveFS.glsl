/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper_AdditiveFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Fragment program with ray cast and Maximum Intensity Projection (MIP)
// method.
// Compilation: header part and the projection part are inserted first.
// pos is defined and initialized in header
// rayDir is defined in header and initialized in the projection part
// initMaxValue() and writeColorAndMaxScalar are defined in some specific
// file depending on cropping flag being on or off.

#version 110

uniform sampler3D dataSetTexture;
uniform sampler1D colorTexture;
uniform sampler1D opacityTexture;

uniform vec3 lowBounds;
uniform vec3 highBounds;

// Entry position (global scope)
vec3 pos;
// Incremental vector in texture space (global scope)
vec3 rayDir;

float tMax;

// Sub-functions, depending on cropping mode
float initialValue();
void writeColorAndSumScalar(vec4 color,
                            float sumValue);

void trace(void)
{
  // Max intensity is the lowest value.
  float sumValue=initialValue();
  bool inside=true;
  vec4 sample;

  float t=0.0;
  // We NEED two nested while loops. It is trick to work around hardware
  // limitation about the maximum number of loops.
  while(inside)
    {
    while(inside)
      {
      sample=texture3D(dataSetTexture,pos);

      vec4 opacity=texture1D(opacityTexture,sample.r);
      sumValue=sumValue+opacity.a*sample.r;
      pos=pos+rayDir;
      t+=1.0;

      // yes, t<tMax && all(greaterThanEqual(pos,lowBounds))
      // && all(lessThanEqual(pos,highBounds));
      // looks better but the latest nVidia 177.80 has a bug...
      inside=t<tMax && pos.x>=lowBounds.x && pos.y>=lowBounds.y
        && pos.z>=lowBounds.z && pos.x<=highBounds.x && pos.y<=highBounds.y
        && pos.z<=highBounds.z;
      }
    }

  float clampedValue=clamp(sumValue,0.0,1.0);
  vec4 color=vec4(clampedValue,clampedValue,clampedValue,1.0); // opaque

  writeColorAndSumScalar(color,sumValue);
}
