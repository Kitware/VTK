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

#include "vtkRenderingOpenGLConfigure.h" // For VTK_USE_COCOA

#if !defined(VTK_LEGACY_SILENT)
#ifdef _MSC_VER
#pragma message(                                                                                   \
  "vtkOpenGL.h is deprecated. vtk_glew.h should be used if you require OpenGL headers.")
#else
#warning "vtkOpenGL.h is deprecated. vtk_glew.h should be used if you require OpenGL headers."
#endif
#endif

// Must be included before `gl.h` due to glew.
#include "vtkOpenGLError.h"

// To prevent gl.h to include glext.h provided by the system
#define GL_GLEXT_LEGACY
#if defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
#include <ES3/gl.h> // Include OpenGL API.
#elif TARGET_OS_MAC
#include <OpenGL/gl.h> // Include OpenGL API.
#endif
#else
#include "vtkWindows.h" // Needed to include OpenGL header on Windows.
#include <GL/gl.h>      // Include OpenGL API.
#endif

#endif
// VTK-HeaderTest-Exclude: vtkOpenGL.h
