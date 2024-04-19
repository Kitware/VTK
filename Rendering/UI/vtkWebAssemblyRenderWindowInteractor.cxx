// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRenderWindowInteractor.h"

// Ignore reserved-identifier warnings from
// 1. SDL2/SDL_stdinc.h: warning: identifier '_SDL_size_mul_overflow_builtin'
// 2. SDL2/SDL_stdinc.h: warning: identifier '_SDL_size_add_overflow_builtin'
// 3. SDL2/SDL_audio.h: warning: identifier '_SDL_AudioStream'
// 4. SDL2/SDL_joystick.h: warning: identifier '_SDL_Joystick'
// 5. SDL2/SDL_sensor.h: warning: identifier '_SDL_Sensor'
// 6. SDL2/SDL_gamecontroller.h: warning: identifier '_SDL_GameController'
// 7. SDL2/SDL_haptic.h: warning: identifier '_SDL_Haptic'
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif
#include "SDL.h"
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include "emscripten.h"
#include "emscripten/html5.h"

#include "vtkWebAssemblyRenderWindowInteractor.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"

VTK_ABI_NAMESPACE_BEGIN

namespace
{

//------------------------------------------------------------------------------
void spinOnce(void* arg)
{
  vtkWebAssemblyRenderWindowInteractor* iren =
    static_cast<vtkWebAssemblyRenderWindowInteractor*>(arg);
  iren->ProcessEvents();
}

} // namespace

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebAssemblyRenderWindowInteractor);

//------------------------------------------------------------------------------
vtkWebAssemblyRenderWindowInteractor::vtkWebAssemblyRenderWindowInteractor()
{
  // default is #canvas unless explicitly set by application.
  this->SetCanvasSelector("#canvas");
}

