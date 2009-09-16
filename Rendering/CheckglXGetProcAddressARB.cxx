/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CheckglXGetProcAddressARB.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

  Copyright 2005 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

// This is just a test program so that CMake can determine if
// glXGetProcAddressARB needs to be declared.

#define GLX_GLXEXT_LEGACY
#define GL_GLEXT_LEGACY
#include <GL/gl.h>
#include <GL/glx.h>

int main(int, char **)
{
  void (*FuncPointer)(void);

  FuncPointer = glXGetProcAddressARB((const GLubyte *)"glHelloWorldEXT");

  return 0;
}

