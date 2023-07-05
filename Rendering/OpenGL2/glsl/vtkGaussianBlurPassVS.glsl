//VTK::System::Dec

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

in vec4 vertexMC;

in vec2 tcoordMC;
out vec2 tcoordVC;

void main()
{
  tcoordVC = tcoordMC;
  gl_Position = vertexMC;
}
