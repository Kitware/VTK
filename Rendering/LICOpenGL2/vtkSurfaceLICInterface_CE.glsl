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

vec3 HSLToRGB(vec3 HSL)
{
  vec3 RGB;
  float v;
  float h = HSL.x;
  float sl = HSL.y;
  float l = HSL.z;

  v = (l <= 0.5) ? (l * (1.0 + sl)) : (l + sl - l * sl);
  if (v <= 0) {
    RGB = vec3(0.0,0.0,0.0);
  } else {
    float m;
    int sextant;
    float fract, vsf, mid1, mid2;

    m = l + l - v;
    h *= 6.0;
    sextant = int(h);
    fract = h - sextant;

    vsf = (v - m) * fract;
    mid1 = m + vsf;
    mid2 = v - vsf;
    switch (sextant) {
      case 0: RGB.r = v; RGB.g = mid1; RGB.b = m; break;
      case 1: RGB.r = mid2; RGB.g = v; RGB.b = m; break;
      case 2: RGB.r = m; RGB.g = v; RGB.b = mid1; break;
      case 3: RGB.r = m; RGB.g = mid2; RGB.b = v; break;
      case 4: RGB.r = mid1; RGB.g = m; RGB.b = v; break;
      case 5: RGB.r = v; RGB.g = m; RGB.b = mid2; break;
    }
  }
  return RGB;
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
