//VTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

in vec2 texCoord;

uniform sampler2D fluidZTexture;
uniform sampler2D fluidThicknessTexture;
uniform sampler2D opaqueRGBATexture;
uniform sampler2D opaqueZTexture;

// the output of this shader
//VTK::Output::Dec

void main()
{
  float fdepth = texture(fluidZTexture, texCoord).r;
  float fthick = texture(fluidThicknessTexture, texCoord).r;

  float odepth = texture(opaqueZTexture, texCoord).r;
  vec4 ocolor = texture(opaqueRGBATexture, texCoord);

  if (fdepth >= odepth) { discard; }

  // just a quick hack placeholder
  // water color, more opaque with thickness
  // more dark blue with thickness
  float accum = 1.0 - pow(0.98, fthick);
  vec4 wc = vec4(
    max(0.0, 0.3 - accum),
    max(0.0, 0.8 - accum),
    max(0.2, 1.0 - accum),
    min(1.0, accum));

  // gl_FragData[0] = wc;
  //gl_FragData[0] = vec4(fthick/100.0, fthick/100.0, fthick/100.0, 1.0);
  gl_FragData[0] = vec4(mix(ocolor.rgb, wc.rgb, wc.a), 1.0);
  gl_FragDepth = fdepth;
}
