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
// Template for the polydata mappers geometry shader

uniform int PrimitiveIDOffset;

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

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform vec4 vpDims;
out vec4 edgeEqn[3];
uniform float lineWidth;

//VTK::Edges::Dec

void main()
{
  //VTK::Normal::Start

  vec2 pos[4];

  for (int i = 0; i < 3; i++)
  {
    pos[i] = gl_in[i].gl_Position.xy/gl_in[i].gl_Position.w;
    pos[i] = pos[i]*vec2(0.5) + vec2(0.5);
    pos[i] = pos[i]*vpDims.zw + vpDims.xy;
  }
  pos[3] = pos[0];

  float ccw = sign(cross(vec3(pos[1] - pos[0], 0.0), vec3(pos[2] - pos[0], 0.0)).z);

  for (int i = 0; i < 3; i++)
  {
    vec2 tmp = normalize(pos[i+1] - pos[i]);
    tmp = ccw*vec2(-tmp.y, tmp.x);
    float d = dot(pos[i], tmp);
    edgeEqn[i] = vec4(tmp.x, tmp.y, 0.0, -d);
  }

  vec2 offsets[3];

  offsets[0] = edgeEqn[2].xy + edgeEqn[0].xy;
  offsets[0] = -0.5*normalize(offsets[0])*lineWidth;
  offsets[0] /= vpDims.zw;

  offsets[1] = edgeEqn[0].xy + edgeEqn[1].xy;
  offsets[1] = -0.5*normalize(offsets[1])*lineWidth;
  offsets[1] /= vpDims.zw;

  offsets[2] = edgeEqn[1].xy + edgeEqn[2].xy;
  offsets[2] = -0.5*normalize(offsets[2])*lineWidth;
  offsets[2] /= vpDims.zw;

  //VTK::Edges::Impl

  for (int i = 0; i < 3; i++)
    {
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

    // gl_Position = gl_in[i].gl_Position;

    gl_Position = gl_in[i].gl_Position;
    gl_Position.xy = gl_Position.xy + offsets[i]*gl_Position.w;

    EmitVertex();
    }
  EndPrimitive();
}
