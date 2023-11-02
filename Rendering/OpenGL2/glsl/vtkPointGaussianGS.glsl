//VTK::System::Dec

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Template for the polydata mappers geometry shader

// primitiveID
//VTK::PrimID::Dec

// optional color passed in from the vertex shader, vertexColor
//VTK::Color::Dec

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform float boundScale;

in mat2 transformVCVSOutput[];
out vec2 offsetVCGSOutput;

// clipping plane vars
//VTK::Clip::Dec

// picking support
//VTK::Picking::Dec

void main()
{
  int i = 0;

  //VTK::PrimID::Impl

  //VTK::Clip::Impl

  //VTK::Color::Impl

  //VTK::Picking::Impl

  vec2 offsets[4] = vec2[](vec2(-boundScale, -boundScale),
                           vec2(boundScale, -boundScale),
                           vec2(-boundScale, boundScale),
                           vec2(boundScale, boundScale));

  vec4 posNDC = gl_in[0].gl_Position;
  posNDC = posNDC / posNDC.w;

  for (int i = 0; i < 4; i++)
  {
    vec2 offset = transformVCVSOutput[0] * offsets[i];
    offsetVCGSOutput = offsets[i];
    gl_Position = vec4(posNDC.xy + offset, posNDC.zw);
    EmitVertex();
  }

  EndPrimitive();
}
