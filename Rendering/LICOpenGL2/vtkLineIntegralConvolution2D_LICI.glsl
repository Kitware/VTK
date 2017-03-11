//VTK::System::Dec

//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkLineIntegralConvolution2D_fs1.glsl
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

// the output of this shader
//VTK::Output::Dec

uniform sampler2D  texVectors;
uniform sampler2D  texNoise;
uniform sampler2D  texLIC;
uniform sampler2D  texSeedPts;

uniform int   uPassNo;          // in pass 1 hpf of pass 0 is convolved.
uniform float uStepSize;        // step size in parametric space

uniform vec2  uNoiseBoundsPt1;  // tc of upper right pt of noise texture

varying vec2 tcoordVC;

//VTK::LICVectorLookup::Impl

// convert from vector coordinate space to noise coordinate space.
// the noise texture is tiled across the whole domain
vec2 VectorTCToNoiseTC(vec2 vectc)
{
  return vectc/uNoiseBoundsPt1;
}

// get the texture coordidnate to lookup noise value.
// in pass 1 repeatedly tile the noise texture across
// the computational domain.
vec2 getNoiseTC(vec2 tc)
{
  if (uPassNo == 0)
    {
    return VectorTCToNoiseTC(tc);
    }
  else
    {
    return tc;
    }
}

// look up noise value at the given location. The location
// is supplied in vector texture coordinates, hence the need
// to convert to either noise or lic texture coordinates in
// pass 1 and 2 respectively.
float getNoise(vec2 vectc)
{
  return texture2D(texNoise, getNoiseTC(vectc)).r;
}

// fourth-order Runge-Kutta streamline integration
// no bounds checks are made, therefor it's essential
// to have the entire texture initialized to 0
// and set clamp to border and have border color 0
// an integer is set if the step was taken, keeping
// an accurate step count is necessary to prevent
// boundary artifacts. Don't count the step if
// all vector lookups are identically 0. This is
// a proxy for "stepped outside valid domain"
vec2 rk4(vec2 pt0, float dt, out bool count)
{
  count=true;
  float dtHalf = dt * 0.5;
  vec2 pt1;

  vec2 v0 = getVector(pt0);
  pt1 = pt0 + v0 * dtHalf;

  vec2 v1 = getVector(pt1);
  pt1 = pt0 + v1 * dtHalf;

  vec2 v2 = getVector(pt1);
  pt1 = pt0 + v2 * dt;

  vec2 v3 = getVector(pt1);
  vec2 vSum = v0 + v1 + v1 + v2 + v2 + v3;

  if (vSum == vec2(0.0,0.0))
    {
    count = false;
    }

  pt1 = pt0 + (vSum) * (dt * (1.0/6.0));

 return pt1;
}

void main(void)
{
  vec2 lictc = tcoordVC.st;
  vec4 lic = texture2D(texLIC, lictc);
  vec2 pt0 = texture2D(texSeedPts, lictc).st;

  bool count;
  vec2 pt1 = rk4(pt0, uStepSize, count);

  if (count)
    {
    // accumulate lic step
    // (lic, mask, 0, step count)
    float noise = getNoise(pt1);
    gl_FragData[0] = vec4(lic.r + noise, lic.g, 0.0, lic.a + 1.0);
    gl_FragData[1] = vec4(pt1, 0.0, 1.0);
    }
  else
    {
    // keep existing values
    gl_FragData[0] = lic;
    gl_FragData[1] = vec4(pt0, 0.0, 1.0);
    }
}
