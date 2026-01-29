// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWaylandRenderWindowInteractor.h"

// Must be included first to avoid conflicts
#include <vtksys/SystemTools.hxx>

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"

// Wayland and helper libraries
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <map>
#include <poll.h>
#include <set>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

//-------------------------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWaylandRenderWindowInteractor);

// The internal helper class for managing timers and multiple interactor instances.
// This is largely unchanged.
class vtkWaylandRenderWindowInteractorInternals
{
public:
  vtkWaylandRenderWindowInteractorInternals()
  {
    this->DisplayConnection = -1;
    this->TimerIdCount = 1;
  }
  ~vtkWaylandRenderWindowInteractorInternals() = default;

  //-----------------------------------------------------------------------------------------------
  int CreateLocalTimer(unsigned long duration)
  {
    int id = this->TimerIdCount++;
    this->LocalToTimer[id].duration = duration;
    gettimeofday(&this->LocalToTimer[id].lastFire, nullptr);
    return id;
  }

  //-----------------------------------------------------------------------------------------------
  void DestroyLocalTimer(int id) { this->LocalToTimer.erase(id); }

  //-----------------------------------------------------------------------------------------------
  bool GetTimeToNextTimer(int& timeout)
  {
    bool useTimeout = false;
    uint64_t delta = 0;
    if (!this->LocalToTimer.empty())
    {
      timeval ctv;
      gettimeofday(&ctv, nullptr);
      delta = VTK_UNSIGNED_INT_MAX;
      for (auto& timer : this->LocalToTimer)
      {
        uint64_t duration = timer.second.duration * 1000; // microsecs
        uint64_t elapsed = (ctv.tv_sec - timer.second.lastFire.tv_sec) * 1000000 + ctv.tv_usec -
          timer.second.lastFire.tv_usec;
        delta = duration > elapsed ? std::min<uint64_t>(duration - elapsed, delta) : 0;
        useTimeout = true;
      }
    }
    timeout = static_cast<int>(delta / 1000);
    return useTimeout;
  }

  //-----------------------------------------------------------------------------------------------
  void FireTimers(vtkWaylandRenderWindowInteractor* rwi)
  {
    if (!this->LocalToTimer.empty())
    {
      timeval ctv;
      gettimeofday(&ctv, nullptr);
      std::vector<LocalToTimerType::value_type> timers(
        this->LocalToTimer.begin(), this->LocalToTimer.end());
      for (auto& timer : timers)
      {
        int64_t delta = (ctv.tv_sec - timer.second.lastFire.tv_sec) * 1000000 + ctv.tv_usec -
          timer.second.lastFire.tv_usec;
        if (delta / 1000 >= static_cast<int64_t>(timer.second.duration))
        {
          int timerId = rwi->GetVTKTimerId(timer.first);
          if (timerId != 0)
          {
            rwi->InvokeEvent(vtkCommand::TimerEvent, &timerId);
            if (rwi->IsOneShotTimer(timerId))
            {
              this->DestroyLocalTimer(timer.first);
            }
            else
            {
              auto it = this->LocalToTimer.find(timer.first);
              if (it != this->LocalToTimer.end())
              {
                it->second.lastFire = ctv;
              }
            }
          }
        }
      }
    }
  }

  //-----------------------------------------------------------------------------------------------
  struct vtkWaylandRenderWindowInteractorTimer
  {
    unsigned long duration;
    timeval lastFire;
  };

  static std::set<vtkWaylandRenderWindowInteractor*> Instances;
  bool LoopDone = false;
  int DisplayConnection;

private:
  int TimerIdCount;
  typedef std::map<int, vtkWaylandRenderWindowInteractorTimer> LocalToTimerType;
  LocalToTimerType LocalToTimer;
};

std::set<vtkWaylandRenderWindowInteractor*> vtkWaylandRenderWindowInteractorInternals::Instances;

//-------------------------------------------------------------------------------------------------
// --- Wayland Listeners (Static Callbacks) ---
// Note: Windowing related listeners (xdg_surface, xdg_toplevel) have been removed
// as they are now handled by vtkWaylandHardwareWindow.

