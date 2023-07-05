//VTK::System::Dec

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

//VTK::Output::Dec

in vec3 fcolor;
in float fdepth;
in float fattenuation;

void main()
{
  // the following exp is done in the fragment shader
  // because linear interpolation (from the VS) of the resulting
  // value would not match the exp of the interpolated
  // source values
  float opacity = 1.0 - exp(-1.0*fattenuation*fdepth);


  gl_FragData[0] =  vec4(fcolor,opacity);

  if (gl_FragData[0].a <= 0.0)
    {
    discard;
    }
}
