// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkEGLMesaConfig_h
#define vtkEGLMesaConfig_h

#include "Private/vtkEGLConfig.h"
#include "vtkABINamespace.h"
#include "vtkSetGet.h" // for vtkNotUsed

VTK_ABI_NAMESPACE_BEGIN

/**
 * @brief vtkEGLMesaConfig
 *
 * This class is used to setup the EGL configuration for Mesa offscreen rendering.
 * It supports offscreen rendering using EGL_MESA_platform_surfaceless extension,
 * with optional software rendering via EGL_MESA_device_software when available.
 *
 * Recommended setup:
 * export GALLIUM_DRIVER=llvmpipe
 * export EGL_PLATFORM=surfaceless
 *
 * @a vtkEGLRenderWindowInternals
 */
class vtkEGLMesaConfig : public vtkEGLConfig
{
public:
  vtkEGLMesaConfig();

  ///@{
  /**
   * Implementation of vtkEGLConfig
   */
  [[nodiscard]] void* GetDisplay() override { return nullptr; }
  [[nodiscard]] EGLenum GetPlatform() override { return EGL_PLATFORM_SURFACELESS_MESA; }
  void CreateContext(EGLContext& context, EGLDisplay display, EGLConfig config) override;
  ///@}

  /**
   * Create a surfaceless surface (EGL_NO_SURFACE).
   */
  void CreateWindowSurface(
    EGLSurface& surface, EGLDisplay display, EGLConfig config, int width, int height) override;

  /**
   * As only offscreen rendering is supported, this method is a noop and the onscreenRendering
   * member is set to false in the constructor.
   */
  void SetOnscreenRendering(bool vtkNotUsed(onscreenRendering)) override { /*noop*/ }
};

VTK_ABI_NAMESPACE_END

#endif
