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
// this shader implements imposters in OpenGL for Spheres

in vec4 vertexMC;

// optional normal declaration
//VTK::Normal::Dec

// Texture coordinates
//VTK::TCoord::Dec

// material property values
//VTK::Color::Dec

// clipping plane vars
//VTK::Clip::Dec

// camera and actor matrix values
uniform mat4 MCVCMatrix;

// picking support
//VTK::Picking::Dec

void main()
{
  //VTK::Color::Impl

  //VTK::Normal::Impl

  //VTK::TCoord::Impl

  //VTK::Clip::Impl

  gl_Position = MCVCMatrix * vertexMC;

  //VTK::Picking::Impl
}
