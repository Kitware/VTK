//VTK::System::Dec

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause


// all variables that represent positions or directions have a suffix
// indicating the coordinate system they are in. The possible values are
// MC - Model Coordinates
// WC - WC world coordinates
// VC - View Coordinates
// DC - Display Coordinates
in vec4 vertexDC;
in vec3 scalarColor;
in float depthArray;
in float attenuationArray;

out float fdepth;
out float fattenuation;
out vec3 fcolor;

void main()
{
  fcolor = scalarColor;
  fdepth = depthArray;
  fattenuation = attenuationArray;
  gl_Position = vertexDC;
}
