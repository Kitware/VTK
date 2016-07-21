//VTK::System::Dec

//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkSurfaceLICMapper_CE.glsl
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

// color contrast enhance stage implemented via histogram stretching
// on lightness channel. if the min and max are tweaked it can generate
// out-of-range values these will be clamped in 0 to 1

// the output of this shader
//VTK::Output::Dec

uniform sampler2D texGeomColors; // scalars + lighting
uniform sampler2D texLIC;        // image lic, mask
uniform sampler2D texHSLColors;  // hsla colors

uniform float     uLMin;         // min lightness over all fragments
uniform float     uLMaxMinDiff;  // max - min lightness over all fragments

varying vec2 tcoordVC;

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
  // lookup hsl color , mask
  vec4 fragColor = texture2D(texHSLColors, tcoordVC.st);

  // don't modify masked fragments (masked => lic.g==1)
  vec4 lic = texture2D(texLIC, tcoordVC.st);
  if (lic.g==0.0)
    {
    // normalize lightness channel
    fragColor.z = clamp((fragColor.z - uLMin)/uLMaxMinDiff, 0.0, 1.0);
    }

  // back into rgb space
  fragColor.rgb = HSLToRGB(fragColor.xyz);

  // add alpha
  vec4 geomColor = texture2D(texGeomColors, tcoordVC.st);
  fragColor.a = geomColor.a;

  gl_FragData[0] = fragColor;
}