// -- Pointer Listener --
static void handle_pointer_enter(void* data, struct wl_pointer* pointer, uint32_t serial,
  struct wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy)
{
  auto* rwi = static_cast<vtkWaylandRenderWindowInteractor*>(data);
  rwi->PointerSerial = serial;
  rwi->SetEventInformationFlipY(
    wl_fixed_to_int(sx), wl_fixed_to_int(sy), rwi->GetControlKey(), rwi->GetShiftKey());
  rwi->InvokeEvent(vtkCommand::EnterEvent, nullptr);
}

//-------------------------------------------------------------------------------------------------
static void handle_pointer_leave(
  void* data, struct wl_pointer* pointer, uint32_t serial, struct wl_surface* surface)
{
  auto* rwi = static_cast<vtkWaylandRenderWindowInteractor*>(data);
  rwi->PointerSerial = serial;
  rwi->InvokeEvent(vtkCommand::LeaveEvent, nullptr);
}

//-------------------------------------------------------------------------------------------------
static void handle_pointer_motion(
  void* data, struct wl_pointer* pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
  auto* rwi = static_cast<vtkWaylandRenderWindowInteractor*>(data);
  rwi->SetEventInformationFlipY(
    wl_fixed_to_int(sx), wl_fixed_to_int(sy), rwi->GetControlKey(), rwi->GetShiftKey());
  rwi->InvokeEvent(vtkCommand::MouseMoveEvent, nullptr);
}

//-------------------------------------------------------------------------------------------------
static void handle_pointer_button(void* data, struct wl_pointer* pointer, uint32_t serial,
  uint32_t time, uint32_t button, uint32_t state)
{
  auto* rwi = static_cast<vtkWaylandRenderWindowInteractor*>(data);
  rwi->PointerSerial = serial;

  if (state == WL_POINTER_BUTTON_STATE_PRESSED)
  {
    // BTN_LEFT=0x110, BTN_RIGHT=0x111, BTN_MIDDLE=0x112
    switch (button)
    {
      case 0x110:
        rwi->InvokeEvent(vtkCommand::LeftButtonPressEvent, nullptr);
        break;
      case 0x112:
        rwi->InvokeEvent(vtkCommand::MiddleButtonPressEvent, nullptr);
        break;
      case 0x111:
        rwi->InvokeEvent(vtkCommand::RightButtonPressEvent, nullptr);
        break;
    }
  }
  else // WL_POINTER_BUTTON_STATE_RELEASED
  {
    switch (button)
    {
      case 0x110:
        rwi->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, nullptr);
        break;
      case 0x112:
        rwi->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent, nullptr);
        break;
      case 0x111:
        rwi->InvokeEvent(vtkCommand::RightButtonReleaseEvent, nullptr);
        break;
    }
  }
}

//-------------------------------------------------------------------------------------------------
static void handle_pointer_axis(
  void* data, struct wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
  auto* rwi = static_cast<vtkWaylandRenderWindowInteractor*>(data);
  if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
  {
    if (wl_fixed_to_double(value) < 0)
    {
      rwi->InvokeEvent(vtkCommand::MouseWheelForwardEvent, nullptr);
    }
    else
    {
      rwi->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, nullptr);
    }
  }
}

//-------------------------------------------------------------------------------------------------
// Add a handler for the pointer frame event to prevent a crash.
static void handle_pointer_frame(void* data, struct wl_pointer* pointer)
{
  // This function is intentionally left blank.
  // The frame event is a hint that a series of pointer events is complete.
  // We don't need to act on it, but the handler must exist.
  (void)data;
  (void)pointer;
}

//-------------------------------------------------------------------------------------------------
// Add a handler for the pointer axis_source event to prevent a crash.
static void handle_pointer_axis_source(void* data, struct wl_pointer* pointer, uint32_t axis_source)
{
  // This function is intentionally left blank.
  // We don't need to handle the axis source, but the listener must be present.
  (void)data;
  (void)pointer;
  (void)axis_source;
}

//-------------------------------------------------------------------------------------------------
// Add a handler for the pointer axis_stop event to prevent a crash.
static void handle_pointer_axis_stop(
  void* data, struct wl_pointer* pointer, uint32_t time, uint32_t axis)
{
  // This function is intentionally left blank.
  // We don't need to handle the axis stop event, but the listener must be present.
  (void)data;
  (void)pointer;
  (void)time;
  (void)axis;
}

