/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper_CompositeMaskFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Fragment program part with ray cast and composite method with masks.

#version 110

uniform sampler3D dataSetTexture;
uniform sampler3D maskTexture;
uniform sampler1D mask1ColorTexture;
uniform sampler1D mask2ColorTexture;
uniform sampler1D opacityTexture;

uniform vec3 lowBounds;
uniform vec3 highBounds;

uniform float maskBlendFactor;

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
  
  vec4 maskValue;
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
      opacity=texture1D(opacityTexture,scalar);
      
      if(maskBlendFactor==0.0)
        {
        color=shade(value);
        }
      else
        {
        // get the mask value at this same location
        maskValue=texture3D(maskTexture,pos);
        if(maskValue.a==0.0)
          {
          color=shade(value);
          }
        else
          {
          if(maskValue.a==1.0/255.0)
            {
            color=texture1D(mask1ColorTexture,scalar);
            }
          else
            {
            // maskValue.a == 2.0/255.0
            color=texture1D(mask2ColorTexture,scalar);
            }
          color.a=1.0;
          if(maskBlendFactor<1.0)
            {
            color=(1.0-maskBlendFactor)*shade(value)+maskBlendFactor*color;
            }
//        color.r = 1;
//        color.g = 0;
//        color.b = 0;
//        color.a = 1;
          }
        }
      
      color=color*opacity.a;
      destColor=destColor+color*remainOpacity;
      remainOpacity=remainOpacity*(1.0-opacity.a);
      
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
