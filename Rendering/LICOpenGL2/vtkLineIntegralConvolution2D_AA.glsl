//VTK::System::Dec

//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkLineIntegralConvolution2D_fs2.glsl
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

// the output of this shader
//VTK::Output::Dec

uniform sampler2D texLIC;         // inout texture
uniform vec2      uLICTexSize;    // input texture size
uniform vec4      uComputeBounds; // valid region of texture

varying vec2 tcoordVC;

// fragment size
float tcDx = 1.0 / uLICTexSize.x;
float tcDy = 1.0 / uLICTexSize.y;

// 3x3 Gaussian kernel
float K[9] = float[9](
  0.0191724, 0.100120, 0.0191724,
  0.1001200, 0.522831, 0.1001200,
  0.0191724, 0.100120, 0.0191724
  );

// tex lictc neighbor offsets
vec2 fragDx[9] = vec2[9](
  vec2(-tcDx,  tcDy),  vec2(0.0,  tcDy),  vec2(tcDx,  tcDy),
  vec2(-tcDx,  0.0 ),  vec2(0.0,  0.0 ),  vec2(tcDx,  0.0 ),
  vec2(-tcDx, -tcDy),  vec2(0.0, -tcDy),  vec2(tcDx, -tcDy)
  );

// valid domain (because no ghost fragments)
vec2 validBox[2] = vec2[2](
  vec2(      tcDx,       tcDy),
  vec2(1.0 - tcDx, 1.0 - tcDy)
  );

// valid domain (because no ghost fragments)
vec2 computeBounds0 = vec2(uComputeBounds[0] + tcDx, uComputeBounds[2] + tcDy);
vec2 computeBounds1 = vec2(uComputeBounds[1] - tcDx, uComputeBounds[3] - tcDy);

// determinie if fragment is outside the valid regions
bool OutOfBounds( vec2 tc )
{
  return any(lessThan(tc, computeBounds0))
   || any(greaterThan(tc, computeBounds1));
}

// determine if the fragment was masked
bool Masked(float val)
{
  return val == 1.0;
}



void main( void )
{
  vec2 lictc = tcoordVC.st;
  vec4 lic;
  bool dontUse = false;
  float conv = 0.0;
  for (int i=0; i<9; ++i)
    {
    vec2 tc = lictc + fragDx[i];
    lic = texture2D( texLIC, tc );
    dontUse = dontUse || OutOfBounds(tc) || Masked(lic.g);
    conv = conv + K[i] * lic.r;
    }

  lic = texture2D( texLIC, lictc );

  conv
    = ( 1.0 - float( dontUse ) ) * conv
    + float( dontUse ) * lic.r;

  gl_FragData[0] = vec4( conv, lic.g, 0.0, 1.0 );
}
