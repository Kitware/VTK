//VTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyData2DFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

uniform int PrimitiveIDOffset;

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
