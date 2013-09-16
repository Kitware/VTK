//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkSurfaceLICPainter_fs2.glsl
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

// This shader combines surface geometry, LIC, and  scalar colors.

#version 110

uniform sampler2D texVectors;       // vectors, depth
uniform sampler2D texGeomColors;    // scalar colors + lighting
uniform sampler2D texLIC;           // image lic
uniform int       uScalarColorMode; // select between blend, and map shader
uniform float     uLICIntensity;    // blend shader: blending factor for lic'd colors
uniform float     uMapBias;         // map shader: adjust the brightness of the result
uniform float     uMaskIntensity;   // blending factor for mask color
uniform vec3      uMaskColor;       // color for the masked out fragments

/**
Convert from RGB color space into HSL colorspace.
*/
vec3 RGBToHSL(vec3 RGB)
{
  vec3 HSL = vec3(0.0, 0.0, 0.0);

  float RGBMin = min(min(RGB.r, RGB.g), RGB.b);
  float RGBMax = max(max(RGB.r, RGB.g), RGB.b);
  float RGBMaxMinDiff = RGBMax - RGBMin;

  HSL.z = (RGBMax + RGBMin) / 2.0;

  if (RGBMaxMinDiff == 0.0)
    {
    // Gray scale
    HSL.x = 0.0;
    HSL.y = 0.0;
    }
  else
    {
    // Color
    if (HSL.z < 0.5)
      HSL.y = RGBMaxMinDiff / (RGBMax + RGBMin);
    else
      HSL.y = RGBMaxMinDiff / (2.0 - RGBMax - RGBMin);

    float dR
      = (((RGBMax - RGB.r) / 6.0) + (RGBMaxMinDiff / 2.0)) / RGBMaxMinDiff;
    float dG
      = (((RGBMax - RGB.g) / 6.0) + (RGBMaxMinDiff / 2.0)) / RGBMaxMinDiff;
    float dB
      = (((RGBMax - RGB.b) / 6.0) + (RGBMaxMinDiff / 2.0)) / RGBMaxMinDiff;

    if (RGB.r == RGBMax)
      HSL.x = dB - dG;
    else
    if (RGB.g == RGBMax)
      HSL.x = (1.0 / 3.0) + dR - dB;
    else
    if (RGB.b == RGBMax)
      HSL.x = (2.0 / 3.0) + dG - dR;

    if (HSL.x < 0.0)
      HSL.x += 1.0;

    if (HSL.x > 1.0)
      HSL.x -= 1.0;
    }

  return HSL;
}

/**
Helper for HSL to RGB conversion.
*/
float Util(float v1, float v2, float vH)
{
  if (vH < 0.0)
    vH += 1.0;

  if (vH > 1.0)
     vH -= 1.0;

  if ((6.0 * vH) < 1.0)
    return (v1 + (v2 - v1) * 6.0 * vH);

  if ((2.0 * vH) < 1.0)
    return (v2);

  if ((3.0 * vH) < 2.0)
    return (v1 + (v2 - v1) * ((2.0 / 3.0) - vH) * 6.0);

  return v1;
}

/**
Convert from HSL space into RGB space.
*/
vec3 HSLToRGB(vec3 HSL)
{
  vec3 RGB;
  if (HSL.y == 0.0)
    {
    // Gray
    RGB.r = HSL.z;
    RGB.g = HSL.z;
    RGB.b = HSL.z;
    }
  else
    {
    // Chromatic
    float v2;
    if (HSL.z < 0.5)
      v2 = HSL.z * (1.0 + HSL.y);
    else
      v2 = (HSL.z + HSL.y) - (HSL.y * HSL.z);

    float v1 = 2.0 * HSL.z - v2;

    RGB.r = Util(v1, v2, HSL.x + (1.0 / 3.0));
    RGB.g = Util(v1, v2, HSL.x);
    RGB.b = Util(v1, v2, HSL.x - (1.0 / 3.0));
    }

  return RGB.rgb;
}

void main()
{
  vec4 lic = texture2D(texLIC, gl_TexCoord[0].st);
  vec4 geomColor = texture2D(texGeomColors, gl_TexCoord[0].st);

  // depth is used to determine which fragment belong to us
  // and we can change
  float depth = texture2D(texVectors, gl_TexCoord[0].st).a;

  vec3 fragColorRGB;
  float valid;
  if (depth > 1.0e-3)
    {
    // we own it
    // shade LIC'ed geometry, or apply mask
    if (lic.g!=0.0)
      {
      // it's masked
      // apply fragment mask
      fragColorRGB = uMaskIntensity * uMaskColor + (1.0 - uMaskIntensity) * geomColor.rgb;
      valid = 0.0;
      }
    else
      {
      if (uScalarColorMode==0)
        {
        // blend with scalars
        fragColorRGB = lic.rrr * uLICIntensity + geomColor.rgb * (1.0 - uLICIntensity);
        }
      else
        {
        // multiply with scalars
        fragColorRGB = geomColor.rgb * clamp((uMapBias + lic.r), 0.0, 1.0);
        }
      if (lic.b != 0.0)
        {
        // didn't have the required guard pixels
        // don't consider it in min max estimation
        // for histpgram stretching
        valid = 0.0;
        }
      else
        {
        // ok to use in min/max estimates for histogram
        // stretching
        valid = 1.0;
        }
      }
    }
  else
    {
    // we don't own it
    // pass through scalars
    fragColorRGB = geomColor.rgb;
    valid = 0.0;
    }

  // if no further stages this texture is
  // copied to the screen
  gl_FragData[0] = vec4(fragColorRGB, geomColor.a);

  // if further stages, move to hsl space for contrast
  // enhancement. encoding valididty saves moving a texture to the cpu
  vec3 fragColorHSL = RGBToHSL(fragColorRGB);
  gl_FragData[1] = vec4(fragColorHSL, valid);
}
