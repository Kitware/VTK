// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkOpenGLPropertyDefaultMain_fs.glsl
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

// This is the default fragment shader with the main() function. It is used
// when no main() is provided at the renderer level and some fragment shader is
// defined by the user on a property. For example, the depth peeling algorithm
// defines its own main() function for the fragment shader.

#version 110

void propFuncFS();

void main()
{
  propFuncFS();
}
