//VTK::System::Dec

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Template for the polydata mappers geometry shader

// VC position of this fragment
//VTK::PositionVC::Dec

// primitiveID
//VTK::PrimID::Dec

// optional color passed in from the vertex shader, vertexColor
//VTK::Color::Dec

// optional surface normal declaration
//VTK::Normal::Dec

// extra lighting parameters
//VTK::Light::Dec

// Texture coordinates
//VTK::TCoord::Dec

// picking support
//VTK::Picking::Dec

// Depth Peeling Support
//VTK::DepthPeeling::Dec

// clipping plane vars
//VTK::Clip::Dec

// the output of this shader
//VTK::Output::Dec

uniform vec2 lineWidthNVC;

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

void main()
{
  // compute the line direction
  vec2 direction =
    gl_in[1].gl_Position.xy/gl_in[1].gl_Position.w -
    gl_in[0].gl_Position.xy/gl_in[0].gl_Position.w;
  float lineLength = length(direction);
  direction = direction / lineLength;

  // compute the normal by rotating by 90 degrees the direction
  vec2 normal = vec2(-1.0*direction.y,direction.x);

  vec2 _lineVertices[2];
  _lineVertices[0] = gl_in[0].gl_Position.xy;
  _lineVertices[1] = gl_in[1].gl_Position.xy;

  //VTK::Normal::Start

  // Make the line have a minimal screenspace size so it is always visible
  float screenDelta = length(lineWidthNVC) - lineLength;
  screenDelta = max(0.f, screenDelta);
  _lineVertices[0] = _lineVertices[0] - 0.25 * direction * screenDelta * gl_in[1].gl_Position.w;
  _lineVertices[1] = _lineVertices[1] + 0.25 * direction * screenDelta * gl_in[1].gl_Position.w;

  for (int j = 0; j < 4; j++)
  {
    int i = j/2;

    //VTK::PrimID::Impl

    //VTK::Clip::Impl

    //VTK::Color::Impl

    //VTK::Normal::Impl

    //VTK::Light::Impl

    //VTK::TCoord::Impl

    //VTK::DepthPeeling::Impl

    //VTK::Picking::Impl

    // VC position of this fragment
    //VTK::PositionVC::Impl

    gl_Position = vec4(
      _lineVertices[i] + (lineWidthNVC * normal) * ((j + 1) % 2 - 0.5) * gl_in[i].gl_Position.w,
      gl_in[i].gl_Position.z,
      gl_in[i].gl_Position.w);
    EmitVertex();
  }

  EndPrimitive();
}
