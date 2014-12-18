/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGL.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkOpenGL_h
#define vtkOpenGL_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkConfigure.h"

// To prevent gl.h to include glext.h provided by the system
#define GL_GLEXT_LEGACY
#if defined(__APPLE__) && (defined(VTK_USE_CARBON) || defined(VTK_USE_COCOA))
# include <OpenGL/gl.h> // Include OpenGL API.
#else
# include "vtkWindows.h" // Needed to include OpenGL header on Windows.
# include <GL/gl.h> // Include OpenGL API.
#endif

#endif
// VTK-HeaderTest-Exclude: vtkOpenGL.h
