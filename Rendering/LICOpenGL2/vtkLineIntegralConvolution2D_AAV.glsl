//VTK::System::Dec

//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkLineIntegralConvolution2D_AAV.glsl
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

// Anti-alias stage in vtkLineIntegralConvolution2D
// vertical pass of a Gaussian convolution

// the output of this shader
//VTK::Output::Dec

uniform sampler2D texLIC; // input texture
uniform float     uDy;    // fragment size

varying vec2 tcoordVC;

// neighbor offsets
vec2 fragDy[3] = vec2[3](vec2(0.0,-uDy), vec2(0.0,0.0), vec2(0.0,uDy));

// factored 3x3 Gaussian kernel
// K^T*K = G
float K[3] = float[3](0.141421356, 0.707106781, 0.141421356);

// determine if the fragment was masked
bool Masked(float val){ return val != 0.0; }

void main(void)
{
  vec2 lictc = tcoordVC.st;
  vec4 lic[3];
  bool dontUse = false;
  float conv = 0.0;
  for (int i=0; i<3; ++i)
    {
    vec2 tc = lictc + fragDy[i];
    lic[i] = texture2D(texLIC, tc);
    dontUse = dontUse || Masked(lic[i].g);
    conv = conv + K[i] * lic[i].r;
    }
  // output is (conv, mask, skip, 1)
  if (dontUse)
    {
    gl_FragData[0] = vec4(lic[1].rg, 1.0, 1.0);
    }
  else
    {
    gl_FragData[0] = vec4(conv, lic[1].gb, 1.0);
    }
}
