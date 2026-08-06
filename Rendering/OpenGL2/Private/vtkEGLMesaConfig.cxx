// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "Private/vtkEGLMesaConfig.h"
#include "vtkLogger.h"

//------------------------------------------------------------------------------
vtkEGLMesaConfig::vtkEGLMesaConfig()
{
  vtkLog(INFO, "vtkEGLMesaConfig created for Mesa offscreen rendering");
  this->OnscreenRendering = false;
}

//------------------------------------------------------------------------------
void vtkEGLMesaConfig::CreateContext(EGLContext& context, EGLDisplay display, EGLConfig config)
{
  vtkLog(TRACE, "vtkEGLMesaConfig::CreateContext - Requesting OpenGL 3.2 Core Profile");

  const EGLint attribs[] = { EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 2, EGL_NONE };

  eglBindAPI(EGL_OPENGL_API);

  context = eglCreateContext(display, config, EGL_NO_CONTEXT, attribs);
  if (context == EGL_NO_CONTEXT)
  {
    vtkLog(ERROR, "Failed to create EGL context. eglGetError=" << eglGetError());
  }
}

//------------------------------------------------------------------------------
void vtkEGLMesaConfig::CreateWindowSurface(
  EGLSurface& surface, EGLDisplay display, EGLConfig config, int width, int height)
{
  // Some driver configurations require an actual PBuffer surface even when
  // using a headless EGL platform. Prefer a true surfaceless context if the
  // config does not advertise pbuffer support, otherwise create a PBuffer.
  EGLint surfaceType = 0;
  eglGetConfigAttrib(display, config, EGL_SURFACE_TYPE, &surfaceType);

  if (surfaceType & EGL_PBUFFER_BIT)
  {
    vtkLog(TRACE,
      "vtkEGLMesaConfig::CreateWindowSurface - PBuffer config detected, creating PBuffer surface.");
    const EGLint pbufferAttribs[] = { EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE };
    surface = eglCreatePbufferSurface(display, config, pbufferAttribs);
    if (surface == EGL_NO_SURFACE)
    {
      vtkLog(WARNING, "Failed to create EGL PBuffer surface. Falling back to surfaceless mode.");
      surface = EGL_NO_SURFACE;
    }
    else
    {
      vtkLog(TRACE, "vtkEGLMesaConfig::CreateWindowSurface - Created EGL PBuffer surface.");
    }
  }
  else
  {
    vtkLog(TRACE,
      "vtkEGLMesaConfig::CreateWindowSurface - Surfaceless mode, bypassing surface creation.");
    surface = EGL_NO_SURFACE;
  }
}
