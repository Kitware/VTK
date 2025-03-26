// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebAssemblyRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkWeakPointer.h"

#include <cstdint>
#include <emscripten/emscripten.h>
#include <emscripten/eventloop.h>
#include <emscripten/html5.h>

VTK_ABI_NAMESPACE_BEGIN

class vtkEmscriptenEventHandler
{
public:
  explicit vtkEmscriptenEventHandler(vtkWebAssemblyRenderWindowInteractor* interactor)
  {
    this->Interactor = interactor;
  }

  //------------------------------------------------------------------------------
  void PushEvent(int eventType, const void* e)
  {
    this->Interactor->Events.push_back(vtkWebAssemblyRenderWindowInteractor::Event{ eventType, e });
  }

private:
  vtkWeakPointer<vtkWebAssemblyRenderWindowInteractor> Interactor;
};

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
EM_BOOL HandleResize(int eventType, const EmscriptenUiEvent* e, void* userData)
{
  auto handler = reinterpret_cast<vtkEmscriptenEventHandler*>(userData);
  handler->PushEvent(eventType, e);
  return EM_TRUE;
}

//------------------------------------------------------------------------------
EM_BOOL HandleMouseMove(int eventType, const EmscriptenMouseEvent* e, void* userData)
{
  auto handler = reinterpret_cast<vtkEmscriptenEventHandler*>(userData);
  handler->PushEvent(eventType, e);
  return EM_TRUE;
}

//------------------------------------------------------------------------------
EM_BOOL HandleMouseButton(int eventType, const EmscriptenMouseEvent* e, void* userData)
{
  auto handler = reinterpret_cast<vtkEmscriptenEventHandler*>(userData);
  handler->PushEvent(eventType, e);
  return EM_TRUE;
}

//------------------------------------------------------------------------------
EM_BOOL HandleTouch(int eventType, const EmscriptenTouchEvent* e, void* userData)
{
  auto handler = reinterpret_cast<vtkEmscriptenEventHandler*>(userData);
  handler->PushEvent(eventType, e);
  return EM_TRUE;
}

//------------------------------------------------------------------------------
EM_BOOL HandleMouseFocus(int eventType, const EmscriptenMouseEvent* e, void* userData)
{
  auto handler = reinterpret_cast<vtkEmscriptenEventHandler*>(userData);
  handler->PushEvent(eventType, e);
  return EM_TRUE;
}

//------------------------------------------------------------------------------
EM_BOOL HandleWheel(int eventType, const EmscriptenWheelEvent* e, void* userData)
{
  auto handler = reinterpret_cast<vtkEmscriptenEventHandler*>(userData);
  handler->PushEvent(eventType, e);
  return EM_TRUE;
}

//------------------------------------------------------------------------------
EM_BOOL HandleFocus(int eventType, const EmscriptenFocusEvent* e, void* userData)
{
  auto handler = reinterpret_cast<vtkEmscriptenEventHandler*>(userData);
  handler->PushEvent(eventType, e);
  return EM_TRUE;
}

//------------------------------------------------------------------------------
EM_BOOL HandleKey(int eventType, const EmscriptenKeyboardEvent* e, void* userData)
{
  auto handler = reinterpret_cast<vtkEmscriptenEventHandler*>(userData);
  handler->PushEvent(eventType, e);
  return EM_TRUE;
}

//------------------------------------------------------------------------------
EM_BOOL HandleKeyPress(int eventType, const EmscriptenKeyboardEvent* e, void* userData)
{
  auto handler = reinterpret_cast<vtkEmscriptenEventHandler*>(userData);
  handler->PushEvent(eventType, e);
  return EM_TRUE;
}

//------------------------------------------------------------------------------
void spinOnce(void* arg)
{
  vtkWebAssemblyRenderWindowInteractor* iren =
    static_cast<vtkWebAssemblyRenderWindowInteractor*>(arg);
  iren->ProcessEvents();
}

