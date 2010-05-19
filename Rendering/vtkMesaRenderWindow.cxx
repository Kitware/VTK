/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Probably not needed for abstract classes without new methods.
#define VTK_IMPLEMENT_MESA_CXX
#include "MangleMesaInclude/gl_mangle.h"
#include "MangleMesaInclude/gl.h"

#include "vtkOpenGLRenderWindow.h"
#include "vtkXMesaRenderWindow.h"

// Make sure vtkMesaRenderWindow is a copy of vtkOpenGLRenderWindow
// with vtkOpenGLRenderWindow replaced with vtkMesaRenderWindow
#define vtkOpenGLRenderWindow vtkMesaRenderWindow
#include "vtkOpenGLRenderWindow.cxx"
#undef vtkOpenGLRenderWindow
