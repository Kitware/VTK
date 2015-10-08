//VTK::System::Dec

//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkLineIntegralConvolution2D_LIC0.glsl
//
//  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
//  All rights reserved.
//  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
//
//     This software is distributed WITHOUT ANY WARRANTY; without even
//     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//     PURPOSE.  See the above copyright notice for more information.
//
//=========================================================================

/**
This shader initializes the convolution for the LIC computation.
*/

// the output of this shader
//VTK::Output::Dec

uniform sampler2D texMaskVectors;
uniform sampler2D texNoise;
uniform sampler2D texLIC;

uniform int   uStepNo;         // in step 0 initialize lic and seeds, else just seeds
uniform int   uPassNo;         // in pass 1 hpf of pass 0 is convolved.
uniform float uMaskThreshold;  // if |V| < uMaskThreshold render transparent
uniform vec2  uNoiseBoundsPt1; // tc of upper right pt of noise texture

varying vec2 tcoordVC;

// convert from vector coordinate space to noise coordinate space.
// the noise texture is tiled across the *whole* domain
vec2 VectorTCToNoiseTC(vec2 vectc)
{
  return vectc/uNoiseBoundsPt1;
}

// get the texture coordidnate to lookup noise value. this
// depends on the pass number.
vec2 getNoiseTC(vec2 vectc)
{
  // in pass 1 : convert from vector tc to noise tc
  // in pass 2 : use vector tc
  if (uPassNo == 0)
    {
    return VectorTCToNoiseTC(vectc);
    }
  else
    {
    return vectc;
    }
}

// look up noise value at the given location. The location
// is supplied in vector texture coordinates, hence the
// need to convert to noise texture coordinates.
float getNoise(vec2 vectc)
{
  return texture2D(texNoise, getNoiseTC(vectc)).r;
}

void main(void)
{
  vec2 vectc = tcoordVC.st;

  // lic => (convolution, mask, 0, step count)
  if (uStepNo == 0)
    {
    float maskCriteria = length(texture2D(texMaskVectors, vectc).xyz);
    float maskFlag;
    if (maskCriteria <= uMaskThreshold)
      {
      maskFlag = 1.0;
      }
    else
      {
      maskFlag = 0.0;
      }
    float noise = getNoise(vectc);
    gl_FragData[0] = vec4(noise, maskFlag, 0.0, 1.0);
    }
  else
    {
    gl_FragData[0] = texture2D(texLIC, vectc);
    }

  // initial seed
  gl_FragData[1] = vec4(vectc, 0.0, 1.0);
}
