/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper_CompositeFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Fragment program part with ray cast and composite method.

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

// from cropping vs no cropping
vec4 initialColor();

// from 1 vs 4 component shader.
float scalarFromValue(vec4 value);
vec4 colorFromValue(vec4 value);

// from noshade vs shade.
void initShade();
vec4 shade(vec4 value);

void trace(void)
{
  vec4 destColor=initialColor();
  float remainOpacity=1.0-destColor.a;

  bool inside=true;
  
  vec4 color;
  vec4 opacity;

  initShade();
  
  float t=0.0;
  
  // We NEED two nested while loops. It is trick to work around hardware
  // limitation about the maximum number of loops.

  while(inside)
    {  
    while(inside)
      {
      vec4 value=texture3D(dataSetTexture,pos);
      float scalar=scalarFromValue(value);
      // opacity is the sampled texture value in the 1D opacity texture at
      // scalarValue
      opacity=texture1D(opacityTexture,scalar);
      if(opacity.a>0.0)
        {
        color=shade(value);
        color=color*opacity.a;
        destColor=destColor+color*remainOpacity;
        remainOpacity=remainOpacity*(1.0-opacity.a);
        }
      pos=pos+rayDir;
      t+=1.0;
      inside=t<tMax && all(greaterThanEqual(pos,lowBounds))
        && all(lessThanEqual(pos,highBounds))
        && (remainOpacity>=0.0039); // 1/255=0.0039
      }
    }
  gl_FragColor = destColor;
  gl_FragColor.a = 1.0-remainOpacity;
}
