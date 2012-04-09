// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkOpenGLPropertyDefaultMain_vs.glsl
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

// This is the default vertex shader with the main() function. It is used when
// no main() is provided at the renderer level and some vertex shader is
// defined by the user on a property.

#version 110

void propFuncVS();

void main()
{
  propFuncVS();
}
