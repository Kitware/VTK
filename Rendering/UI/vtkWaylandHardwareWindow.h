/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWaylandHardwareWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWaylandHardwareWindow
 * @brief   represents a window in a Wayland GUI
 *
 * vtkWaylandHardwareWindow is a class for managing a Wayland window.
 * It uses the xdg-shell protocol for window management, which is the
 * current standard.
 */

#ifndef vtkWaylandHardwareWindow_h
#define vtkWaylandHardwareWindow_h

#include "vtkHardwareWindow.h"
#include "vtkRenderingUIModule.h" // For export macro

// Forward declarations for Wayland types to keep the header clean.
// The actual headers will be included in the .cxx file.
struct wl_array;
struct wl_callback;
struct wl_display;
struct wl_compositor;
struct wl_surface;
struct wl_registry;
struct wl_seat;
struct wl_pointer;
struct wl_shm;
struct xdg_wm_base;
struct xdg_surface;
struct xdg_toplevel;
struct zxdg_decoration_manager_v1;

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGUI_EXPORT vtkWaylandHardwareWindow : public vtkHardwareWindow
{
public:
  static vtkWaylandHardwareWindow* New();
  vtkTypeMacro(vtkWaylandHardwareWindow, vtkHardwareWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the Wayland display.
   */
  wl_display* GetDisplayId();

  /**
   * Get the Wayland surface. This is the core window object.
   */
  wl_surface* GetWindowId();

  // vtkHardwareWindow overrides
  void Create() override;
  void Destroy() override;

  void* GetGenericDisplayId() override;
  void* GetGenericWindowId() override;

  ///@{
  /**
   * Set the size of the window in pixels.
   * Note: With Wayland, this is a request to the compositor, which may or
   * may not be honored. The actual size will be provided via a configure event.
   */
  void SetSize(int, int) override;
  using vtkHardwareWindow::SetSize;
  ///@}

  ///@{
  /**
   * Set the position of the window.
   * Note: In Wayland, clients cannot set their own position. This is a no-op.
   */
  void SetPosition(int, int) override;
  using vtkHardwareWindow::SetPosition;
  ///@}

  /**
   * Set the name of the window (the title).
   */
  void SetWindowName(const char*) override;

  /**
   * Hide or Show the mouse cursor.
   * Note: This is a placeholder and not fully implemented. Cursor handling in
   * Wayland requires more complex SHM buffer management.
   */
  void HideCursor() override;
  void ShowCursor() override;

  /**
   * Change the shape of the cursor.
   * Note: This is a placeholder and not fully implemented.
   */
  void SetCurrentCursor(int) override;

  // Listener callbacks for Wayland events
  static void RegistryHandleGlobal(
    void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version);
  static void RegistryHandleGlobalRemove(void* data, wl_registry* registry, uint32_t name);

  static void XdgWmBaseHandlePing(void* data, xdg_wm_base* xdg_wm_base, uint32_t serial);

  static void XdgSurfaceHandleConfigure(void* data, xdg_surface* xdg_surface, uint32_t serial);

  static void XdgToplevelHandleConfigure(
    void* data, xdg_toplevel* xdg_toplevel, int32_t width, int32_t height, wl_array* states);
  static void XdgToplevelHandleClose(void* data, xdg_toplevel* xdg_toplevel);

  static void FrameHandleDone(void* data, wl_callback* callback, uint32_t time);

  // Request a redraw for the next frame.
  void ScheduleRedraw();

protected:
  vtkWaylandHardwareWindow();
  ~vtkWaylandHardwareWindow() override;

  // Wayland-specific members
  wl_display* DisplayId = nullptr;
  wl_registry* Registry = nullptr;
  wl_compositor* Compositor = nullptr;
  wl_surface* Surface = nullptr;
  wl_shm* Shm = nullptr;
  wl_seat* Seat = nullptr;
  wl_pointer* Pointer = nullptr;
  xdg_wm_base* XdgWmBase = nullptr;
  xdg_surface* XdgSurface = nullptr;
  xdg_toplevel* XdgToplevel = nullptr;
  zxdg_decoration_manager_v1* DecorationManager = nullptr;

  bool OwnDisplay = false;
  bool CursorHidden = false;

  wl_callback* FrameCallback = nullptr;
  bool RedrawPending = false;

private:
  vtkWaylandHardwareWindow(const vtkWaylandHardwareWindow&) = delete;
  void operator=(const vtkWaylandHardwareWindow&) = delete;

  // Internal state to track window configuration
  bool IsConfigured = false;
};

VTK_ABI_NAMESPACE_END
#endif // vtkWaylandHardwareWindow_h
