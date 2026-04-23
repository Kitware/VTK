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

// Texture coordinates
//VTK::TCoord::Dec

// picking support
//VTK::Picking::Dec

// clipping plane vars
//VTK::Clip::Dec

// the output of this shader
//VTK::Output::Dec

//VTK::Layout::Dec

void main()
{
  //VTK::Normal::Start

  for (int i = 0; i < VERT_COUNT; i++)
  {
    //VTK::PrimID::Impl

    //VTK::Clip::Impl

    //VTK::Color::Impl

    //VTK::Normal::Impl

    //VTK::TCoord::Impl

    //VTK::Picking::Impl

    // VC position of this fragment
    //VTK::PositionVC::Impl

    gl_Position = gl_in[i].gl_Position;

    EmitVertex();
  }
  EndPrimitive();
}