//------------------------------------------------------------------------------
void onTimerEvent(void* param)
{
  auto bridge = reinterpret_cast<vtkWebAssemblyRenderWindowInteractor::TimerBridgeData*>(param);
  bridge->Handler->PushEvent(0, reinterpret_cast<void*>(bridge->TimerId));
}
} // namespace

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
  : Handler(std::make_shared<vtkEmscriptenEventHandler>(this))
{
  // default is #canvas unless explicitly set by application.
  this->SetCanvasId("#canvas");
  this->ExpandCanvasToContainer = ::DefaultExpandVTKCanvasToContainer;
  this->InstallHTMLResizeObserver = ::DefaultInstallHTMLResizeObserver;
}

//------------------------------------------------------------------------------
vtkWebAssemblyRenderWindowInteractor::~vtkWebAssemblyRenderWindowInteractor()
{
  for (const auto& timerIds : this->VTKToPlatformTimerMap)
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

  Event event;
  std::vector<Event> filteredEvents;

  while (!this->Events.empty())
  {
    event = this->Events.front();
    if (event.Type == EMSCRIPTEN_EVENT_MOUSEMOVE && !filteredEvents.empty() &&
      filteredEvents.back().Type == event.Type)
    {
      // refresh the event with the latest mouse move.
      filteredEvents.back() = event;
    }
    else
    {
      // push new event.
      filteredEvents.push_back(event);
    }
    this->Events.pop_front();
  }

  for (auto& ev : filteredEvents)
  {
    if (this->ProcessEvent(&ev))
    {
      break;
    }
  }
}

