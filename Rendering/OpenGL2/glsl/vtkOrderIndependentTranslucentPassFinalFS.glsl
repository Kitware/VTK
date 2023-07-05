//VTK::System::Dec

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

in vec2 texCoord;

uniform sampler2D translucentRTexture;
uniform sampler2D translucentRGBATexture;

// the output of this shader
//VTK::Output::Dec

void main()
{
  vec4 t1Color = texture(translucentRGBATexture, texCoord);
  float t2Color = texture(translucentRTexture, texCoord).r;

  gl_FragData[0] = vec4(t1Color.rgb/max(t2Color,0.01), t1Color.a);
  // gl_FragData[0] = vec4(t1Color.a, t1Color.a, t1Color.a, 0.0);
  // gl_FragData[0] = vec4(t2Color, t2Color, t2Color, 0.0);
}
