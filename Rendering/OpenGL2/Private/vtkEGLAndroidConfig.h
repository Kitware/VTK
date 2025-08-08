// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkEGLAndroidConfig_h
#define vtkEGLAndroidConfig_h

#include "Private/vtkEGLConfig.h"
#include "vtkABINamespace.h"

#include <android/native_window.h>

VTK_ABI_NAMESPACE_BEGIN

/**
 * @brief vtkEGLAndroidConfig
 *
 * This class implements the EGLConfig interface for Android. It supports only
 * onscreen rendering.
 */
class vtkEGLAndroidConfig : public vtkEGLConfig
{
public:
  /**
   * Constructor for vtkEGLAndroidConfig.
   * Override the default value of the OnscreenRendering to true.
   */
  vtkEGLAndroidConfig();

  ///@{
  /**
   * Implementation of vtkEGLConfig
   */
  [[nodiscard]] void* GetDisplay() override { return EGL_NO_DISPLAY; }
  [[nodiscard]] EGLenum GetPlatform() override { return EGL_PLATFORM_DEVICE_EXT; }
  void CreateContext(EGLContext& context, EGLDisplay display, EGLConfig config) override;
  ///@}

  /**
   * Create the window surface.
   */
  void CreateWindowSurface(
    EGLSurface& surface, EGLDisplay display, EGLConfig config, int width, int height) override;

  /**
   * As only onscreen rendering is supported, this method is a noop and the onscreenRendering
   * member is set to true in the constructor.
   */
  void SetOnscreenRendering([[maybe_unused]] bool onscreenRendering) override
  { /*noop*/
  }

private:
  EGLNativeWindowType Window;
};

VTK_ABI_NAMESPACE_END
#endif
