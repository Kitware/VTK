// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkCompositeZPassShader_fs.glsl
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

// Fragment shader used by the composite z render pass.

#version 110

uniform sampler2D depth;

void main(void)
{
  vec2 tcoord=gl_TexCoord[0].st;
  gl_FragDepth=texture2D(depth,tcoord).x;
}
