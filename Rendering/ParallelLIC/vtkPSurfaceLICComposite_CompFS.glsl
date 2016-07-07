//VTK::System::Dec
//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkPSurfaceLICComposite_CompFS.glsl
//
//  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
//  All rights reserved.
//  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
//
//     This software is distributed WITHOUT ANY WARRANTY; without even
//     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//     PURPOSE.  See the above copyright notice for more information.
//
//=========================================================================

// This shader composites for surface lic
// it expects float depth values encoded
// in alpha channel.

// The following line handles system declarations such as
// default precisions, or defining precisions to null
// the output of this shader
//VTK::Output::Dec

uniform sampler2D texData;

varying vec2 tcoordVC;

void main()
{
  vec4 newData = texture2D(texData, tcoordVC.st);
  float newDepth = newData.a;
  if (newDepth == 0.0)
    {
    discard;
    }
  gl_FragDepth = newDepth;
  gl_FragData[0] = newData;
}
