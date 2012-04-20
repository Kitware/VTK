// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkOpenGLPropertyDefaultPropFunc_fs.glsl
//
//  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
//  All rights reserved.
//  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
//
//     This software is distributed WITHOUT ANY WARRANTY; without even
//     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//     PURPOSE.  See the above copyright notice for more information.
//
// ============================================================================

// Default fragment shader used on property.

#version 110

uniform int useTexture;
uniform sampler2D texture;

void propFuncFS()
{
  if(useTexture==1)
    {
    gl_FragColor=gl_Color*texture2D(texture,gl_TexCoord[0].xy);
    }
  else
    {
    gl_FragColor=gl_Color;
    }
}
