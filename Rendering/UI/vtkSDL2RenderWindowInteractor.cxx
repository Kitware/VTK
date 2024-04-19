// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Hide VTK_DEPRECATED_IN_9_3_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "vtkHardwareWindow.h"
#include "vtkRenderWindow.h"
#include "vtkSDL2RenderWindowInteractor.h"
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

#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#include "emscripten/html5.h"
#endif

#include "vtkStringArray.h"

#include "vtkActor.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

#ifdef __EMSCRIPTEN__
namespace
{
EM_BOOL ResizeCallback(int vtkNotUsed(eventType), const EmscriptenUiEvent* e, void* userData)
{
  auto interactor = reinterpret_cast<vtkRenderWindowInteractor*>(userData);
  interactor->UpdateSize(e->windowInnerWidth, e->windowInnerHeight);
  return 0;
}
}
#endif

vtkStandardNewMacro(vtkSDL2RenderWindowInteractor);

//------------------------------------------------------------------------------
// Construct object so that light follows camera motion.
vtkSDL2RenderWindowInteractor::vtkSDL2RenderWindowInteractor()
  : StartedMessageLoop(false)
{
}

//------------------------------------------------------------------------------
vtkSDL2RenderWindowInteractor::~vtkSDL2RenderWindowInteractor() {}

//------------------------------------------------------------------------------
void vtkSDL2RenderWindowInteractor::ProcessEvents()
{
  // No need to do anything if this is a 'mapped' interactor
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

bool vtkSDL2RenderWindowInteractor::ProcessEvent(void* arg)
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
      // The precise y value is more robust because browsers set a value b/w 0 and 1.
      // Otherwise, the value is often rounded to an integer =zero which causes a stutter
      // in dolly motion.
#ifdef __EMSCRIPTEN__
      int ev = event->wheel.preciseY > 0 ? vtkCommand::MouseWheelForwardEvent
                                         : vtkCommand::MouseWheelBackwardEvent;
#else
      int ev = event->wheel.y > 0 ? vtkCommand::MouseWheelForwardEvent
                                  : vtkCommand::MouseWheelBackwardEvent;
#endif
      this->InvokeEvent(ev, nullptr);
    }
    break;

    case SDL_WINDOWEVENT:
    {
      switch (event->window.event)
      {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        {
          this->UpdateSize(event->window.data1, event->window.data2);
          this->Render();
        }
        break;
        case SDL_WINDOWEVENT_CLOSE:
        {
          this->TerminateApp();
        }
        break;
      }
    }
    break;
  }
  return false;
}

namespace
{

void mainLoopCallback(void* arg)
{
  vtkSDL2RenderWindowInteractor* iren = static_cast<vtkSDL2RenderWindowInteractor*>(arg);
  iren->ProcessEvents();
}
}

//------------------------------------------------------------------------------
void vtkSDL2RenderWindowInteractor::StartEventLoop()
{
  // No need to do anything if this is a 'mapped' interactor
  if (!this->Enabled)
  {
    return;
  }

  this->StartedMessageLoop = 1;
#ifdef __EMSCRIPTEN__
  emscripten_set_resize_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW, reinterpret_cast<void*>(this), 1, ::ResizeCallback);
  emscripten_set_main_loop_arg(&mainLoopCallback, (void*)this, 0, 1);
#else
  while (!this->Done)
  {
    this->ProcessEvents();
  }
#endif
}

//------------------------------------------------------------------------------
void vtkSDL2RenderWindowInteractor::AddEventHandler()
{
  // No need to do anything if this is a 'mapped' interactor
  if (!this->Enabled)
  {
    return;
  }

  this->StartedMessageLoop = 1;
  this->Done = false;
#ifdef __EMSCRIPTEN__
  emscripten_set_resize_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW, reinterpret_cast<void*>(this), 1, ::ResizeCallback);
  emscripten_set_main_loop_arg(&mainLoopCallback, (void*)this, 0, 0);
#endif
}

//------------------------------------------------------------------------------
// Begin processing keyboard strokes.
void vtkSDL2RenderWindowInteractor::Initialize()
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
}

//------------------------------------------------------------------------------
void vtkSDL2RenderWindowInteractor::TerminateApp(void)
{
  this->Done = true;

#ifdef __EMSCRIPTEN__
  // Only post a quit message if Start was called...
  if (this->StartedMessageLoop)
  {
    emscripten_cancel_main_loop();
  }
#endif
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
}

//------------------------------------------------------------------------------
int vtkSDL2RenderWindowInteractor::InternalCreateTimer(
  int timerId, int vtkNotUsed(timerType), unsigned long duration)
{
  auto result = SDL_AddTimer(duration, timerCallback, reinterpret_cast<void*>(timerId));
  this->VTKToPlatformTimerMap[timerId] = result;
  return result;
}

//------------------------------------------------------------------------------
int vtkSDL2RenderWindowInteractor::InternalDestroyTimer(int platformTimerId)
{
  int tid = this->GetVTKTimerId(platformTimerId);
  auto i = this->VTKToPlatformTimerMap.find(tid);
  this->VTKToPlatformTimerMap.erase(i);
  return SDL_RemoveTimer(platformTimerId);
}

//------------------------------------------------------------------------------
void vtkSDL2RenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "StartedMessageLoop: " << this->StartedMessageLoop << endl;
}

//------------------------------------------------------------------------------
void vtkSDL2RenderWindowInteractor::ExitCallback()
{
  if (this->HasObserver(vtkCommand::ExitEvent))
  {
    this->InvokeEvent(vtkCommand::ExitEvent, nullptr);
  }

  this->TerminateApp();
}
VTK_ABI_NAMESPACE_END
