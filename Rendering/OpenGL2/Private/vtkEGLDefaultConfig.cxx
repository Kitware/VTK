// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "Private/vtkEGLDefaultConfig.h"
#include "vtkLogger.h"

#include <string.h>

//------------------------------------------------------------------------------
vtkEGLDefaultConfig::vtkEGLDefaultConfig()
{
  // Set the default values for the EGL configuration
  this->OnscreenRendering = false;
}

//------------------------------------------------------------------------------
void vtkEGLDefaultConfig::CreateContext(EGLContext& context, EGLDisplay display, EGLConfig config)
{
#if VTK_OPENGL_USE_GLES
  const EGLint attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
#else
  const EGLint attribs[] = { EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 2, EGL_NONE };
#endif

  context = eglCreateContext(display, config, EGL_NO_CONTEXT, attribs);
}

//------------------------------------------------------------------------------
void vtkEGLDefaultConfig::CreateWindowSurface(
  EGLSurface& surface, EGLDisplay display, EGLConfig config, int width, int height)
{
  const EGLint surface_attribs[] = { EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE };
  surface = eglCreatePbufferSurface(display, config, surface_attribs);
}
