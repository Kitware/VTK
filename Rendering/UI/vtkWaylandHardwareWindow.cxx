// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWaylandHardwareWindow.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"

// Wayland specific headers
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-util.h>

#include "xdg-decoration-protocol.h" // Generated from xdg-decoration-unstable-v1.xml
#include "xdg-shell-protocol.h"      // Generated from xdg-shell.xml

#include <iostream> // for cerr
#include <string.h> // for strcmp

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWaylandHardwareWindow);

//------------------------------------------------------------------------------------------------
// Static Wayland listener callbacks
//------------------------------------------------------------------------------------------------

// Listener for the wl_registry
void vtkWaylandHardwareWindow::RegistryHandleGlobal(
  void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
  vtkWaylandHardwareWindow* self = static_cast<vtkWaylandHardwareWindow*>(data);
  if (strcmp(interface, wl_compositor_interface.name) == 0)
  {
    self->Compositor =
      static_cast<wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 4));
  }
  else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
  {
    self->XdgWmBase =
      static_cast<xdg_wm_base*>(wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
  }
  // Look for the decoration manager global
  else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0)
  {
    self->DecorationManager = static_cast<zxdg_decoration_manager_v1*>(
      wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, 1));
  }
  else if (strcmp(interface, wl_shm_interface.name) == 0)
  {
    self->Shm = static_cast<wl_shm*>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
  }
  else if (strcmp(interface, wl_seat_interface.name) == 0)
  {
    self->Seat = static_cast<wl_seat*>(wl_registry_bind(registry, name, &wl_seat_interface, 7));
  }
}

//------------------------------------------------------------------------------------------------
void vtkWaylandHardwareWindow::RegistryHandleGlobalRemove(
  void* data, wl_registry* registry, uint32_t name)
{
  // This space is intentionally left blank. In a more complex application,
  // we would handle the removal of global objects.
  (void)data;
  (void)registry;
  (void)name;
}

//------------------------------------------------------------------------------------------------
static const wl_registry_listener registry_listener = {
  .global = vtkWaylandHardwareWindow::RegistryHandleGlobal,
  .global_remove = vtkWaylandHardwareWindow::RegistryHandleGlobalRemove,
};

//------------------------------------------------------------------------------------------------
// Listener for xdg_wm_base (the window manager)
void vtkWaylandHardwareWindow::XdgWmBaseHandlePing(
  void* data, xdg_wm_base* xdg_wm_base, uint32_t serial)
{
  xdg_wm_base_pong(xdg_wm_base, serial);
}

//------------------------------------------------------------------------------------------------
static const xdg_wm_base_listener xdg_wm_base_listener = {
  .ping = vtkWaylandHardwareWindow::XdgWmBaseHandlePing,
};

//------------------------------------------------------------------------------------------------
// Add a listener for the decoration object.
// The compositor uses this to tell us which decoration mode is active.
static void decoration_handle_configure(
  void* data, zxdg_toplevel_decoration_v1* decoration, uint32_t mode)
{
  // We requested server-side, but the compositor makes the final decision.
  // A full CSD implementation would check the mode here and draw decorations if needed.
}

//------------------------------------------------------------------------------------------------
static const struct zxdg_toplevel_decoration_v1_listener decoration_listener = {
  .configure = decoration_handle_configure,
};

//------------------------------------------------------------------------------------------------
// Listener for xdg_surface (the window surface itself)
void vtkWaylandHardwareWindow::XdgSurfaceHandleConfigure(
  void* data, xdg_surface* xdg_surface, uint32_t serial)
{
  vtkWaylandHardwareWindow* self = static_cast<vtkWaylandHardwareWindow*>(data);
  xdg_surface_ack_configure(xdg_surface, serial);

  // A configure event marks that the surface is ready to be drawn on.
  if (!self->IsConfigured)
  {
    self->IsConfigured = true;
  }
}

//------------------------------------------------------------------------------------------------
static const xdg_surface_listener xdg_surface_listener = {
  .configure = vtkWaylandHardwareWindow::XdgSurfaceHandleConfigure,
};

//------------------------------------------------------------------------------------------------
static const struct wl_callback_listener frame_listener = {
  .done = vtkWaylandHardwareWindow::FrameHandleDone,
};

// Listener for xdg_toplevel (the main window properties)
void vtkWaylandHardwareWindow::XdgToplevelHandleConfigure(
  void* data, xdg_toplevel* xdg_toplevel, int32_t width, int32_t height, wl_array* states)
{
  vtkWaylandHardwareWindow* self = static_cast<vtkWaylandHardwareWindow*>(data);
  // if width and height are 0, we can choose our own size.
  int current_width = (width > 0) ? width : self->GetSize()[0];
  int current_height = (height > 0) ? height : self->GetSize()[1];

  // Update our size if it has changed.
  if (self->Size[0] != current_width || self->Size[1] != current_height)
  {
    self->Size[0] = current_width;
    self->Size[1] = current_height;
    self->Modified(); // Notify that the window has been modified
  }
}

