// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkEGLRenderWindowInternals_h
#define vtkEGLRenderWindowInternals_h

#include "Private/vtkEGLConfig.h"
#include "vtkOpenGLRenderer.h"

#include "vtk_glad.h" // OpenGL functions
#include "vtkglad/include/glad/egl.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkRecti;
class vtkRendererCollection;

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

  /**
   * Set the EGLWindow
   */
  void SetWindow(EGLNativeWindowType window) { this->Window = window; }

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

  /**
   * Return true if the current build is using Mesa software rendering backend.
   * For more details, see the CMake option VTK_USE_MESA_SOFTWARE_RENDERING.
   */
  bool IsMesaSoftwareRenderer() const;

private:
  /**
   * Initialize a software EGL display via EGL_MESA_device_software or
   * EGL_PLATFORM_SURFACELESS_MESA.
   */
  bool TryInitializeMesaSoftware(EGLint& major, EGLint& minor);

  /**
   * Initialize a hardware EGL display using deviceIndex, fallback devices, or eglGetDisplay.
   */
  bool TryInitializeHardware(int deviceIndex, EGLint& major, EGLint& minor);

  /**
   * Load EGL extensions and bind EGL_OPENGL_API (skipped on Android).
   */
  bool FinalizeDisplaySetup(EGLint major, EGLint minor);

  /**
   * Create a custom framebuffer object for offscreen rendering as
   * Mesa surfaceless mode does not provide a default drawing surface.
   */
  void InitializeOffscreenFramebuffer();

  /**
   * Destroy the offscreen framebuffer linked to SurfacelessFBO, ColorTexture and DepthBuffer. This
   * is required to avoid memory leaks when the EGL context is destroyed.
   */
  void DestroyOffscreenFramebuffer();

  EGLNativeWindowType Window;
  EGLDisplay Display;
  EGLSurface Surface;
  EGLContext Context;

  int DeviceIndex = -1;
  bool UseOnscreenRendering = false;

  std::unique_ptr<vtkEGLConfig> Config;

  unsigned int Height = 0;
  unsigned int Width = 0;

  unsigned int SurfacelessFBO = 0;
  unsigned int ColorTexture = 0;
  unsigned int DepthBuffer = 0;
};

VTK_ABI_NAMESPACE_END
#endif
