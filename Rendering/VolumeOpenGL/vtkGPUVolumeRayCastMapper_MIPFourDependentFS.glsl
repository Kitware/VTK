/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper_MIPFourDependentFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Fragment program with ray cast and 4-dependent-component Maximum Intensity
// Projection (MIP) method.
// Compilation: header part and the projection part are inserted first.
// pos is defined and initialized in header
// rayDir is defined in header and initialized in the projection part

#version 110

uniform sampler3D dataSetTexture;
uniform sampler1D opacityTexture;

uniform vec3 lowBounds;
uniform vec3 highBounds;

// Entry position (global scope)
vec3 pos;
// Incremental vector in texture space (global scope)
vec3 rayDir;

float tMax;

// Sub-functions, depending on cropping mode
float initialMaxValue();
vec4 initialColor();
void writeColorAndMaxScalar(vec4 color,
                            vec4 opacity,
                            float maxValue);

void trace(void)
{
  // Max intensity is the lowest value.
  float maxValue=initialMaxValue();
  vec4 color=initialColor();
  bool inside=true;
  float t=0.0;
  vec4 sample;
  bool changed=false;

  // We NEED two nested while loops. It is a trick to work around hardware
  // limitation about the maximum number of loops.
  while(inside)
    {
    while(inside)
      {
      sample=texture3D(dataSetTexture,pos);
      if(sample.w>maxValue)
        {
        changed=true;
        maxValue=sample.w;
        color=sample;
        }
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

  if(changed)
    {
    vec4 opacity=texture1D(opacityTexture,maxValue);
    writeColorAndMaxScalar(color,opacity,maxValue);
    }
  else
    {
    discard;
    }
}
