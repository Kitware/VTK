//VTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataWideLineGS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
  // compute the lines direction
  vec2 normal = normalize(
    gl_in[1].gl_Position.xy/gl_in[1].gl_Position.w -
    gl_in[0].gl_Position.xy/gl_in[0].gl_Position.w);

  // rotate 90 degrees
  normal = vec2(-1.0*normal.y,normal.x);

  //VTK::Normal::Start

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
      gl_in[i].gl_Position.xy + (lineWidthNVC*normal)*((j+1)%2 - 0.5)*gl_in[i].gl_Position.w,
      gl_in[i].gl_Position.z,
      gl_in[i].gl_Position.w);
    EmitVertex();
    }
  EndPrimitive();
}
