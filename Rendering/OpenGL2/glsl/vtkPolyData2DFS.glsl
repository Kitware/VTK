//VTK::System::Dec

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

uniform int PrimitiveIDOffset;

// Custom uniforms
//VTK::CustomUniforms::Dec

// Texture coordinates
//VTK::TCoord::Dec

// Scalar coloring
//VTK::Color::Dec

// Depth Peeling
//VTK::DepthPeeling::Dec

// picking support
//VTK::Picking::Dec

// the output of this shader
//VTK::Output::Dec

// Apple Bug
//VTK::PrimID::Dec

void main()
{
  // Apple Bug
  //VTK::PrimID::Impl

  //VTK::Color::Impl
  //VTK::TCoord::Impl

  //VTK::DepthPeeling::Impl
  //VTK::Picking::Impl

  if (gl_FragData[0].a <= 0.0)
    {
    discard;
    }
}
