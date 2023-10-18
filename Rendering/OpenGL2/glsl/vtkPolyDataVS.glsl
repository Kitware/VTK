//VTK::System::Dec

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

in vec4 vertexMC;

//VTK::CustomUniforms::Dec

// frag position in VC
//VTK::PositionVC::Dec

// optional normal declaration
//VTK::Normal::Dec

// extra lighting parameters
//VTK::Light::Dec

// Texture coordinates
//VTK::TCoord::Dec

// material property values
//VTK::Color::Dec

// clipping plane vars
//VTK::Clip::Dec

// camera and actor matrix values
//VTK::Camera::Dec

// Apple Bug
//VTK::PrimID::Dec

// Value raster
//VTK::ValuePass::Dec

// picking support
//VTK::Picking::Dec

// Surface with edges on GLES 3.0
//VTK::EdgesGLES30::Dec

// PointSize on GLES 3.0
//VTK::PointSizeGLES30::Dec

// LineWidth on GLES 3.0
//VTK::LineWidthGLES30::Dec

void main()
{
  //VTK::CustomBegin::Impl

  //VTK::PointSizeGLES30::Impl

  //VTK::Color::Impl

  //VTK::Normal::Impl

  //VTK::TCoord::Impl

  //VTK::Clip::Impl

  //VTK::PrimID::Impl

  //VTK::PositionVC::Impl

  //VTK::LineWidthGLES30::Impl

  //VTK::ValuePass::Impl

  //VTK::Light::Impl

  //VTK::Picking::Impl

  //VTK::CustomEnd::Impl

  //VTK::EdgesGLES30::Impl
}
