//VTK::System::Dec

// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkCompositeZPassFS.glsl
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

// the output of this shader
//VTK::Output::Dec

varying vec2 tcoordVC;
uniform sampler2D depth;

void main(void)
{
  gl_FragDepth = texture2D(depth,tcoordVC).x;
}
