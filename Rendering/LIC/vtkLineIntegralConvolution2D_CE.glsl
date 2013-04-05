//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkLineIntegralConvolution2D_CE.glsl
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

// gray scale contrast enhance stage implemented via histogram stretching
// if the min and max are tweaked it can generate out-of-range values
// these will be clamped in 0 to 1

#version 110

uniform sampler2D texLIC;  // most recent lic pass
uniform float uMin;        // min gray scale color value
uniform float uMaxMinDiff; // max-min

void main( void )
{
  vec4 lic = texture2D(texLIC, gl_TexCoord[0].st);
  if (lic.g!=0.0)
    {
    gl_FragData[0] = lic;
    }
  else
    {
    float CElic = clamp((lic.r - uMin)/uMaxMinDiff, 0.0, 1.0);
    gl_FragData[0] = vec4(CElic, lic.gb, 1.0);
    }
}
