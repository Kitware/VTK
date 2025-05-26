// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "Private/vtkEGLAndroidConfig.h"

//------------------------------------------------------------------------------
vtkEGLAndroidConfig::vtkEGLAndroidConfig()
  : Window(nullptr)
{
  this->OnscreenRendering = true;
}

//------------------------------------------------------------------------------
void vtkEGLAndroidConfig::CreateContext(EGLContext& context, EGLDisplay display, EGLConfig config)
{
  EGLint format = 0;
  /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
   * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
   * As soon as we picked a EGLConfig, we can safely reconfigure the
   * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
  eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

  ANativeWindow_setBuffersGeometry(this->Window, 0, 0, format);
  const EGLint contextES2[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
  context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextES2);
}

//------------------------------------------------------------------------------
void vtkEGLAndroidConfig::CreateWindow(
  EGLSurface& surface, EGLDisplay display, EGLConfig config, int width, int height)
{
  const EGLint surface_attribs[] = { EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE };

  surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)this->Window, NULL);
}