//-------------------------------------------------------------------------------------------------
// Add a handler for the pointer axis_discrete event.
static void handle_pointer_axis_discrete(
  void* data, struct wl_pointer* pointer, uint32_t axis, int32_t discrete)
{
  auto* rwi = static_cast<vtkWaylandRenderWindowInteractor*>(data);
  if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
  {
    // A negative value indicates scrolling forward (away from the user).
    if (discrete < 0)
    {
      rwi->InvokeEvent(vtkCommand::MouseWheelForwardEvent, nullptr);
    }
    else if (discrete > 0)
    {
      rwi->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, nullptr);
    }
  }
}

//-------------------------------------------------------------------------------------------------
// Add all handlers to the listener struct
static const struct wl_pointer_listener pointer_listener = {
  .enter = handle_pointer_enter,
  .leave = handle_pointer_leave,
  .motion = handle_pointer_motion,
  .button = handle_pointer_button,
  .axis = handle_pointer_axis,
  .frame = handle_pointer_frame,
  .axis_source = handle_pointer_axis_source,
  .axis_stop = handle_pointer_axis_stop,
  .axis_discrete = handle_pointer_axis_discrete,
};

//-------------------------------------------------------------------------------------------------
// -- Keyboard Listener --
static void handle_keyboard_keymap(
  void* data, struct wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size)
{
  auto* rwi = static_cast<vtkWaylandRenderWindowInteractor*>(data);
  if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
  {
    close(fd);
    return;
  }
  char* map_str = (char*)mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (map_str == MAP_FAILED)
  {
    close(fd);
    return;
  }
  rwi->XkbKeymap = xkb_keymap_new_from_string(
    rwi->XkbContext, map_str, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
  munmap(map_str, size);
  close(fd);
  if (rwi->XkbKeymap)
  {
    rwi->XkbState = xkb_state_new(rwi->XkbKeymap);
  }
}

//-------------------------------------------------------------------------------------------------
static void handle_keyboard_enter(void* data, struct wl_keyboard* keyboard, uint32_t serial,
  struct wl_surface* surface, struct wl_array* keys)
{
  auto* rwi = static_cast<vtkWaylandRenderWindowInteractor*>(data);
  rwi->KeyboardSerial = serial;
}

//-------------------------------------------------------------------------------------------------
static void handle_keyboard_leave(
  void* data, struct wl_keyboard* keyboard, uint32_t serial, struct wl_surface* surface)
{
  auto* rwi = static_cast<vtkWaylandRenderWindowInteractor*>(data);
  rwi->KeyboardSerial = serial;
}

//-------------------------------------------------------------------------------------------------
static void handle_keyboard_key(void* data, struct wl_keyboard* keyboard, uint32_t serial,
  uint32_t time, uint32_t key, uint32_t state)
{
  auto* rwi = static_cast<vtkWaylandRenderWindowInteractor*>(data);
  rwi->KeyboardSerial = serial;

  if (!rwi->XkbState)
    return;

  // Wayland gives scancodes; add 8 for xkbcommon
  uint32_t xkb_key = key + 8;
  // First, update the xkb state machine with the key press/release.
  // This is crucial for correctly resolving the character and keysym later,
  // as it handles latched/locked modifiers and layout shifts.
  xkb_state_update_key(rwi->XkbState, xkb_key, (enum xkb_key_direction)state);
  xkb_keysym_t sym = xkb_state_key_get_one_sym(rwi->XkbState, xkb_key);
  char keyChar = 0;
  // Use the stateless xkb_keysym_to_utf8 function. This is more robust as the state
  // (e.g., Shift modifier) has already been baked into the keysym 'sym'.
  xkb_keysym_to_utf8(sym, &keyChar, 8);
  // xkb_state_key_get_utf8(rwi->XkbState, xkb_key, &keyChar, 1);
  char keySymStr[64];
  xkb_keysym_get_name(sym, keySymStr, sizeof(keySymStr));

  // The repeat count must be set to 1 for the event to be processed,
  // matching the behavior of the vtkXRenderWindowInteractor.
  rwi->SetEventInformationFlipY(rwi->GetEventPosition()[0], rwi->GetEventPosition()[1],
    rwi->GetControlKey(), rwi->GetShiftKey(), keyChar, 1, keySymStr);
  rwi->SetAltKey(rwi->GetAltKey());

  if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
  {
    rwi->InvokeEvent(vtkCommand::KeyPressEvent, nullptr);
    rwi->InvokeEvent(vtkCommand::CharEvent, nullptr);
  }
  else // WL_KEYBOARD_KEY_STATE_RELEASED
  {
    rwi->InvokeEvent(vtkCommand::KeyReleaseEvent, nullptr);
  }
}

//-------------------------------------------------------------------------------------------------
static void handle_keyboard_modifiers(void* data, struct wl_keyboard* keyboard, uint32_t serial,
  uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
  auto* rwi = static_cast<vtkWaylandRenderWindowInteractor*>(data);
  if (rwi->XkbState)
  {
    xkb_state_update_mask(rwi->XkbState, mods_depressed, mods_latched, mods_locked, 0, 0, group);

    auto ctrl =
      xkb_state_mod_name_is_active(rwi->XkbState, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE);
    auto shift =
      xkb_state_mod_name_is_active(rwi->XkbState, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE);
    auto alt =
      xkb_state_mod_name_is_active(rwi->XkbState, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE);

    rwi->SetControlKey(ctrl);
    rwi->SetShiftKey(shift);
    rwi->SetAltKey(alt);
  }
}

//-------------------------------------------------------------------------------------------------
// Add a handler for the key repeat info event to prevent a crash.
static void handle_keyboard_repeat_info(
  void* data, struct wl_keyboard* keyboard, int32_t rate, int32_t delay)
{
  // This function is intentionally left blank.
  // We don't handle key repeat info directly, but the listener must be present.
  (void)data;
  (void)keyboard;
  (void)rate;
  (void)delay;
}

//-------------------------------------------------------------------------------------------------
// Update the listener struct to include the new handler.
static const struct wl_keyboard_listener keyboard_listener = {
  .keymap = handle_keyboard_keymap,
  .enter = handle_keyboard_enter,
  .leave = handle_keyboard_leave,
  .key = handle_keyboard_key,
  .modifiers = handle_keyboard_modifiers,
  .repeat_info = handle_keyboard_repeat_info,
};

//-------------------------------------------------------------------------------------------------
// -- Seat Listener --
static void handle_seat_capabilities(void* data, struct wl_seat* seat, uint32_t capabilities)
{
  auto* rwi = static_cast<vtkWaylandRenderWindowInteractor*>(data);
  if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && !rwi->Pointer)
  {
    rwi->Pointer = wl_seat_get_pointer(seat);
    wl_pointer_add_listener(rwi->Pointer, &pointer_listener, rwi);
  }
  else if (!(capabilities & WL_SEAT_CAPABILITY_POINTER) && rwi->Pointer)
  {
    wl_pointer_release(rwi->Pointer);
    rwi->Pointer = nullptr;
  }

  if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && !rwi->Keyboard)
  {
    rwi->Keyboard = wl_seat_get_keyboard(seat);
    wl_keyboard_add_listener(rwi->Keyboard, &keyboard_listener, rwi);
  }
  else if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && rwi->Keyboard)
  {
    wl_keyboard_release(rwi->Keyboard);
    rwi->Keyboard = nullptr;
  }
}

