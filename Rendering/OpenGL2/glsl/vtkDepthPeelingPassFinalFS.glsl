//VTK::System::Dec

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

in vec2 texCoord;

uniform sampler2D translucentRGBATexture;
uniform sampler2D opaqueRGBATexture;
uniform sampler2D opaqueZTexture;

// the output of this shader
//VTK::Output::Dec

void main()
{
  vec4 t1Color = texture2D(translucentRGBATexture, texCoord);
  vec4 t2Color = texture2D(opaqueRGBATexture, texCoord);
  gl_FragData[0].a = t1Color.a +  (1.0-t1Color.a)*t2Color.a;
  gl_FragData[0].rgb = (t1Color.rgb*t1Color.a + t2Color.rgb*(1.0-t1Color.a));

  float depth = texture2D(opaqueZTexture, texCoord).x;
  gl_FragDepth = depth;
}
