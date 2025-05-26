// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "Private/vtkEGLXConfig.h"
#include "vtkLogger.h"

#include <string.h>

//------------------------------------------------------------------------------
vtkEGLXConfig::vtkEGLXConfig()
{
  // Set the default values for the EGL configuration
  this->OnscreenRendering = false;
}

//------------------------------------------------------------------------------
void vtkEGLXConfig::CreateContext(EGLContext& context, EGLDisplay display, EGLConfig config)
{
  const EGLint contextES2[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
  context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextES2);
}

//------------------------------------------------------------------------------
void vtkEGLXConfig::CreateWindowSurface(
  EGLSurface& surface, EGLDisplay display, EGLConfig config, int width, int height)
{
  const EGLint surface_attribs[] = { EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE };
  surface = eglCreatePbufferSurface(display, config, surface_attribs);
}