//-------------------------------------------------------------------------------------------------
// Add a handler for the seat name event to prevent a crash.
static void handle_seat_name(void* data, struct wl_seat* seat, const char* name)
{
  // This function is intentionally left blank.
  // We don't need to handle the seat name, but the listener must be present
  // to avoid a NULL callback, which would crash the application.
  (void)data;
  (void)seat;
  (void)name;
}

//-------------------------------------------------------------------------------------------------
// Update the listener struct to include the new handler.
static const struct wl_seat_listener seat_listener = {
  .capabilities = handle_seat_capabilities,
  .name = handle_seat_name,
};

//-------------------------------------------------------------------------------------------------
// -- Registry Listener --
// This is now simplified to only look for the input seat.
static void handle_registry_global(
  void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
  auto* rwi = static_cast<vtkWaylandRenderWindowInteractor*>(data);
  if (strcmp(interface, wl_seat_interface.name) == 0)
  {
    rwi->Seat = (wl_seat*)wl_registry_bind(registry, name, &wl_seat_interface, 7);
    wl_seat_add_listener(rwi->Seat, &seat_listener, rwi);
  }
}

//-------------------------------------------------------------------------------------------------
static void handle_registry_global_remove(void* data, struct wl_registry* registry, uint32_t name)
{
  // This space is intentionally left blank
}

