// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebAssemblyRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkWeakPointer.h"

#include <cstdint>
#include <deque>
#include <map>

#include <emscripten/emscripten.h>
#include <emscripten/eventloop.h>
#include <emscripten/html5.h>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
//------------------------------------------------------------------------------
bool DefaultExpandVTKCanvasToContainer = true;

//------------------------------------------------------------------------------
bool DefaultInstallHTMLResizeObserver = true;

//------------------------------------------------------------------------------
int EmscriptenMouseButtonDownEventMap[3] = { vtkCommand::LeftButtonPressEvent,
  vtkCommand::MiddleButtonPressEvent, vtkCommand::RightButtonPressEvent };

//------------------------------------------------------------------------------
int EmscriptenMouseButtonUpEventMap[3] = { vtkCommand::LeftButtonReleaseEvent,
  vtkCommand::MiddleButtonReleaseEvent, vtkCommand::RightButtonReleaseEvent };

//------------------------------------------------------------------------------
int EmscriptenMouseButtonDblClickEventMap[3] = { vtkCommand::LeftButtonDoubleClickEvent,
  vtkCommand::MiddleButtonDoubleClickEvent, vtkCommand::RightButtonDoubleClickEvent };

//------------------------------------------------------------------------------
void spinOnce(void* arg)
{
  vtkWebAssemblyRenderWindowInteractor* iren =
    static_cast<vtkWebAssemblyRenderWindowInteractor*>(arg);
  iren->ProcessEvents();
}
} // namespace

class vtkWebAssemblyRenderWindowInteractor::vtkInternals
{
public:
  class EventDataWrapper
  {
    int Type = -1; // Takes on values that are defined in emscripten/html5.h `EMSCRIPTEN_EVENT_NAME`
    std::unique_ptr<std::uint8_t> Data = nullptr;

  public:
    explicit EventDataWrapper(int type, const void* data, std::size_t size)
      : Type(type)
    {
      // copies the event data because the event processing is deferred.
      this->Data.reset(new std::uint8_t[size]);
      auto* src = reinterpret_cast<const std::uint8_t*>(data);
      std::copy(src, src + size, this->Data.get());
    }

    /// provides read-only access to prevent accidental manipulation of events.
    inline const std::uint8_t* data() { return this->Data.get(); }

    /// A value of > 1 implies the `Data` pointer describes an emscripten event. A value of 0 is
    /// reserved for forwarding timer events.
    inline int type() { return this->Type; }
  };

  /// Bridge data structure used to forward VTK timer ID from JS to C++.
  struct TimerBridgeData
  {
    std::shared_ptr<vtkWebAssemblyRenderWindowInteractor::vtkInternals> Internals;
    int TimerId;
  };

  /// Wraps the given event with EventDataWrapper and push into the event queue.
  template <typename EventDataType>
  static EM_BOOL PushEvent(int eventType, const EventDataType* e, void* userdata);

  /// Since emscripten does not handle timer events,
  /// the companion JavaScript code invokes this function to push timer events into
  /// the event queue.
  static void ForwardTimerEvent(void* param);

  std::deque<std::shared_ptr<EventDataWrapper>> Events;
  std::map<int, TimerBridgeData> Timers;
  std::map<int, int> VTKToPlatformTimerMap;

  bool StartedMessageLoop = false;
  bool ResizeObserverInstalled = false;
  int RepeatCounter = 0;
};

