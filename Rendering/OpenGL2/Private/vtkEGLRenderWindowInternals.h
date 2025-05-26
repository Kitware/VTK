// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkEGLRenderWindowInternals_h
#define vtkEGLRenderWindowInternals_h

#include "Private/vtkEGLConfig.h"
#include "vtkOpenGLRenderer.h"

#include "vtkglad/include/glad/egl.h"

VTK_ABI_NAMESPACE_BEGIN

/**
 * Internal class used to store and control EGL state.
 *
 * @a vtkEGLRenderWindow
 */
class VTKRENDERINGOPENGL2_EXPORT vtkEGLRenderWindowInternals
{
public:
  /**
   * Return the EGLDisplay.
   */
  [[nodiscard]] EGLDisplay GetDisplay() const { return this->Display; }

  /**
   * Return the EGLSurface.
   */
  [[nodiscard]] EGLDisplay GetSurface() const { return this->Surface; }

  ///@{
  /**
   * Get/Set the EGLContext
   */
  [[nodiscard]] EGLContext GetContext() const { return this->Context; }
  void SetContext(EGLContext context) { this->Context = context; }
  ///@}

  ///@{
  /**
   * Get/Set the EGLWindow
   */
  [[nodiscard]] EGLNativeWindowType GetWindow() const { return this->Window; }
  void SetWindow(EGLNativeWindowType window) { this->Window = window; }
  ///@}

  /**
   * Ensure to call the SwapBuffers() on the correct display and surface.
   */
  void SwapBuffer();

  /**
   * Get the number of devices (graphic cards) from the system.
   */
  [[nodiscard]] int GetNumberOfDevices();

  /**
   * Set the wanted device for display if the device is available.
   * Return true if it found a valid device, false otherwiise.
   */
  [[nodiscard]] bool SetDeviceAsDisplay(int deviceIndex);

  /**
   * Release EGL window.
   */
  void DestroyWindow();

  /**
   * Configure the current EGLWindow with the requested size. Ensure to create the window if its not
   * already the case.
   */
  void ConfigureWindow(int width, int height);

  /**
   * Setter used to store the device expected by the user.
   */
  void SetDeviceIndex(int deviceIndex) { this->DeviceIndex = deviceIndex; };

  /**
   * Setter to handle onscreen rendering.
   * See upper class for more information espcially regarding limitations regarding platforms.
   */
  void SetUseOnscreenRendering(bool useOnscreenRendering);

  /**
   * Get the current size of the EGLSurface.
   */
  void GetSizeFromSurface(int* size);

  /**
   * Release the EGL state.
   */
  void ReleaseCurrent();

  /**
   * Try to attach the EGL rendering context to the EGL surface.
   * Return false if it fails.
   */
  bool MakeCurrent();

  vtkEGLRenderWindowInternals();
  ~vtkEGLRenderWindowInternals() = default;

private:
  EGLNativeWindowType Window;
  EGLDisplay Display;
  EGLSurface Surface;
  EGLContext Context;

  int DeviceIndex = -1;
  bool UseOnscreenRendering = false;

  std::unique_ptr<vtkEGLConfig> Config;
};

VTK_ABI_NAMESPACE_END
#endif
