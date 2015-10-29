//VTK::System::Dec

// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkDepthOfFieldPassFS.glsl
//
//  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
//  All rights reserved.
//  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
//
//     This software is distributed WITHOUT ANY WARRANTY; without even
//     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//     PURPOSE.  See the above copyright notice for more information.
//
// ============================================================================

// Fragment shader used by the DOF render pass.

varying vec2 tcoordVC;
uniform sampler2D source;
uniform sampler2D depth;

uniform vec2  worldToTCoord;
uniform vec2  pixelToTCoord;
uniform float nearC;
uniform float farC;
uniform float focalDisk;
uniform float focalDistance;

// the output of this shader
//VTK::Output::Dec

vec2 rand2(vec2 co)
{
  float a = 12.9898;
  float b = 78.233;
  float c = 43758.5453;
  float dt= dot(co.xy ,vec2(a,b));
  float sn= mod(dt,3.14);
  float dt2= dot(co.xy ,vec2(b,a));
  float sn2= mod(dt2,3.14);
  return vec2(fract(sin(sn) * c), fract(sin(sn2) * c));
}

void main(void)
{
  // original pixel
  vec4 fcolor = texture2D(source,tcoordVC);
  float fsum = 1.0;

  float fdist = focalDistance;
  // use automatic focalDistance?  when focalDistance = 0
  if (fdist == 0.0)
    {
    fdist = -farC * nearC / (texture2D(depth,vec2(0.5,0.5)).r * (farC - nearC) - farC);
    }

  float CoCScale = focalDisk*fdist*(farC - nearC)/(farC*nearC);
  float CoCBias = focalDisk*(nearC - fdist)/nearC;

  float cdepth = texture2D(depth,tcoordVC).r;
  float CoC = CoCScale*cdepth + CoCBias;

  // loop over pixels
  for (int i = 0; i < 9; i++)
    {
    for (int j = 0; j < 9; j++)
      {
      vec2 newOffset = pixelToTCoord*(vec2(i-4,j-4)*2.0 + rand2(tcoordVC));
      vec2 newtc = tcoordVC + newOffset;
      float tdepth = texture2D(depth,newtc).r;
      float tCoC = CoCScale*tdepth + CoCBias;
      // is the sample in range?
      float close = abs(tCoC) - length(newOffset/worldToTCoord);
      if (close > 0.0)
        {
        // is the sample to be blended in front?
        // or if behind, not too far behind
        if ((tCoC < 0.0 || (CoC > 0.0 && tCoC < (CoC * 2.0f))))
          {
          float weight = close/abs(tCoC);
          fcolor = fcolor + weight*texture2D(source,newtc);
          fsum += weight;
          }
        }
      }
    }

  gl_FragData[0] = fcolor/fsum;
}
