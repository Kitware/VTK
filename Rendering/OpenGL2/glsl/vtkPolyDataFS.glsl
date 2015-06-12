/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Template for the polydata mappers fragment shader

// The following line handle system declarations such a
// default precisions, or defining precisions to null
//VTK::System::Dec

uniform int PrimitiveIDOffset;

// VC position of this fragment
//VTK::PositionVC::Dec

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

// Apple Bug
//VTK::PrimID::Dec

void main()
{
  // Apple Bug
  //VTK::PrimID::Impl

  //VTK::Clip::Impl

  //VTK::Color::Impl

  // Generate the normal if we are not passed in one
  //VTK::Normal::Impl

  //VTK::Light::Impl

  //VTK::TCoord::Impl

  if (gl_FragData[0].a <= 0.0)
    {
    discard;
    }

  //VTK::DepthPeeling::Impl

  //VTK::Picking::Impl

}