//-------------------------------------------------------------------------------------------------
static const struct wl_registry_listener registry_listener = {
  .global = handle_registry_global,
  .global_remove = handle_registry_global_remove,
};

//-------------------------------------------------------------------------------------------------
// --- Class Implementation ---

vtkWaylandRenderWindowInteractor::vtkWaylandRenderWindowInteractor()
{
  this->Internal = new vtkWaylandRenderWindowInteractorInternals;
  this->XkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
}

//-------------------------------------------------------------------------------------------------
vtkWaylandRenderWindowInteractor::~vtkWaylandRenderWindowInteractor()
{
  this->Finalize();
  if (this->XkbContext)
  {
    xkb_context_unref(this->XkbContext);
  }
  delete this->Internal;
}

//-------------------------------------------------------------------------------------------------
void vtkWaylandRenderWindowInteractor::Initialize()
{
  if (this->Initialized)
  {
    return;
  }

  if (!this->RenderWindow)
  {
    vtkErrorMacro("No renderer window defined!");
    return;
  }

  // Get the info we need from the RenderWindow
  this->RenderWindow->Start();
  this->RenderWindow->End();

  // Get the hardware window and ensure it's a Wayland window
  vtkWaylandHardwareWindow* hw_win =
    vtkWaylandHardwareWindow::SafeDownCast(this->RenderWindow->GetHardwareWindow());
  if (!hw_win)
  {
    vtkErrorMacro("Interactor requires a vtkWaylandHardwareWindow.");
    return;
  }

  // Get display and surface handles from the hardware window
  this->DisplayId = hw_win->GetDisplayId(); //
  this->WindowId = hw_win->GetWindowId();   //

  if (!this->DisplayId || !this->WindowId)
  {
    vtkErrorMacro("Could not get Wayland display or surface from hardware window.");
    return;
  }

  this->Initialized = 1;
  vtkWaylandRenderWindowInteractorInternals::Instances.insert(this);

  // Get the registry to find the input devices (seat)
  this->Registry = wl_display_get_registry(this->DisplayId);
  wl_registry_add_listener(this->Registry, &registry_listener, this);

  // Process events to ensure we bind the seat
  wl_display_roundtrip(this->DisplayId);

  if (!this->Seat)
  {
    vtkErrorMacro("Failed to get Wayland seat for input.");
    this->Finalize();
    return;
  }

  // Set up the file descriptor for poll()
  this->Internal->DisplayConnection = wl_display_get_fd(this->DisplayId);

  this->Enable();

  // Set initial size from render window
  int* size = this->RenderWindow->GetSize();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

//-------------------------------------------------------------------------------------------------
void vtkWaylandRenderWindowInteractor::TerminateApp()
{
  this->Done = true;
}

//-------------------------------------------------------------------------------------------------
void vtkWaylandRenderWindowInteractor::ProcessEvents()
{
  if (!this->DisplayId)
    return;

  // Dispatch any pending Wayland events, which will trigger callbacks
  while (wl_display_prepare_read(this->DisplayId) != 0)
  {
    wl_display_dispatch_pending(this->DisplayId);
  }
  wl_display_read_events(this->DisplayId);
  wl_display_dispatch_pending(this->DisplayId);

  // Fire any VTK timers that are due
  for (auto rwi : vtkWaylandRenderWindowInteractorInternals::Instances)
  {
    if (!rwi->Done)
    {
      rwi->FireTimers();
    }
    this->Internal->LoopDone = this->Internal->LoopDone || rwi->Done;
  }
}

//-------------------------------------------------------------------------------------------------
void vtkWaylandRenderWindowInteractor::WaitForEvents()
{
  if (!this->DisplayId)
    return;

  bool useTimeout = false;
  int soonestTimer = 0;

  for (auto rwi : vtkWaylandRenderWindowInteractorInternals::Instances)
  {
    if (rwi->Done)
      continue;
    int t;
    bool haveTimer = rwi->Internal->GetTimeToNextTimer(t);
    if (haveTimer)
    {
      if (!useTimeout)
      {
        useTimeout = true;
        soonestTimer = t;
      }
      else
      {
        soonestTimer = std::min(soonestTimer, t);
      }
    }
  }

  // Flush any buffered requests to the compositor
  wl_display_flush(this->DisplayId);

  struct pollfd pfd;
  pfd.fd = this->Internal->DisplayConnection;
  pfd.events = POLLIN;
  pfd.revents = 0;

  int timeout = useTimeout ? soonestTimer : -1;
  poll(&pfd, 1, timeout);
}

//-------------------------------------------------------------------------------------------------
void vtkWaylandRenderWindowInteractor::StartEventLoop()
{
  if (!this->DisplayId || !this->WindowId)
  {
    vtkWarningMacro(<< "Cannot start event loop without Wayland display or window.");
    return;
  }
  for (auto rwi : vtkWaylandRenderWindowInteractorInternals::Instances)
  {
    rwi->Done = false;
  }

  auto& loopDone = this->Internal->LoopDone;
  loopDone = false;
  do
  {
    this->ProcessEvents();
    if (!loopDone)
    {
      this->WaitForEvents();
    }
  } while (!loopDone);
}

//-------------------------------------------------------------------------------------------------
void vtkWaylandRenderWindowInteractor::Finalize()
{
  vtkWaylandRenderWindowInteractorInternals::Instances.erase(this);

  // Destroy Wayland objects created by this interactor
  if (this->XkbState)
    xkb_state_unref(this->XkbState);
  if (this->XkbKeymap)
    xkb_keymap_unref(this->XkbKeymap);
  if (this->Keyboard)
    wl_keyboard_release(this->Keyboard);
  if (this->Pointer)
    wl_pointer_release(this->Pointer);
  if (this->Seat)
    wl_seat_release(this->Seat);
  if (this->Registry)
    wl_registry_destroy(this->Registry);

  // Do not disconnect from display, as it's owned by the HardwareWindow
  this->DisplayId = nullptr;
  this->Internal->DisplayConnection = -1;
  this->Initialized = false;
  this->Enabled = false;
}

//-------------------------------------------------------------------------------------------------
void vtkWaylandRenderWindowInteractor::Enable()
{
  if (this->Enabled)
    return;
  this->Enabled = 1;
  this->Modified();
}

//-------------------------------------------------------------------------------------------------
void vtkWaylandRenderWindowInteractor::Disable()
{
  if (!this->Enabled)
    return;
  this->Enabled = 0;
  this->Modified();
}

//-------------------------------------------------------------------------------------------------
void vtkWaylandRenderWindowInteractor::FireTimers()
{
  if (this->GetEnabled())
  {
    this->Internal->FireTimers(this);
  }
}

//-------------------------------------------------------------------------------------------------
int vtkWaylandRenderWindowInteractor::InternalCreateTimer(int, int, unsigned long duration)
{
  duration = (duration > 0 ? duration : this->TimerDuration);
  return this->Internal->CreateLocalTimer(duration);
}

//-------------------------------------------------------------------------------------------------
int vtkWaylandRenderWindowInteractor::InternalDestroyTimer(int platformTimerId)
{
  this->Internal->DestroyLocalTimer(platformTimerId);
  return 1;
}

//-------------------------------------------------------------------------------------------------
void vtkWaylandRenderWindowInteractor::Render()
{
  // Instead of rendering immediately, we ask the hardware window to schedule
  // a render for the next frame callback.
  if (this->HardwareWindow)
  {
    vtkWaylandHardwareWindow* hw_win = vtkWaylandHardwareWindow::SafeDownCast(this->HardwareWindow);
    if (hw_win)
    {
      hw_win->ScheduleRedraw();
    }
  }
}

//-------------------------------------------------------------------------------------------------
void vtkWaylandRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DisplayId (from HW window): " << this->DisplayId << std::endl;
  os << indent << "WindowId (from HW window): " << this->WindowId << std::endl;
}

VTK_ABI_NAMESPACE_END
