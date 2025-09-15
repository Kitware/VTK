// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebAssemblyRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkDeprecation.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkWeakPointer.h"

#include <cstdint>
#include <deque>
#include <map>
#include <pthread.h>

#include <emscripten/emscripten.h>
#include <emscripten/eventloop.h>
#include <emscripten/html5.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>

// This interactor creates timers with custom callback which injects
// a special event type in the queue. We use an eventType = 0 for timer events.
// This macro is a convenience similar to other EMSCRIPTEN_EVENT_* macros.
#define EMSCRIPTEN_EVENT_VTK_TIMER 0

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

  // Push the event into queue.
  void EnqueueEvent(std::shared_ptr<EventDataWrapper> eventData);

  /// Wraps the given event with EventDataWrapper and push into the event queue.
  /// This method takes care of proxying the event if `Start()` was called from
  /// another thread that is not the browser UI thread.
  template <typename EventDataType>
  static EM_BOOL MaybeProxyEvent(int eventType, const EventDataType* e, void* userdata);

  /// Since emscripten does not handle timer events,
  /// the companion JavaScript code invokes this function to push timer events into
  /// the event queue.
  static void ForwardTimerEvent(void* param);

  std::deque<std::shared_ptr<EventDataWrapper>> Events;
  std::map<int, TimerBridgeData> Timers;
  std::map<int, int> VTKToPlatformTimerMap;

  pthread_t EventProcessingThread = 0;
  emscripten::ProxyingQueue EventProxyingQueue;

  bool StartedMessageLoop = false;
  bool RegisteredUICallbacks = false;
  bool ResizeObserverInstalled = false;
  bool ExpandedCanvasToContainerElement = false;
  int RepeatCounter = 0;
};

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::vtkInternals::EnqueueEvent(
  std::shared_ptr<EventDataWrapper> eventData)
{
  // swallow the previous event if it was a mouse move event.
  if (!this->Events.empty() && eventData->type() == EMSCRIPTEN_EVENT_MOUSEMOVE &&
    this->Events.back()->type() == EMSCRIPTEN_EVENT_MOUSEMOVE)
  {
    this->Events.pop_back();
  }
  this->Events.push_back(eventData);
}

