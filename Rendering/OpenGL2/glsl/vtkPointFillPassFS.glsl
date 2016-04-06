//VTK::System::Dec

// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkPointFillPassFS.glsl
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
uniform float nearC;
uniform float farC;
uniform float MinimumCandidateAngle;
uniform float CandidatePointRatio;
uniform vec2  pixelToTCoord;

// the output of this shader
//VTK::Output::Dec

void main(void)
{
  // original pixel
  float fbdepth = texture2D(depth,tcoordVC).r;
  fbdepth = 2.0*nearC/(farC + nearC -fbdepth*(farC - nearC));
  vec4  fbcolor = texture2D(source,tcoordVC);

  vec4  closestColor = vec4(0.0,0.0,0.0,0.0);
  float closestDepth = 0.0;
  int count = 0;

  // we track the theta range twice
  // the original values and a shifted by pi version
  // this is to deal with the cyclic nature of atan2
  // e.g. 1 degree and 359 degrees are really only 2
  // degrees apart. have to handle that.
  float minTheta = 4.0;
  float maxTheta = -4.0;
  float minTheta2 = 4.0;
  float maxTheta2 = -4.0;

  // loop over pixels
  for (int i = -3; i <= 3; i++)
    {
    for (int j = -3; j <= 3; j++)
      {
      float adepth = texture2D(depth,tcoordVC + pixelToTCoord*vec2(i,j)).r;
      float mdepth = 2.0*nearC/(farC + nearC -adepth*(farC - nearC));
      if (mdepth < fbdepth*CandidatePointRatio && (i != 0 || j != 0))
        {
        float theta = atan(float(j),float(i));
        minTheta = min(minTheta,theta);
        maxTheta = max(maxTheta,theta);
        if (theta > 0)
          {
          theta -= 3.1415926;
          }
        else
          {
          theta += 3.1415926;
          }
        minTheta2 = min(minTheta2,theta);
        maxTheta2 = max(maxTheta2,theta);
        count = count + 1;
        closestColor += texture2D(source,tcoordVC + pixelToTCoord*vec2(i,j));
        closestDepth += adepth;
        }
      }
    }

// must be at least the candidate angle of support
if (min(maxTheta-minTheta, maxTheta2-minTheta2) > MinimumCandidateAngle)
  {
  gl_FragData[0] = closestColor/count;
  gl_FragDepth = closestDepth/count;
  }
else
  {
  gl_FragData[0] = fbcolor;
  gl_FragDepth = fbdepth;
  }
}
