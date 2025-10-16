// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWaylandRenderWindowInteractor
 * @brief   a Wayland event-driven interface for a RenderWindow
 *
 * vtkWaylandRenderWindowInteractor is a convenience object that provides event
 * bindings for a VTK render window on a Wayland-based desktop. It is designed
 * to work with a vtkWaylandHardwareWindow, which handles the window management.
 * This class is responsible for handling input events from the pointer and keyboard.
 *
 * @sa
 * vtkRenderWindowInteractor vtkWaylandHardwareWindow
 */

#ifndef vtkWaylandRenderWindowInteractor_h
#define vtkWaylandRenderWindowInteractor_h

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderingUIModule.h"     // For export macro
#include "vtkWaylandHardwareWindow.h" // The hardware window it depends on

// Forward declarations for Wayland and xkbcommon types.
struct wl_display;
struct wl_registry;
struct wl_surface;
struct wl_seat;
struct wl_pointer;
struct wl_keyboard;
struct wl_data_device_manager;
struct wl_data_device;
struct wl_data_offer;
struct xkb_context;
struct xkb_keymap;
struct xkb_state;

VTK_ABI_NAMESPACE_BEGIN

class vtkWaylandRenderWindowInteractorInternals;

class VTKRENDERINGUI_EXPORT vtkWaylandRenderWindowInteractor : public vtkRenderWindowInteractor
{
public:
  static vtkWaylandRenderWindowInteractor* New();
  vtkTypeMacro(vtkWaylandRenderWindowInteractor, vtkRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initializes the event handlers. This must be called after the
   * RenderWindow has been set and the hardware window has been created.
   */
  void Initialize() override;

  /**
   * Sets a flag to break the event loop.
   */
  void TerminateApp() override;

  /**
   * Process all pending Wayland events and timer events, then return.
   */
  void ProcessEvents() override;

  ///@{
  /**
   * Enable/Disable interactions.
   */
  void Enable() override;
  void Disable() override;
  ///@}

  // These members are accessed by static C-style callbacks and must be public.

  // Input device objects
  struct wl_seat* Seat = nullptr;
  struct wl_pointer* Pointer = nullptr;
  struct wl_keyboard* Keyboard = nullptr;
  uint32_t PointerSerial = 0;
  uint32_t KeyboardSerial = 0;

  // Keyboard handling via xkbcommon
  struct xkb_context* XkbContext = nullptr;
  struct xkb_keymap* XkbKeymap = nullptr;
  struct xkb_state* XkbState = nullptr;

protected:
  vtkWaylandRenderWindowInteractor();
  ~vtkWaylandRenderWindowInteractor() override;

  // Wayland connection and window handles, obtained from HardwareWindow
  struct wl_display* DisplayId = nullptr;
  struct wl_surface* WindowId = nullptr;
  struct wl_registry* Registry = nullptr; // Own registry for finding input devices

  // Drag and drop related (placeholder for future implementation)
  struct wl_data_device_manager* DataDeviceManager = nullptr;
  struct wl_data_device* DataDevice = nullptr;
  struct wl_data_offer* DndDataOffer = nullptr;

  // Internal state
  vtkWaylandRenderWindowInteractorInternals* Internal;

  ///@{
  /**
   * Wayland-specific internal timer methods.
   */
  int InternalCreateTimer(int timerId, int timerType, unsigned long duration) override;
  int InternalDestroyTimer(int platformTimerId) override;
  ///@}

  void FireTimers();

  /**
   * This starts the Wayland event loop and never returns.
   */
  void StartEventLoop() override;

  /**
   * Wait for new events on the Wayland display file descriptor.
   */
  void WaitForEvents();

  /**
   * Deallocate Wayland input resources.
   */
  void Finalize();

  /**
   * Request a render.
   * Overridden to schedule a render for the hardware window
   */
  void Render() override;

  friend class vtkWaylandRenderWindowInteractorInternals;

private:
  vtkWaylandRenderWindowInteractor(const vtkWaylandRenderWindowInteractor&) = delete;
  void operator=(const vtkWaylandRenderWindowInteractor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