//------------------------------------------------------------------------------
bool vtkWebAssemblyRenderWindowInteractor::ProcessEvent(Event* event)
{
  switch (event->Type)
  {
    case 0: // user event
    {
      const int tid = reinterpret_cast<std::intptr_t>(event->Data);
      auto iter = this->VTKToPlatformTimerMap.find(tid);
      if (iter != this->VTKToPlatformTimerMap.end())
      {
        this->InvokeEvent(vtkCommand::TimerEvent, (void*)&tid);
        int ptid = (*iter).second;
        // Here we deal with one-shot versus repeating timers
        if (this->IsOneShotTimer(tid))
        {
          vtkDestroyTimer(ptid, true);
          this->Timers.erase(tid);
          this->VTKToPlatformTimerMap.erase(iter);
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
      this->InvokeEvent(vtkCommand::LeaveEvent);
      break;
    }
    case EMSCRIPTEN_EVENT_KEYUP:
    case EMSCRIPTEN_EVENT_KEYDOWN:
    case EMSCRIPTEN_EVENT_KEYPRESS:
    {
      auto emEvent = reinterpret_cast<const EmscriptenKeyboardEvent*>(event->Data);
      const size_t nChar = strlen(emEvent->key);
      char keyCode = 0;
      if (nChar == 1)
      {
        keyCode = emEvent->key[0];
      }
      int eventVTK = 0;
      if (event->Type == EMSCRIPTEN_EVENT_KEYDOWN)
      {
        eventVTK = vtkCommand::KeyPressEvent;
      }
      else if (event->Type == EMSCRIPTEN_EVENT_KEYUP)
      {
        this->RepeatCounter = 0;
        eventVTK = vtkCommand::KeyReleaseEvent;
      }
      else // EMSCRIPTEN_EVENT_KEYPRESS
      {
        ++this->RepeatCounter;
        eventVTK = vtkCommand::CharEvent;
      }
      this->SetAltKey(emEvent->altKey);
      this->SetKeyEventInformation(
        emEvent->ctrlKey, emEvent->shiftKey, keyCode, this->RepeatCounter, emEvent->key);
      this->InvokeEvent(eventVTK, nullptr);
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEMOVE:
    {
      auto emEvent = reinterpret_cast<const EmscriptenMouseEvent*>(event->Data);
      this->SetEventInformationFlipY(
        emEvent->targetX, emEvent->targetY, emEvent->ctrlKey, emEvent->shiftKey);
      this->SetAltKey(emEvent->altKey);
      this->InvokeEvent(vtkCommand::MouseMoveEvent, nullptr);
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEDOWN:
    {
      auto emEvent = reinterpret_cast<const EmscriptenMouseEvent*>(event->Data);
      this->SetEventInformationFlipY(
        emEvent->targetX, emEvent->targetY, emEvent->ctrlKey, emEvent->shiftKey);
      this->SetAltKey(emEvent->altKey);
      this->InvokeEvent(::EmscriptenMouseButtonDownEventMap[emEvent->button]);
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEUP:
    {
      auto emEvent = reinterpret_cast<const EmscriptenMouseEvent*>(event->Data);
      this->SetEventInformationFlipY(
        emEvent->targetX, emEvent->targetY, emEvent->ctrlKey, emEvent->shiftKey);
      this->SetAltKey(emEvent->altKey);
      this->InvokeEvent(::EmscriptenMouseButtonUpEventMap[emEvent->button]);
      break;
    }
    case EMSCRIPTEN_EVENT_DBLCLICK:
    {
      auto emEvent = reinterpret_cast<const EmscriptenMouseEvent*>(event->Data);
      this->SetEventInformationFlipY(
        emEvent->targetX, emEvent->targetY, emEvent->ctrlKey, emEvent->shiftKey);
      this->SetAltKey(emEvent->altKey);
      this->InvokeEvent(::EmscriptenMouseButtonDblClickEventMap[emEvent->button]);
      break;
    }
    case EMSCRIPTEN_EVENT_WHEEL:
    {
      auto emEvent = reinterpret_cast<const EmscriptenWheelEvent*>(event->Data);
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
      auto emEvent = reinterpret_cast<const EmscriptenTouchEvent*>(event->Data);
      for (int idx = 0; idx < emEvent->numTouches; idx++)
      {
        this->SetEventInformationFlipY(emEvent->touches[idx].targetX, emEvent->touches[idx].targetY,
          emEvent->ctrlKey, emEvent->shiftKey, 0, 0, nullptr, idx);
      }
      this->LeftButtonPressEvent();
      break;
    }
    case EMSCRIPTEN_EVENT_TOUCHCANCEL:
    case EMSCRIPTEN_EVENT_TOUCHEND:
    {
      auto emEvent = reinterpret_cast<const EmscriptenTouchEvent*>(event->Data);
      for (int idx = 0; idx < emEvent->numTouches; idx++)
      {
        this->SetEventInformationFlipY(emEvent->touches[idx].targetX, emEvent->touches[idx].targetY,
          emEvent->ctrlKey, emEvent->shiftKey, 0, 0, nullptr, idx);
      }
      this->LeftButtonReleaseEvent();
      break;
    }
    case EMSCRIPTEN_EVENT_TOUCHMOVE:
    {
      auto emEvent = reinterpret_cast<const EmscriptenTouchEvent*>(event->Data);
      for (int idx = 0; idx < emEvent->numTouches; idx++)
      {
        this->SetEventInformationFlipY(emEvent->touches[idx].targetX, emEvent->touches[idx].targetY,
          emEvent->ctrlKey, emEvent->shiftKey, 0, 0, nullptr, idx);
      }
      this->MouseMoveEvent();
      break;
    }
    default:
      vtkWarningMacro(<< "Unhandled event " << event->Type);
      break;
  }
  return false;
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

  const char* canvas = this->CanvasId;

  if (this->InstallHTMLResizeObserver)
  {
    emscripten_set_resize_callback(
      EMSCRIPTEN_EVENT_TARGET_WINDOW, this->Handler.get(), 0, ::HandleResize);
    this->ResizeObserverInstalled = true;
  }

  emscripten_set_mousemove_callback(canvas, this->Handler.get(), 0, ::HandleMouseMove);

  emscripten_set_mousedown_callback(canvas, this->Handler.get(), 0, ::HandleMouseButton);
  emscripten_set_mouseup_callback(canvas, this->Handler.get(), 0, ::HandleMouseButton);

  emscripten_set_touchmove_callback(canvas, this->Handler.get(), 0, ::HandleTouch);

  emscripten_set_touchstart_callback(canvas, this->Handler.get(), 0, ::HandleTouch);
  emscripten_set_touchend_callback(canvas, this->Handler.get(), 0, ::HandleTouch);
  emscripten_set_touchcancel_callback(canvas, this->Handler.get(), 0, ::HandleTouch);

  emscripten_set_mouseenter_callback(canvas, this->Handler.get(), 0, ::HandleMouseFocus);
  emscripten_set_mouseleave_callback(canvas, this->Handler.get(), 0, ::HandleMouseFocus);

  emscripten_set_wheel_callback(canvas, this->Handler.get(), 0, ::HandleWheel);

  emscripten_set_focus_callback(canvas, this->Handler.get(), 0, ::HandleFocus);
  emscripten_set_blur_callback(canvas, this->Handler.get(), 0, ::HandleFocus);

  emscripten_set_keydown_callback(canvas, this->Handler.get(), 0, ::HandleKey);
  emscripten_set_keyup_callback(canvas, this->Handler.get(), 0, ::HandleKey);
  emscripten_set_keypress_callback(canvas, this->Handler.get(), 0, ::HandleKeyPress);

  if (!this->StartedMessageLoop)
  {
    this->StartedMessageLoop = true;
    emscripten_set_main_loop_arg(
      &spinOnce, (void*)this, 0, vtkRenderWindowInteractor::InteractorManagesTheEventLoop);
  }
  vtkInitializeCanvasElement(this->CanvasId, this->ExpandCanvasToContainer);
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::TerminateApp(void)
{
  this->Done = true;

  const char* canvas = this->CanvasId;
  if (this->InstallHTMLResizeObserver && this->ResizeObserverInstalled)
  {
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 0, nullptr);
    this->ResizeObserverInstalled = false;
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
  if (this->StartedMessageLoop)
  {
    emscripten_cancel_main_loop();
    this->StartedMessageLoop = false;
  }
}

//------------------------------------------------------------------------------
int vtkWebAssemblyRenderWindowInteractor::InternalCreateTimer(
  int timerId, int timerType, unsigned long duration)
{
  this->Timers.insert(std::make_pair(timerId, TimerBridgeData{ this->Handler, timerId }));
  auto& userData = this->Timers[timerId];
  int platformTimerId = -1;
  auto* timerBridgePtr = reinterpret_cast<void*>(&userData);
  platformTimerId = vtkCreateTimer(
    duration, timerType == vtkRenderWindowInteractor::OneShotTimer, ::onTimerEvent, timerBridgePtr);
  this->VTKToPlatformTimerMap[timerId] = platformTimerId;
  return platformTimerId;
}

//------------------------------------------------------------------------------
int vtkWebAssemblyRenderWindowInteractor::InternalDestroyTimer(int platformTimerId)
{
  int tid = this->GetVTKTimerId(platformTimerId);
  vtkDestroyTimer(platformTimerId, this->IsOneShotTimer(tid));
  this->Timers.erase(tid);
  auto i = this->VTKToPlatformTimerMap.find(tid);
  this->VTKToPlatformTimerMap.erase(i);
  return 0;
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CanvasId: " << this->CanvasId << endl;
  os << indent << "ExpandCanvasToContainer: " << this->ExpandCanvasToContainer << endl;
  os << indent << "InstallHTMLResizeObserver: " << this->InstallHTMLResizeObserver << endl;
  os << indent << "StartedMessageLoop: " << this->StartedMessageLoop << endl;
  os << indent << "ResizeObserverInstalled: " << this->ResizeObserverInstalled << endl;
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