//------------------------------------------------------------------------------------------------
void vtkWaylandHardwareWindow::XdgToplevelHandleClose(void* data, xdg_toplevel* xdg_toplevel)
{
  vtkWaylandHardwareWindow* self = static_cast<vtkWaylandHardwareWindow*>(data);
  // The compositor is telling us to close the window.
  self->InvokeEvent(vtkCommand::DeleteEvent, nullptr);
}

static const xdg_toplevel_listener xdg_toplevel_listener = {
  .configure = vtkWaylandHardwareWindow::XdgToplevelHandleConfigure,
  .close = vtkWaylandHardwareWindow::XdgToplevelHandleClose,
};

//------------------------------------------------------------------------------------------------
// vtkWaylandHardwareWindow class implementation
//------------------------------------------------------------------------------------------------
vtkWaylandHardwareWindow::vtkWaylandHardwareWindow()
{
  // Default values
  this->OwnDisplay = false;
  this->CursorHidden = false;
  this->Mapped = false;
  this->IsConfigured = false;
  this->Platform = "Wayland";
}

//------------------------------------------------------------------------------------------------
vtkWaylandHardwareWindow::~vtkWaylandHardwareWindow()
{
  this->Destroy();
}

//------------------------------------------------------------------------------------------------
void vtkWaylandHardwareWindow::Create()
{
  // Step 1: Connect to the Wayland display
  this->DisplayId = wl_display_connect(nullptr);
  if (!this->DisplayId)
  {
    vtkErrorMacro("Failed to connect to Wayland display.");
    return;
  }
  this->OwnDisplay = true;

  // Step 2: Get the registry and bind to global interfaces
  this->Registry = wl_display_get_registry(this->DisplayId);
  wl_registry_add_listener(this->Registry, &registry_listener, this);

  // This call dispatches events and blocks until the server has processed
  // all requests, ensuring we get the global objects.
  wl_display_roundtrip(this->DisplayId);

  // Check if we got the necessary globals
  if (!this->Compositor || !this->XdgWmBase)
  {
    vtkErrorMacro("Failed to bind to required Wayland globals (compositor or xdg_wm_base).");
    this->Destroy();
    return;
  }

  // Add the ping listener for the window manager
  xdg_wm_base_add_listener(this->XdgWmBase, &xdg_wm_base_listener, this);

  // Check if we found the decoration manager. It is optional.
  if (!this->DecorationManager)
  {
    vtkWarningMacro("Compositor does not support xdg-decoration protocol. "
                    "Window decorations will not be available.");
  }

  // Step 3: Create the core surface
  this->Surface = wl_compositor_create_surface(this->Compositor);
  if (!this->Surface)
  {
    vtkErrorMacro("Failed to create Wayland surface.");
    this->Destroy();
    return;
  }
  // Schedule an initial draw
  this->ScheduleRedraw();

  // Step 4: Create the xdg_surface and toplevel window
  this->XdgSurface = xdg_wm_base_get_xdg_surface(this->XdgWmBase, this->Surface);
  xdg_surface_add_listener(this->XdgSurface, &xdg_surface_listener, this);

  this->XdgToplevel = xdg_surface_get_toplevel(this->XdgSurface);
  xdg_toplevel_add_listener(this->XdgToplevel, &xdg_toplevel_listener, this);

  // If the decoration manager exists, request server-side decorations.
  if (this->DecorationManager)
  {
    struct zxdg_toplevel_decoration_v1* decoration =
      zxdg_decoration_manager_v1_get_toplevel_decoration(
        this->DecorationManager, this->XdgToplevel);
    zxdg_toplevel_decoration_v1_add_listener(decoration, &decoration_listener, this);
    zxdg_toplevel_decoration_v1_set_mode(decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
  }

  // Set the window title if it exists
  if (this->WindowName)
  {
    this->SetWindowName(this->WindowName);
  }

  // Commit the surface state to make the window appear
  wl_surface_commit(this->Surface);

  // Dispatch events until the surface is configured
  while (!this->IsConfigured)
  {
    wl_display_dispatch(this->DisplayId);
  }

  this->Mapped = true;
}

//------------------------------------------------------------------------------------------------
void vtkWaylandHardwareWindow::Destroy()
{
  if (this->FrameCallback)
  {
    wl_callback_destroy(this->FrameCallback);
    this->FrameCallback = nullptr;
  }
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
  if (this->XdgWmBase)
  {
    xdg_wm_base_destroy(this->XdgWmBase);
    this->XdgWmBase = nullptr;
  }
  if (this->Surface)
  {
    wl_surface_destroy(this->Surface);
    this->Surface = nullptr;
  }
  if (this->Compositor)
  {
    wl_compositor_destroy(this->Compositor);
    this->Compositor = nullptr;
  }
  if (this->Seat)
  {
    // Note: we don't destroy the seat, just release our reference
    wl_seat_release(this->Seat);
    this->Seat = nullptr;
  }
  if (this->Shm)
  {
    wl_shm_destroy(this->Shm);
    this->Shm = nullptr;
  }
  if (this->Registry)
  {
    wl_registry_destroy(this->Registry);
    this->Registry = nullptr;
  }
  if (this->DisplayId && this->OwnDisplay)
  {
    wl_display_flush(this->DisplayId);
    wl_display_disconnect(this->DisplayId);
    this->DisplayId = nullptr;
  }
  this->Mapped = false;
}

//------------------------------------------------------------------------------------------------
void vtkWaylandHardwareWindow::SetSize(int width, int height)
{
  if (this->Size[0] != width || this->Size[1] != height)
  {
    this->Superclass::SetSize(width, height);
    if (this->Interactor)
    {
      this->Interactor->SetSize(width, height);
    }
    if (this->XdgToplevel)
    {
      // Note: This is a suggestion. The compositor will respond with a
      // configure event that contains the actual size.
      // EGL/Vulkan surfaces will need to be resized in response to that event.
    }
    this->Modified();
  }
}

//------------------------------------------------------------------------------------------------
void vtkWaylandHardwareWindow::SetPosition(int x, int y)
{
  // Wayland does not allow clients to set their absolute position.
  // This is a design choice for security and compositor flexibility.
  vtkDebugMacro("SetPosition is a no-op on Wayland.");
  this->Superclass::SetPosition(x, y);
}

//------------------------------------------------------------------------------------------------
void vtkWaylandHardwareWindow::SetWindowName(const char* name)
{
  this->Superclass::SetWindowName(name);
  if (this->XdgToplevel)
  {
    xdg_toplevel_set_title(this->XdgToplevel, name);
  }
}

//------------------------------------------------------------------------------------------------
void vtkWaylandHardwareWindow::HideCursor()
{
  this->CursorHidden = true;
  vtkWarningMacro("HideCursor() is not fully implemented for Wayland yet.");
  // A full implementation would involve:
  // 1. Getting a wl_pointer from the wl_seat.
  // 2. Creating a 1x1 transparent wl_buffer using wl_shm.
  // 3. Calling wl_pointer_set_cursor with the transparent buffer.
}

//------------------------------------------------------------------------------------------------
void vtkWaylandHardwareWindow::ShowCursor()
{
  this->CursorHidden = false;
  vtkWarningMacro("ShowCursor() is not fully implemented for Wayland yet.");
  // A full implementation would unset the custom cursor.
}

//------------------------------------------------------------------------------------------------
void vtkWaylandHardwareWindow::SetCurrentCursor(int shape)
{
  this->Superclass::SetCurrentCursor(shape);
  vtkWarningMacro("SetCurrentCursor() is not implemented for Wayland yet.");
  // A full implementation would require loading cursor themes or using wl_shm
  // to create custom cursor surfaces.
}

//------------------------------------------------------------------------------------------------
wl_display* vtkWaylandHardwareWindow::GetDisplayId()
{
  return this->DisplayId;
}

//------------------------------------------------------------------------------------------------
wl_surface* vtkWaylandHardwareWindow::GetWindowId()
{
  return this->Surface;
}

//------------------------------------------------------------------------------------------------
void* vtkWaylandHardwareWindow::GetGenericDisplayId()
{
  return reinterpret_cast<void*>(this->DisplayId);
}

//------------------------------------------------------------------------------------------------
void* vtkWaylandHardwareWindow::GetGenericWindowId()
{
  return reinterpret_cast<void*>(this->Surface);
}

//------------------------------------------------------------------------------------------------
void vtkWaylandHardwareWindow::FrameHandleDone(void* data, wl_callback* callback, uint32_t time)
{
  vtkWaylandHardwareWindow* self = static_cast<vtkWaylandHardwareWindow*>(data);

  // The previous frame callback is now invalid, destroy it.
  if (callback)
  {
    wl_callback_destroy(callback);
  }
  self->FrameCallback = nullptr;
  self->RedrawPending = false;

  // The compositor is ready for a new frame, so render it now.
  if (self->Interactor && self->Interactor->GetRenderWindow())
  {
    self->Interactor->GetRenderWindow()->Render();
  }
}

//------------------------------------------------------------------------------------------------
void vtkWaylandHardwareWindow::ScheduleRedraw()
{
  // If a redraw isn't already pending, request one.
  if (!this->RedrawPending && this->Surface)
  {
    this->FrameCallback = wl_surface_frame(this->Surface);
    wl_callback_add_listener(this->FrameCallback, &frame_listener, this);
    this->RedrawPending = true;
    wl_surface_commit(this->Surface);
  }
}
//------------------------------------------------------------------------------------------------
void vtkWaylandHardwareWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DisplayId: " << this->DisplayId << "\n";
  os << indent << "Surface: " << this->Surface << "\n";
  os << indent << "OwnDisplay: " << (this->OwnDisplay ? "Yes" : "No") << "\n";
}

VTK_ABI_NAMESPACE_END
