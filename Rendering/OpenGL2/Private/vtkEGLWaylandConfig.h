// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkEGLWaylandConfig_h
#define vtkEGLWaylandConfig_h

#include "Private/vtkEGLConfig.h"

#include <wayland-client.h>
#include <wayland-egl.h>

#include "vtkABINamespace.h"

VTK_ABI_NAMESPACE_BEGIN

/**
 * @brief vtkEGLWaylandConfig
 *
 * This class is used to setup the EGL configuration for Wayland.
 *
 * This wayland implementation depends on xdg-shell and not wl-shell, see:
 * https://wayland-book.com/xdg-shell-basics.html
 *
 * @a vtkEGLConfig vtkEGLRenderWindowInternals
 */
class vtkEGLWaylandConfig : public vtkEGLConfig
{
public:
  /**
   * Setup wayland attributes
   */
  vtkEGLWaylandConfig();
  ~vtkEGLWaylandConfig();

  ///@{
  /**
   * Implementation of vtkEGLConfig
   */
  [[nodiscard]] void* GetDisplay() override { return this->Display; }
  [[nodiscard]] EGLenum GetPlatform() override { return EGL_PLATFORM_WAYLAND_EXT; }
  void CreateContext(EGLContext& context, EGLDisplay display, EGLConfig config) override;
  ///@}

  /**
   * Create the EGL window surface.
   * If the vtkEGLRenderWindow required offscreen rendering, the surface is created using a pbuffer
   * surface.
   *
   * Requires the wl_display and wl_surface to be set.
   */
  void CreateWindowSurface(
    EGLSurface& surface, EGLDisplay display, EGLConfig config, int width, int height) override;

private:
  struct wl_display* Display = nullptr;
  struct wl_surface* Surface = nullptr;
  wl_egl_window* Window = nullptr;

  struct xdg_surface* XdgSurface = nullptr;
  struct xdg_toplevel* XdgToplevel = nullptr;
};

VTK_ABI_NAMESPACE_END

#endif