//------------------------------------------------------------------------------
template <typename EventDataType>
EM_BOOL vtkWebAssemblyRenderWindowInteractor::vtkInternals::MaybeProxyEvent(
  int type, const EventDataType* event, void* userdata)
{
  if (userdata == nullptr)
  {
    vtkGenericWarningMacro(<< "MaybeProxyEvent received null user data!");
    return EM_TRUE;
  }
  auto* self = reinterpret_cast<vtkInternals*>(userdata);
  auto eventData = std::make_shared<EventDataWrapper>(type, event, sizeof(EventDataType));
  // if `Start()` was called on main thread, no need to proxy the event.
  if (self->EventProcessingThread == emscripten_main_runtime_thread_id())
  {
    self->EnqueueEvent(eventData);
    return EM_TRUE;
  }
  self->EventProxyingQueue.proxyAsync(
    self->EventProcessingThread, [eventData, self]() { self->EnqueueEvent(eventData); });
  return EM_TRUE;
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::vtkInternals::ForwardTimerEvent(void* param)
{
  auto bridge = reinterpret_cast<TimerBridgeData*>(param);
  auto eventData =
    std::make_shared<EventDataWrapper>(EMSCRIPTEN_EVENT_VTK_TIMER, &bridge->TimerId, sizeof(int));
  bridge->Internals->EnqueueEvent(eventData);
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
  this->SetCanvasSelector("#canvas");
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
  this->SetCanvasSelector(nullptr);
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::SetCanvasSelector(const char* value)
{
  auto& internals = (*this->Internals);
  // remove callbacks from previous target
  const bool reInstallUICallbacks = internals.RegisteredUICallbacks;
  if (internals.RegisteredUICallbacks)
  {
    this->UnRegisterUICallbacks();
  }
  vtkSetStringBodyMacro(CanvasSelector, value);
  // add callbacks to new target
  if (reInstallUICallbacks && value)
  {
    this->RegisterUICallbacks();
  }
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::ProcessEvents()
{
  // initialize if not already done.
  if (!this->Initialized)
  {
    this->Initialize();
  }
  if (!this->Enabled)
  {
    return;
  }
  auto& internals = (*this->Internals);
  // register UI callbacks if not already done. This can happen when the end-user application
  // bypasses `vtkRenderWindowInteractor::Start` and directly invokes `ProcessEvents`.
  if (!internals.RegisteredUICallbacks)
  {
    this->RegisterUICallbacks();
  }
  while (!internals.Events.empty())
  {
    auto& event = (*internals.Events.front());
    this->ProcessEvent(event.type(), event.data());
    internals.Events.pop_front();
  }
  if (!internals.ExpandedCanvasToContainerElement)
  {
    vtkInitializeCanvasElement(this->CanvasSelector, this->ExpandCanvasToContainer);
    internals.ExpandedCanvasToContainerElement = true;
  }
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::RegisterUICallbacks()
{
  auto& internals = (*this->Internals);
  if (internals.RegisteredUICallbacks)
  {
    // already registered.
    return;
  }
  internals.EventProcessingThread = pthread_self();

  const char* canvas = this->CanvasSelector;
  if (this->InstallHTMLResizeObserver)
  {
    emscripten_set_resize_callback_on_thread(EMSCRIPTEN_EVENT_TARGET_WINDOW, &internals, 0,
      vtkInternals::MaybeProxyEvent, EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
    internals.ResizeObserverInstalled = true;
  }

  emscripten_set_mousemove_callback_on_thread(canvas, &internals, 0, vtkInternals::MaybeProxyEvent,
    EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);

  emscripten_set_mousedown_callback_on_thread(canvas, &internals, 0, vtkInternals::MaybeProxyEvent,
    EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_set_mouseup_callback_on_thread(canvas, &internals, 0, vtkInternals::MaybeProxyEvent,
    EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);

  emscripten_set_touchmove_callback_on_thread(canvas, &internals, 0, vtkInternals::MaybeProxyEvent,
    EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);

  emscripten_set_touchstart_callback_on_thread(canvas, &internals, 0, vtkInternals::MaybeProxyEvent,
    EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_set_touchend_callback_on_thread(canvas, &internals, 0, vtkInternals::MaybeProxyEvent,
    EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_set_touchcancel_callback_on_thread(canvas, &internals, 0,
    vtkInternals::MaybeProxyEvent, EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);

  emscripten_set_mouseenter_callback_on_thread(canvas, &internals, 0, vtkInternals::MaybeProxyEvent,
    EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_set_mouseleave_callback_on_thread(canvas, &internals, 0, vtkInternals::MaybeProxyEvent,
    EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);

  emscripten_set_wheel_callback_on_thread(canvas, &internals, 0, vtkInternals::MaybeProxyEvent,
    EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);

  emscripten_set_focus_callback_on_thread(canvas, &internals, 0, vtkInternals::MaybeProxyEvent,
    EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_set_blur_callback_on_thread(canvas, &internals, 0, vtkInternals::MaybeProxyEvent,
    EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);

  emscripten_set_keydown_callback_on_thread(canvas, &internals, 0, vtkInternals::MaybeProxyEvent,
    EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_set_keyup_callback_on_thread(canvas, &internals, 0, vtkInternals::MaybeProxyEvent,
    EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_set_keypress_callback_on_thread(canvas, &internals, 0, vtkInternals::MaybeProxyEvent,
    EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);

  internals.RegisteredUICallbacks = true;
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::UnRegisterUICallbacks()
{
  auto& internals = (*this->Internals);
  if (!internals.RegisteredUICallbacks)
  {
    // already unregistered
    return;
  }
  const char* canvas = this->CanvasSelector;
  if (this->InstallHTMLResizeObserver && internals.ResizeObserverInstalled)
  {
    emscripten_set_resize_callback_on_thread(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 0, nullptr,
      EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
    internals.ResizeObserverInstalled = false;
  }
  emscripten_set_mousemove_callback_on_thread(
    canvas, nullptr, 0, nullptr, EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);

  emscripten_set_mousedown_callback_on_thread(
    canvas, nullptr, 0, nullptr, EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_set_mouseup_callback_on_thread(
    canvas, nullptr, 0, nullptr, EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);

  emscripten_set_touchmove_callback_on_thread(
    canvas, nullptr, 0, nullptr, EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);

  emscripten_set_touchstart_callback_on_thread(
    canvas, nullptr, 0, nullptr, EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_set_touchend_callback_on_thread(
    canvas, nullptr, 0, nullptr, EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_set_touchcancel_callback_on_thread(
    canvas, nullptr, 0, nullptr, EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);

  emscripten_set_mouseenter_callback_on_thread(
    canvas, nullptr, 0, nullptr, EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_set_mouseleave_callback_on_thread(
    canvas, nullptr, 0, nullptr, EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);

  emscripten_set_wheel_callback_on_thread(
    canvas, nullptr, 0, nullptr, EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);

  emscripten_set_focus_callback_on_thread(
    canvas, nullptr, 0, nullptr, EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_set_blur_callback_on_thread(
    canvas, nullptr, 0, nullptr, EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);

  emscripten_set_keydown_callback_on_thread(
    canvas, nullptr, 0, nullptr, EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_set_keyup_callback_on_thread(
    canvas, nullptr, 0, nullptr, EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);
  emscripten_set_keypress_callback_on_thread(
    canvas, nullptr, 0, nullptr, EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD);

  internals.RegisteredUICallbacks = false;
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::ProcessEvent(int type, const std::uint8_t* event)
{
  auto& internals = (*this->Internals);
  const double dpr = emscripten_get_device_pixel_ratio();

  switch (type)
  {
    case EMSCRIPTEN_EVENT_VTK_TIMER:
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
      auto* size = vtkGetParentElementBoundingRectSize(this->CanvasSelector);
      this->UpdateSize(size[0], size[1]);
      free(size);
      this->InvokeEvent(vtkCommand::ConfigureEvent);
      this->Render();
      // VTK_DEPRECATED_IN_9_5_0()
      // Remove this InvokeEvent when removing 9.5.0 deprecations. Interactor
      // resizing should be observed via ConfigureEvent (generated by the UI
      // in the vtkRenderWindowInteractor subclasses).
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
      this->SetEventInformationFlipY(emEvent->mouse.targetX * dpr, emEvent->mouse.targetY * dpr,
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

  vtkInitializeCanvasElement(this->CanvasSelector, this->ExpandCanvasToContainer);
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
  if (internals.StartedMessageLoop)
  {
    vtkWarningMacro(<< "An event loop has already been started!");
    return;
  }

  this->RegisterUICallbacks();

  if (!internals.StartedMessageLoop)
  {
    internals.StartedMessageLoop = true;
    emscripten_set_main_loop_arg(
      &spinOnce, (void*)this, 0, vtkRenderWindowInteractor::InteractorManagesTheEventLoop);
  }
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::TerminateApp(void)
{
  this->Done = true;

  auto& internals = (*this->Internals);

  this->UnRegisterUICallbacks();

  // Only post a quit message if Start was called...
  if (internals.StartedMessageLoop)
  {
    emscripten_cancel_main_loop();
    internals.StartedMessageLoop = false;
  }
  internals.ExpandedCanvasToContainerElement = false;
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
  os << indent << "CanvasSelector: " << this->CanvasSelector << endl;
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
