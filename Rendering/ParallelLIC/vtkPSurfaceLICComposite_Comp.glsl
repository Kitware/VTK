//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkSurfaceLICComposite_fs1.glsl
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

#version 110

uniform sampler2D texData;

void main()
{
  vec4 newData = texture2D(texData, gl_TexCoord[0].st);
  float newDepth = newData.a;
  if (newDepth == 0.0)
    {
    discard;
    }
  gl_FragDepth = newDepth;
  gl_FragData[0] = newData;
}
