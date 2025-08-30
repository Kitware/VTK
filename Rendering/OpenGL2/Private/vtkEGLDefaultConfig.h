// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkEGLDefaultConfig_h
#define vtkEGLDefaultConfig_h

#include "Private/vtkEGLConfig.h"

#include "vtkABINamespace.h"

VTK_ABI_NAMESPACE_BEGIN

/**
 * @brief vtkEGLDefaultConfig
 *
 * This class is used to setup the EGL configuration for default builds.
 * It supports only offscreen rendering.
 *
 * @a vtkEGLRenderWindowInternals
 */
class vtkEGLDefaultConfig : public vtkEGLConfig
{
public:
  vtkEGLDefaultConfig();

  ///@{
  /**
   * Implementation of vtkEGLConfig
   */
  [[nodiscard]] void* GetDisplay() override { return EGL_NO_DISPLAY; }
  [[nodiscard]] EGLenum GetPlatform() override { return EGL_PLATFORM_DEVICE_EXT; }
  void CreateContext(EGLContext& context, EGLDisplay display, EGLConfig config) override;
  ///@}

  /**
   * Create a pbuffer surface.
   */
  void CreateWindowSurface(
    EGLSurface& surface, EGLDisplay display, EGLConfig config, int width, int height) override;

  /**
   * As only offscreen rendering is supported, this method is a noop and the onscreenRendering
   * member is set to false in the constructor.
   */
  void SetOnscreenRendering([[maybe_unused]] bool onscreenRendering) override
  { /*noop*/
  }
};

VTK_ABI_NAMESPACE_END

#endif