//------------------------------------------------------------------------------
template <typename EventDataType>
EM_BOOL vtkWebAssemblyRenderWindowInteractor::vtkInternals::PushEvent(
  int type, const EventDataType* event, void* userdata)
{
  auto* self = reinterpret_cast<vtkInternals*>(userdata);
  // swallow the previous event if it was a mouse move event.
  if (!self->Events.empty() && type == EMSCRIPTEN_EVENT_MOUSEMOVE &&
    self->Events.back()->type() == EMSCRIPTEN_EVENT_MOUSEMOVE)
  {
    self->Events.pop_back();
  }
  self->Events.emplace_back(std::make_shared<EventDataWrapper>(type, event, sizeof(EventDataType)));
  return EM_TRUE;
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::vtkInternals::ForwardTimerEvent(void* param)
{
  auto bridge = reinterpret_cast<TimerBridgeData*>(param);
  vtkInternals::PushEvent<int>(0, &bridge->TimerId, bridge->Internals.get());
}

extern "C"
{
  // These functions must have C linkage to prevent mangling in order to appear
  // on the wasm table as "_FunctionName"
  //------------------------------------------------------------------------------
  void EMSCRIPTEN_KEEPALIVE setDefaultExpandVTKCanvasToContainer(bool value)
  {
    ::DefaultExpandVTKCanvasToContainer = value;
  }

  //------------------------------------------------------------------------------
  void EMSCRIPTEN_KEEPALIVE setDefaultInstallHTMLResizeObserver(bool value)
  {
    ::DefaultInstallHTMLResizeObserver = value;
  }
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebAssemblyRenderWindowInteractor);

//------------------------------------------------------------------------------
vtkWebAssemblyRenderWindowInteractor::vtkWebAssemblyRenderWindowInteractor()
  : Internals(std::make_shared<vtkInternals>())
{
  // default is #canvas unless explicitly set by application.
  this->SetCanvasId("#canvas");
  this->ExpandCanvasToContainer = ::DefaultExpandVTKCanvasToContainer;
  this->InstallHTMLResizeObserver = ::DefaultInstallHTMLResizeObserver;
}

//------------------------------------------------------------------------------
vtkWebAssemblyRenderWindowInteractor::~vtkWebAssemblyRenderWindowInteractor()
{
  for (const auto& timerIds : this->Internals->VTKToPlatformTimerMap)
  {
    auto& tid = timerIds.first;
    auto& platformTimerId = timerIds.second;
    vtkDestroyTimer(platformTimerId, this->IsOneShotTimer(tid));
  }
  this->SetCanvasId(nullptr);
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::ProcessEvents()
{
  if (!this->Enabled)
  {
    return;
  }
  auto& internals = (*this->Internals);
  while (!internals.Events.empty())
  {
    auto& event = (*internals.Events.front());
    this->ProcessEvent(event.type(), event.data());
    internals.Events.pop_front();
  }
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::ProcessEvent(int type, const std::uint8_t* event)
{
  auto& internals = (*this->Internals);
  const double dpr = emscripten_get_device_pixel_ratio();

  switch (type)
  {
    case 0: // user event
    {
      auto* timerId = reinterpret_cast<const int*>(event);
      auto iter = internals.VTKToPlatformTimerMap.find(*timerId);
      if (iter != internals.VTKToPlatformTimerMap.end())
      {
        this->InvokeEvent(vtkCommand::TimerEvent, (void*)timerId);
        int platformTimerId = (*iter).second;
        // Here we deal with one-shot versus repeating timers
        if (this->IsOneShotTimer(*timerId))
        {
          vtkDestroyTimer(platformTimerId, true);
          internals.Timers.erase(*timerId);
          internals.VTKToPlatformTimerMap.erase(iter);
        }
      }
      break;
    }
    case EMSCRIPTEN_EVENT_RESIZE:
    {
      auto* size = vtkGetParentElementBoundingRectSize(this->CanvasId);
      this->UpdateSize(size[0], size[1]);
      free(size);
      this->Render();
      this->InvokeEvent(vtkCommand::WindowResizeEvent);
      break;
    }
    case EMSCRIPTEN_EVENT_FOCUS:
    case EMSCRIPTEN_EVENT_FOCUSIN:
    case EMSCRIPTEN_EVENT_MOUSEENTER:
    {
      this->InvokeEvent(vtkCommand::EnterEvent);
      break;
    }
    case EMSCRIPTEN_EVENT_BLUR:
    case EMSCRIPTEN_EVENT_FOCUSOUT:
    case EMSCRIPTEN_EVENT_MOUSELEAVE:
    {
      // resets repeat counter when focus is lost while a key is being pressed in order to
      // prevent overflow.
      internals.RepeatCounter = 0;
      this->InvokeEvent(vtkCommand::LeaveEvent);
      break;
    }
    case EMSCRIPTEN_EVENT_KEYPRESS:
      // EMSCRIPTEN_EVENT_KEYDOWN tracks these
      break;
    case EMSCRIPTEN_EVENT_KEYDOWN:
    {
      auto emEvent = reinterpret_cast<const EmscriptenKeyboardEvent*>(event);
      const size_t nChar = strlen(emEvent->key);
      char keyCode = 0;
      if (nChar == 1)
      {
        keyCode = emEvent->key[0];
      }
      // track repeated presses as long as the keydown event is sent
      ++internals.RepeatCounter;
      this->SetAltKey(emEvent->altKey);
      this->SetKeyEventInformation(
        emEvent->ctrlKey, emEvent->shiftKey, keyCode, internals.RepeatCounter, emEvent->key);
      this->InvokeEvent(vtkCommand::KeyPressEvent, nullptr);
      // additionally invokes CharEvent to satisfy observers that listen to it.
      // this is similar to other interactors.
      this->InvokeEvent(vtkCommand::CharEvent, nullptr);
      break;
    }
    case EMSCRIPTEN_EVENT_KEYUP:
    {
      auto emEvent = reinterpret_cast<const EmscriptenKeyboardEvent*>(event);
      const size_t nChar = strlen(emEvent->key);
      char keyCode = 0;
      if (nChar == 1)
      {
        keyCode = emEvent->key[0];
      }
      // reset repeat counter
      internals.RepeatCounter = 0;
      this->SetAltKey(emEvent->altKey);
      this->SetKeyEventInformation(
        emEvent->ctrlKey, emEvent->shiftKey, keyCode, internals.RepeatCounter, emEvent->key);
      this->InvokeEvent(vtkCommand::KeyReleaseEvent, nullptr);
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEMOVE:
    {
      auto emEvent = reinterpret_cast<const EmscriptenMouseEvent*>(event);
      this->SetEventInformationFlipY(
        emEvent->targetX * dpr, emEvent->targetY * dpr, emEvent->ctrlKey, emEvent->shiftKey);
      this->SetAltKey(emEvent->altKey);
      this->InvokeEvent(vtkCommand::MouseMoveEvent, nullptr);
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEDOWN:
    {
      auto emEvent = reinterpret_cast<const EmscriptenMouseEvent*>(event);
      this->SetEventInformationFlipY(
        emEvent->targetX * dpr, emEvent->targetY * dpr, emEvent->ctrlKey, emEvent->shiftKey);
      this->SetAltKey(emEvent->altKey);
      this->InvokeEvent(::EmscriptenMouseButtonDownEventMap[emEvent->button]);
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEUP:
    {
      auto emEvent = reinterpret_cast<const EmscriptenMouseEvent*>(event);
      this->SetEventInformationFlipY(
        emEvent->targetX * dpr, emEvent->targetY * dpr, emEvent->ctrlKey, emEvent->shiftKey);
      this->SetAltKey(emEvent->altKey);
      this->InvokeEvent(::EmscriptenMouseButtonUpEventMap[emEvent->button]);
      break;
    }
    case EMSCRIPTEN_EVENT_DBLCLICK:
    {
      auto emEvent = reinterpret_cast<const EmscriptenMouseEvent*>(event);
      this->SetEventInformationFlipY(
        emEvent->targetX * dpr, emEvent->targetY * dpr, emEvent->ctrlKey, emEvent->shiftKey);
      this->SetAltKey(emEvent->altKey);
      this->InvokeEvent(::EmscriptenMouseButtonDblClickEventMap[emEvent->button]);
      break;
    }
    case EMSCRIPTEN_EVENT_WHEEL:
    {
      auto emEvent = reinterpret_cast<const EmscriptenWheelEvent*>(event);
      this->SetEventInformationFlipY(emEvent->mouse.targetX, emEvent->mouse.targetY,
        emEvent->mouse.ctrlKey, emEvent->mouse.shiftKey);
      this->SetAltKey(emEvent->mouse.altKey);
      this->InvokeEvent(emEvent->deltaY < 0 ? vtkCommand::MouseWheelForwardEvent
                                            : vtkCommand::MouseWheelBackwardEvent);
      this->InvokeEvent(
        emEvent->deltaX > 0 ? vtkCommand::MouseWheelRightEvent : vtkCommand::MouseWheelLeftEvent);
      break;
    }
    case EMSCRIPTEN_EVENT_TOUCHSTART:
    {
      auto emEvent = reinterpret_cast<const EmscriptenTouchEvent*>(event);
      for (int idx = 0; idx < emEvent->numTouches; idx++)
      {
        this->SetEventInformationFlipY(emEvent->touches[idx].targetX * dpr,
          emEvent->touches[idx].targetY * dpr, emEvent->ctrlKey, emEvent->shiftKey, 0, 0, nullptr,
          idx);
      }
      this->LeftButtonPressEvent();
      break;
    }
    case EMSCRIPTEN_EVENT_TOUCHCANCEL:
    case EMSCRIPTEN_EVENT_TOUCHEND:
    {
      auto emEvent = reinterpret_cast<const EmscriptenTouchEvent*>(event);
      for (int idx = 0; idx < emEvent->numTouches; idx++)
      {
        this->SetEventInformationFlipY(emEvent->touches[idx].targetX * dpr,
          emEvent->touches[idx].targetY * dpr, emEvent->ctrlKey, emEvent->shiftKey, 0, 0, nullptr,
          idx);
      }
      this->LeftButtonReleaseEvent();
      break;
    }
    case EMSCRIPTEN_EVENT_TOUCHMOVE:
    {
      auto emEvent = reinterpret_cast<const EmscriptenTouchEvent*>(event);
      for (int idx = 0; idx < emEvent->numTouches; idx++)
      {
        this->SetEventInformationFlipY(emEvent->touches[idx].targetX * dpr,
          emEvent->touches[idx].targetY * dpr, emEvent->ctrlKey, emEvent->shiftKey, 0, 0, nullptr,
          idx);
      }
      this->MouseMoveEvent();
      break;
    }
    default:
      vtkWarningMacro(<< "Unhandled event " << type);
      break;
  }
}
//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::Initialize()
{
  vtkRenderWindow* ren;
  int* size;

  // make sure we have a RenderWindow and camera
  if (!this->RenderWindow)
  {
    vtkErrorMacro(<< "No renderer defined!");
    return;
  }
  if (this->Initialized)
  {
    return;
  }
  this->Initialized = 1;
  // get the info we need from the RenderingWindow
  ren = this->RenderWindow;
  ren->Start();
  ren->End();
  size = ren->GetSize();
  ren->GetPosition();

  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];

  vtkInitializeCanvasElement(this->CanvasId, this->ExpandCanvasToContainer);
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::StartEventLoop()
{
  // No need to do anything if this is a 'mapped' interactor
  if (!this->Enabled)
  {
    return;
  }

  auto& internals = (*this->Internals);
  const char* canvas = this->CanvasId;
  if (this->InstallHTMLResizeObserver)
  {
    emscripten_set_resize_callback(
      EMSCRIPTEN_EVENT_TARGET_WINDOW, &internals, 0, vtkInternals::PushEvent);
    internals.ResizeObserverInstalled = true;
  }

  emscripten_set_mousemove_callback(canvas, &internals, 0, vtkInternals::PushEvent);

  emscripten_set_mousedown_callback(canvas, &internals, 0, vtkInternals::PushEvent);
  emscripten_set_mouseup_callback(canvas, &internals, 0, vtkInternals::PushEvent);

  emscripten_set_touchmove_callback(canvas, &internals, 0, vtkInternals::PushEvent);

  emscripten_set_touchstart_callback(canvas, &internals, 0, vtkInternals::PushEvent);
  emscripten_set_touchend_callback(canvas, &internals, 0, vtkInternals::PushEvent);
  emscripten_set_touchcancel_callback(canvas, &internals, 0, vtkInternals::PushEvent);

  emscripten_set_mouseenter_callback(canvas, &internals, 0, vtkInternals::PushEvent);
  emscripten_set_mouseleave_callback(canvas, &internals, 0, vtkInternals::PushEvent);

  emscripten_set_wheel_callback(canvas, &internals, 0, vtkInternals::PushEvent);

  emscripten_set_focus_callback(canvas, &internals, 0, vtkInternals::PushEvent);
  emscripten_set_blur_callback(canvas, &internals, 0, vtkInternals::PushEvent);

  emscripten_set_keydown_callback(canvas, &internals, 0, vtkInternals::PushEvent);
  emscripten_set_keyup_callback(canvas, &internals, 0, vtkInternals::PushEvent);
  emscripten_set_keypress_callback(canvas, &internals, 0, vtkInternals::PushEvent);

  if (!internals.StartedMessageLoop)
  {
    internals.StartedMessageLoop = true;
    emscripten_set_main_loop_arg(
      &spinOnce, (void*)this, 0, vtkRenderWindowInteractor::InteractorManagesTheEventLoop);
  }
  vtkInitializeCanvasElement(this->CanvasId, this->ExpandCanvasToContainer);
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::TerminateApp(void)
{
  this->Done = true;

  auto& internals = (*this->Internals);
  const char* canvas = this->CanvasId;
  if (this->InstallHTMLResizeObserver && internals.ResizeObserverInstalled)
  {
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 0, nullptr);
    internals.ResizeObserverInstalled = false;
  }
  emscripten_set_mousemove_callback(canvas, nullptr, 0, nullptr);

  emscripten_set_mousedown_callback(canvas, nullptr, 0, nullptr);
  emscripten_set_mouseup_callback(canvas, nullptr, 0, nullptr);

  emscripten_set_touchmove_callback(canvas, nullptr, 0, nullptr);

  emscripten_set_touchstart_callback(canvas, nullptr, 0, nullptr);
  emscripten_set_touchend_callback(canvas, nullptr, 0, nullptr);
  emscripten_set_touchcancel_callback(canvas, nullptr, 0, nullptr);

  emscripten_set_mouseenter_callback(canvas, nullptr, 0, nullptr);
  emscripten_set_mouseleave_callback(canvas, nullptr, 0, nullptr);

  emscripten_set_wheel_callback(canvas, nullptr, 0, nullptr);

  emscripten_set_focus_callback(canvas, nullptr, 0, nullptr);
  emscripten_set_blur_callback(canvas, nullptr, 0, nullptr);

  emscripten_set_keydown_callback(canvas, nullptr, 0, nullptr);
  emscripten_set_keyup_callback(canvas, nullptr, 0, nullptr);
  emscripten_set_keypress_callback(canvas, nullptr, 0, nullptr);

  // Only post a quit message if Start was called...
  if (internals.StartedMessageLoop)
  {
    emscripten_cancel_main_loop();
    internals.StartedMessageLoop = false;
  }
}

//------------------------------------------------------------------------------
int vtkWebAssemblyRenderWindowInteractor::InternalCreateTimer(
  int timerId, int timerType, unsigned long duration)
{
  auto& internals = (*this->Internals);
  internals.Timers.insert(
    std::make_pair(timerId, vtkInternals::TimerBridgeData{ this->Internals, timerId }));
  auto& userData = internals.Timers[timerId];
  int platformTimerId = -1;
  auto* timerBridgePtr = reinterpret_cast<void*>(&userData);
  platformTimerId = vtkCreateTimer(duration, timerType == vtkRenderWindowInteractor::OneShotTimer,
    vtkInternals::ForwardTimerEvent, timerBridgePtr);
  internals.VTKToPlatformTimerMap[timerId] = platformTimerId;
  return platformTimerId;
}

//------------------------------------------------------------------------------
int vtkWebAssemblyRenderWindowInteractor::InternalDestroyTimer(int platformTimerId)
{
  auto& internals = (*this->Internals);
  int tid = this->GetVTKTimerId(platformTimerId);
  vtkDestroyTimer(platformTimerId, this->IsOneShotTimer(tid));
  internals.Timers.erase(tid);
  auto i = internals.VTKToPlatformTimerMap.find(tid);
  internals.VTKToPlatformTimerMap.erase(i);
  return 0;
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CanvasId: " << this->CanvasId << endl;
  os << indent << "ExpandCanvasToContainer: " << this->ExpandCanvasToContainer << endl;
  os << indent << "InstallHTMLResizeObserver: " << this->InstallHTMLResizeObserver << endl;
  os << indent << "StartedMessageLoop: " << this->Internals->StartedMessageLoop << endl;
  os << indent << "ResizeObserverInstalled: " << this->Internals->ResizeObserverInstalled << endl;
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::ExitCallback()
{
  if (this->HasObserver(vtkCommand::ExitEvent))
  {
    this->InvokeEvent(vtkCommand::ExitEvent, nullptr);
  }
  this->TerminateApp();
}

VTK_ABI_NAMESPACE_END
