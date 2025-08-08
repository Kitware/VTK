// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkEGLConfig_h
#define vtkEGLConfig_h

#include "vtkglad/include/glad/egl.h"

#include "vtkABINamespace.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN

/**
 * @brief Base class for EGL configuration.
 *
 * This class is used to create EGL configurations for different platforms
 * (e.g., Android, Wayland). It provides a common interface for creating
 * EGL surfaces and contexts, as well as retrieving display information.
 */
class vtkEGLConfig
{
public:
  vtkEGLConfig() = default;
  virtual ~vtkEGLConfig() = default;

  /**
   * Set the onscreen rendering flag.
   * Default is false.
   */
  virtual void SetOnscreenRendering([[maybe_unused]] bool onscreenRendering);

  /**
   * Get the platform type.
   */
  [[nodiscard]] virtual EGLenum GetPlatform() = 0;

  /**
   * Get the display object.
   *
   * @note: return a void* as type is dependent on the platform, see eglGetPlatformDisplay.
   */
  [[nodiscard]] virtual void* GetDisplay() = 0;

  /**
   * Create an EGL window surface.
   */
  virtual void CreateWindowSurface(
    EGLSurface& surface, EGLDisplay display, EGLConfig config, int width, int height) = 0;

  /**
   * Create an EGL context.
   */
  virtual void CreateContext(EGLContext& context, EGLDisplay display, EGLConfig config) = 0;

protected:
  bool OnscreenRendering = false;
};
VTK_ABI_NAMESPACE_END
#endif
