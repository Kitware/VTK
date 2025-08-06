// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "Private/vtkEGLWaylandConfig.h"
#include "vtkLogger.h"

#include "xdg-shell-protocols.h"

#include <string.h>

namespace
{
/**
 * This namespace contains the global Wayland compositor and xdg_wm_base objects.
 * They are initialized in the global registry handler and used to create
 * surfaces and toplevel windows.
 * see : https://wayland-book.com/registry/binding.html
 */
struct wl_compositor* Compositor = nullptr;
struct xdg_wm_base* XdgWmBase = nullptr;

static void global_registry_handler(
  void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
  if (!strcmp(interface, "wl_compositor"))
  {
    Compositor =
      static_cast<wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 1));
  }
  else if (!strcmp(interface, "xdg_wm_base"))
  {
    XdgWmBase =
      static_cast<xdg_wm_base*>(wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
  }
}

static void xdg_surface_configure(void* data, struct xdg_surface* xdg_surface, uint32_t serial)
{
  xdg_surface_ack_configure(xdg_surface, serial);
}

static const struct wl_registry_listener registry_listener = { global_registry_handler, nullptr };

static const struct xdg_surface_listener surface_listener = {
  .configure = xdg_surface_configure,
};
} // namespace

//------------------------------------------------------------------------------
vtkEGLWaylandConfig::vtkEGLWaylandConfig()
{
  this->Display = wl_display_connect(NULL);
  if (this->Display == NULL)
  {
    vtkLog(ERROR, "Can't connect to display");
    return;
  }

  struct wl_registry* registry = wl_display_get_registry(this->Display);
  wl_registry_add_listener(registry, &registry_listener, NULL);
  wl_display_roundtrip(this->Display);

  if (::Compositor == NULL || ::XdgWmBase == NULL)
  {
    vtkLog(ERROR, "Can't find compositor or xdg_wm_base");
    return;
  }
}

//------------------------------------------------------------------------------
vtkEGLWaylandConfig::~vtkEGLWaylandConfig()
{
  if (this->XdgToplevel)
  {
    xdg_toplevel_destroy(this->XdgToplevel);
    this->XdgToplevel = nullptr;
  }
  if (this->XdgSurface)
  {
    xdg_surface_destroy(this->XdgSurface);
    this->XdgSurface = nullptr;
  }
  if (this->Surface)
  {
    wl_surface_destroy(this->Surface);
    this->Surface = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkEGLWaylandConfig::CreateContext(EGLContext& context, EGLDisplay display, EGLConfig config)
{
  const EGLint contextES2[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
  context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextES2);
}

//------------------------------------------------------------------------------
void vtkEGLWaylandConfig::CreateWindowSurface(
  EGLSurface& surface, EGLDisplay display, EGLConfig config, int width, int height)
{
  if (::Compositor == nullptr || ::XdgWmBase == nullptr)
  {
    vtkLog(ERROR, "Can't find compositor or xdg_wm_base, cannot create window without them");
    return;
  }

  this->Surface = wl_compositor_create_surface(::Compositor);
  this->XdgSurface = xdg_wm_base_get_xdg_surface(::XdgWmBase, this->Surface);

  xdg_surface_add_listener(this->XdgSurface, &::surface_listener, NULL);

  this->XdgToplevel = xdg_surface_get_toplevel(this->XdgSurface);
  xdg_toplevel_set_title(this->XdgToplevel, "VTK Wayland Window");
  wl_surface_commit(this->Surface);

  wl_display_roundtrip(this->Display);
  wl_display_dispatch_pending(this->Display);

  this->Window = wl_egl_window_create(this->Surface, width, height);

  if (this->OnscreenRendering)
  {
    surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)this->Window, nullptr);
  }
  else
  {
    const EGLint surface_attribs[] = { EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE };
    surface = eglCreatePbufferSurface(display, config, surface_attribs);
  }
}