//------------------------------------------------------------------------------
vtkWebAssemblyRenderWindowInteractor::~vtkWebAssemblyRenderWindowInteractor()
{
  this->SetCanvasSelector(nullptr);
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::ProcessEvents()
{
  if (!this->Enabled)
  {
    return;
  }

  SDL_Event event;
  std::vector<SDL_Event> events;

  // SDL generates continuous sequences of mouse motion events per frame,
  // let use only last event of each sequence

  while (SDL_PollEvent(&event))
  {
    if (event.type == SDL_MOUSEMOTION && !events.empty() && events.back().type == event.type)
    {
      events.back() = event;
    }
    else
    {
      events.push_back(event);
    }
  }

  for (SDL_Event ev : events)
  {
    if (this->ProcessEvent(&ev))
    {
      break;
    }
  }
}

//------------------------------------------------------------------------------
bool vtkWebAssemblyRenderWindowInteractor::ProcessEvent(void* arg)
{
  SDL_Event* event = reinterpret_cast<SDL_Event*>(arg);
  SDL_Keymod modstates = SDL_GetModState();

  int alt = modstates & (KMOD_LALT | KMOD_RALT) ? 1 : 0;
  int shift = modstates & (KMOD_LSHIFT | KMOD_RSHIFT) ? 1 : 0;
  int ctrl = modstates & (KMOD_LCTRL | KMOD_RCTRL) ? 1 : 0;

  switch (event->type)
  {
    case SDL_QUIT:
    {
      return true;
    }

    case SDL_USEREVENT:
    {
      if (event->user.data1 == reinterpret_cast<void*>(vtkCommand::TimerEvent))
      {
        int tid = static_cast<int>(reinterpret_cast<int64_t>(event->user.data2));
        auto iter = this->VTKToPlatformTimerMap.find(tid);
        if (iter != this->VTKToPlatformTimerMap.end())
        {
          this->InvokeEvent(vtkCommand::TimerEvent, (void*)&tid);
          int ptid = (*iter).second;
          // Here we deal with one-shot versus repeating timers
          if (this->IsOneShotTimer(tid))
          {
            SDL_RemoveTimer(ptid);
          }
        }
      }
    }
    break;

    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
      std::string keyname = SDL_GetKeyName(event->key.keysym.sym);
      {
        this->SetKeyEventInformation(
          ctrl, shift, event->key.keysym.sym, event->key.repeat, keyname.c_str());
        this->SetAltKey(alt);
        this->InvokeEvent(
          (event->type == SDL_KEYDOWN) ? vtkCommand::KeyPressEvent : vtkCommand::KeyReleaseEvent,
          nullptr);
      }
    }
    break;

    case SDL_TEXTINPUT:
    {
      this->SetKeyEventInformation(
        ctrl, shift, event->text.text[0], event->key.repeat, event->text.text);
      this->SetAltKey(alt);
      this->InvokeEvent(vtkCommand::CharEvent);
    }
    break;

    case SDL_MOUSEMOTION:
    {
      this->SetEventInformationFlipY(event->motion.x, event->motion.y, ctrl, shift);
      this->SetAltKey(alt);
      this->InvokeEvent(vtkCommand::MouseMoveEvent, nullptr);
    }
    break;

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    {
      this->SetEventInformationFlipY(event->button.x, event->button.y, ctrl, shift);
      this->SetAltKey(alt);

      int ev = -1;

      switch (event->button.button)
      {
        case SDL_BUTTON_LEFT:
          ev = event->button.state == SDL_PRESSED ? vtkCommand::LeftButtonPressEvent
                                                  : vtkCommand::LeftButtonReleaseEvent;
          break;
        case SDL_BUTTON_MIDDLE:
          ev = event->button.state == SDL_PRESSED ? vtkCommand::MiddleButtonPressEvent
                                                  : vtkCommand::MiddleButtonReleaseEvent;
          break;
        case SDL_BUTTON_RIGHT:
          ev = event->button.state == SDL_PRESSED ? vtkCommand::RightButtonPressEvent
                                                  : vtkCommand::RightButtonReleaseEvent;
          break;
      }
      if (ev >= 0)
      {
        this->InvokeEvent(ev, nullptr);
      }
    }
    break;

    case SDL_MOUSEWHEEL:
    {
      this->SetControlKey(ctrl);
      this->SetShiftKey(shift);
      this->SetAltKey(alt);
      // The precise y value is more robust because browsers set a value b/w 0
      // and 1. Otherwise, the value is often rounded to an integer =zero which
      // causes a stutter in dolly motion.
      int ev = event->wheel.preciseY > 0 ? vtkCommand::MouseWheelForwardEvent
                                         : vtkCommand::MouseWheelBackwardEvent;
      this->InvokeEvent(ev, nullptr);
    }
    break;

    case SDL_WINDOWEVENT:
    {
      switch (event->window.event)
      {
        case SDL_WINDOWEVENT_CLOSE:
        {
          vtkWarningMacro(<< "Terminating application because q or e was pressed. "
                             "You can restart "
                             "the event loop by calling `Start`");
          this->TerminateApp();
        }
        break;
      }
    }
    break;
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::StartEventLoop()
{
  // No need to do anything if this is a 'mapped' interactor
  if (!this->Enabled)
  {
    return;
  }

  this->StartedMessageLoop = 1;
  // re-initialized because SDL2 may reset the style.
  this->InitializeCanvasElement();

  emscripten_set_main_loop_arg(
    &spinOnce, (void*)this, 0, vtkRenderWindowInteractor::InteractorManagesTheEventLoop);
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

  this->InitializeCanvasElement();
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::TerminateApp(void)
{
  this->Done = true;

  // Only post a quit message if Start was called...
  if (this->StartedMessageLoop)
  {
    emscripten_cancel_main_loop();
  }
}

namespace
{
Uint32 timerCallback(Uint32 interval, void* param)
{
  SDL_Event event;
  SDL_UserEvent userevent;

  userevent.type = SDL_USEREVENT;
  userevent.code = 0;
  userevent.data1 = reinterpret_cast<void*>(vtkCommand::TimerEvent);
  userevent.data2 = param;

  event.type = SDL_USEREVENT;
  event.user = userevent;

  SDL_PushEvent(&event);
  return (interval);
}
} // namespace

//------------------------------------------------------------------------------
int vtkWebAssemblyRenderWindowInteractor::InternalCreateTimer(
  int timerId, int vtkNotUsed(timerType), unsigned long duration)
{
  auto result = SDL_AddTimer(duration, timerCallback, reinterpret_cast<void*>(timerId));
  this->VTKToPlatformTimerMap[timerId] = result;
  return result;
}

//------------------------------------------------------------------------------
int vtkWebAssemblyRenderWindowInteractor::InternalDestroyTimer(int platformTimerId)
{
  int tid = this->GetVTKTimerId(platformTimerId);
  auto i = this->VTKToPlatformTimerMap.find(tid);
  this->VTKToPlatformTimerMap.erase(i);
  return SDL_RemoveTimer(platformTimerId);
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CanvasSelector: " << this->CanvasSelector << endl;
  os << indent << "ExpandCanvasToContainer: " << this->ExpandCanvasToContainer << endl;
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

extern "C"
{
  void EMSCRIPTEN_KEEPALIVE setVTKRenderWindowSize(int w, int h, void* self);
}

void setVTKRenderWindowSize(int w, int h, void* self)
{
  if (auto asObjectBase = static_cast<vtkObjectBase*>(self))
  {
    if (auto iren = vtkRenderWindowInteractor::SafeDownCast(asObjectBase))
    {
      iren->UpdateSize(w, h);
      if (auto renWin = iren->GetRenderWindow())
      {
        renWin->Render();
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebAssemblyRenderWindowInteractor::InitializeCanvasElement()
{
  // clang-format off
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"
#endif
  EM_ASM({
    const selector = UTF8ToString($0);
    const applyStyle = $1;
    const canvasElem = document.querySelector(selector);
    if (canvasElem) {
      const containerElem = canvasElem.parentElement;
      const body = document.querySelector('body');
      if (applyStyle) {
        if (body === containerElem) {
          // fill up entire space of the body.
          body.style.margin = 0;
          body.style.width = '100vw';
          body.style.height = '100vh';
        } else {
          containerElem.style.position = 'relative';
          containerElem.style.width = '100%';
          containerElem.style.height = '100%';
        }
        canvasElem.style.position = 'absolute';
        canvasElem.style.top = 0;
        canvasElem.style.left = 0;
        canvasElem.style.width = '100%';
        canvasElem.style.height = '100%';
      }
    }
  }, this->CanvasSelector, this->ExpandCanvasToContainer);
  // clang-format on

  if (!this->ResizeObserverInstalled)
  {
    // clang-format off
    EM_ASM({
      const selector = UTF8ToString($0);
      const canvasElem = document.querySelector(selector);
      if (canvasElem) {
        const containerElem = canvasElem.parentElement;
        const body = document.querySelector('body');
        const resize = () => {
          const dpr = window.devicePixelRatio;
          const width = containerElem.getBoundingClientRect().width;
          const height = containerElem.getBoundingClientRect().height;
          const w = Math.floor(width * dpr + 0.5);
          const h = Math.floor(height * dpr + 0.5);
          Module._setVTKRenderWindowSize(w, h, $1);
        };

        if (body === containerElem) {
          window.addEventListener('resize', resize);
        } else {
          const resizeObserver = new ResizeObserver(resize);
          resizeObserver.observe(containerElem);
        }
      }
    }, this->CanvasSelector, this);
    // clang-format on
#ifdef __clang__
#pragma clang diagnostic pop
#endif
    this->ResizeObserverInstalled = true;
  }
}

VTK_ABI_NAMESPACE_END
