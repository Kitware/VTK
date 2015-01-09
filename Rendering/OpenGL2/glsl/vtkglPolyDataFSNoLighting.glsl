/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkglPolyDataFSNoLighting.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// The following line handle system declarations such a
// default precisions, or defining precisions to null
//VTK::System::Dec

// VC positon of this fragment
//VTK::PositionVC::Dec

varying vec4 fcolor;

// Texture coordinates
//VTK::TCoord::Dec

//VTK::Color::Dec

// picking support
//VTK::Picking::Dec

// Depth Peeling Support
//VTK::DepthPeeling::Dec

// clipping plane vars
//VTK::Clip::Dec

void main()
{
  //VTK::Clip::Impl

  //VTK::Color::Impl
  // Note that the above will always define vec3 ambientColor, vec3 diffuseColor and float opacity

  gl_FragColor =  vec4(ambientColor + diffuseColor, opacity);
  //VTK::TCoord::Impl

  if (gl_FragColor.a <= 0.0)
    {
    discard;
    }

  //VTK::DepthPeeling::Impl

  //VTK::Picking::Impl

}
