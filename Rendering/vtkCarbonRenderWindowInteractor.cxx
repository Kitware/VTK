/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkCarbonRenderWindowInteractor.cxx
Language:  C++
Date:      $Date$
Version:   $Revision$

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
of any contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

* Modified source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "vtkCarbonRenderWindow.h"
#include "vtkCarbonRenderWindowInteractor.h"
#include "vtkInteractorStyle.h"
#include "vtkActor.h"
#include <OpenGL/gl.h>
#include "vtkObjectFactory.h"
#include "vtkCommand.h"

#import <Carbon/Carbon.h>


vtkCxxRevisionMacro(vtkCarbonRenderWindowInteractor, "1.3");
vtkStandardNewMacro(vtkCarbonRenderWindowInteractor);

void (*vtkCarbonRenderWindowInteractor::ClassExitMethod)(void *) 
  = (void (*)(void *))NULL;
void *vtkCarbonRenderWindowInteractor::ClassExitMethodArg = (void *)NULL;
void (*vtkCarbonRenderWindowInteractor::ClassExitMethodArgDelete)(void *) 
  = (void (*)(void *))NULL;

//--------------------------------------------------------------------------
// callback routine to handle all window-related events
// The WindowPtr of the associated window is passed in userData
static pascal OSStatus myWinEvtHndlr(EventHandlerCallRef nextHandler,
                                     EventRef event, void* userData)
{
  WindowRef                        window;
  Rect                             bounds;
  OSStatus                         result = eventNotHandledErr;
  Point                            mouseLoc;
  vtkCarbonRenderWindow            *ren;
  vtkCarbonRenderWindowInteractor  *me;

  ren = (vtkCarbonRenderWindow *)GetWRefCon((WindowPtr)userData);
  
  if (NULL == ren)
    {
    return 0;
    }

  me = (vtkCarbonRenderWindowInteractor *)ren->GetInteractor();
  if (NULL == me)
    {
    return 0;
    }

  UInt32 modifierKeys;
  GetEventParameter(event, kEventParamKeyModifiers,typeUInt32, NULL,
                    sizeof(modifierKeys), NULL, &modifierKeys);
  int controlDown = (modifierKeys & controlKey);
  int shiftDown = (modifierKeys & shiftKey);

  switch (GetEventClass(event))
    {
    case kEventClassWindow:
      {
      switch (GetEventKind(event))
        {
        case kEventWindowClickContentRgn:
          {
          GetEventParameter(event, kEventParamMouseLocation, typeQDPoint,
                            NULL, sizeof(Point), NULL, &mouseLoc);
          SetPortWindowPort(FrontWindow());
          GlobalToLocal(&mouseLoc);

          UInt16 buttonNumber;
          GetEventParameter(event,kEventParamMouseButton,typeMouseButton,NULL,
                            sizeof(buttonNumber),NULL,&buttonNumber);

          me->SetEventInformationFlipY(mouseLoc.h,mouseLoc.v,
                                       controlDown,shiftDown);
          switch (buttonNumber)
            {
            case 1:
              {
              me->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
              break;
              }
            case 2:
              {
              me->InvokeEvent(vtkCommand::RightButtonPressEvent,NULL);
              break;
              }
            case 3:
              {
              me->InvokeEvent(vtkCommand::MiddleButtonPressEvent,NULL);
              break;
              }
            }
          result = noErr;
          break;
          }
        case kEventWindowDrawContent:
          {
          ren->Render();
          result = noErr;
          break;
          }
        case kEventWindowBoundsChanging:
          {
          InvalWindowRect(window, GetWindowPortBounds(window, &bounds));
          GetEventParameter(event, kEventParamCurrentBounds, typeQDRectangle,
                            NULL, sizeof(bounds), NULL, &bounds);
          me->UpdateSize(bounds.right-bounds.left, bounds.bottom-bounds.top);
          me->InvokeEvent(vtkCommand::ConfigureEvent,NULL);
          ren->MakeCurrent();
          aglUpdateContext (ren->GetContextId());

          glViewport (0, 0,
                      bounds.right - bounds.left, 
                      bounds.bottom - bounds.top);
          ren->Render();
          result = noErr;
          break;
          }
        case kEventWindowActivated:
          {
          InvalWindowRect(window, GetWindowPortBounds(window, &bounds));
          ren->MakeCurrent();
          aglUpdateContext (ren->GetContextId());
          result = noErr;
          break;
          }
        case kEventWindowClose:
          {
          result = CallNextEventHandler(nextHandler, event);
          break;
          }
        }
      break;
      }

    case kEventClassKeyboard:
      {
      SInt8 charCode;
      GetEventParameter(event, kEventParamKeyMacCharCodes, typeChar,NULL,
                        sizeof(charCode),NULL,&charCode);
        switch (GetEventKind(event))
          {
          case kEventRawKeyDown:
            {
            me->SetKeyEventInformation(controlDown, shiftDown,
                                       (int)charCode,1,&((char)charCode));
            me->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
            me->InvokeEvent(vtkCommand::CharEvent, NULL);
            break;
            }
          case kEventRawKeyRepeat:
            {
            me->SetKeyEventInformation(controlDown, shiftDown,
                                       (int)charCode,1,&((char)charCode));
            me->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
            me->InvokeEvent(vtkCommand::CharEvent, NULL);
            break;
            }
          case kEventRawKeyUp:
            {
            me->SetKeyEventInformation(controlDown, shiftDown,
                                       (int)charCode,1,&((char)charCode));
            me->InvokeEvent(vtkCommand::KeyReleaseEvent, NULL);
            break;
            }
          }
        break;
      }

    case kEventClassMouse:
      {
      GetEventParameter(event, kEventParamMouseLocation, typeQDPoint,
                        NULL, sizeof(Point), NULL, &mouseLoc);
      SetPortWindowPort(FrontWindow());
      GlobalToLocal(&mouseLoc);
      GetEventParameter(event, kEventParamKeyModifiers,typeUInt32, NULL,
                        sizeof(modifierKeys), NULL, &modifierKeys);
      UInt16 buttonNumber;
      GetEventParameter(event, kEventParamMouseButton, typeMouseButton, NULL,
                        sizeof(buttonNumber), NULL, &buttonNumber);
      me->SetEventInformationFlipY(mouseLoc.h, mouseLoc.v,
                                   (modifierKeys & controlKey),
                                   (modifierKeys & shiftKey));
      switch (GetEventKind(event))
        {
        case kEventMouseUp:
          {
          switch (buttonNumber)
            {
            case 1:
              {
              me->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);
              break;
              }
            case 2:
              {
              me->InvokeEvent(vtkCommand::RightButtonReleaseEvent,NULL);
              break;
              }
            case 3:
              {
              me->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent,NULL);
              break;
              }
            }
          result = noErr;
          break;
          }
        case kEventMouseMoved:
          {
          me->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
          result = noErr;
          break;
          }
        case kEventMouseDragged:
          {
          me->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
          result = noErr;
          break;
          }
        default:
          {
          result = noErr;
          }
        }
      }
    }
  return result;
}

//--------------------------------------------------------------------------
// Construct object so that light follows camera motion.
vtkCarbonRenderWindowInteractor::vtkCarbonRenderWindowInteractor()
{
  this->WindowId           = NULL;
  this->TimerId            = 0;
  this->InstallMessageProc = 1;
  this->OldProc            = NULL;
}

//--------------------------------------------------------------------------
vtkCarbonRenderWindowInteractor::~vtkCarbonRenderWindowInteractor()
{
  this->Enabled = 0;
}

//--------------------------------------------------------------------------
void  vtkCarbonRenderWindowInteractor::Start()
{
  // Let the compositing handle the event loop if it wants to.
  if (this->HasObserver(vtkCommand::StartEvent))
    {
    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    return;
    }

  // No need to do anything if this is a 'mapped' interactor
  if (!this->Enabled || !this->InstallMessageProc)
    {
    return;
    }
  RunApplicationEventLoop();
}

//--------------------------------------------------------------------------
// Fill in some local variables (most of this routine could probably go)
void vtkCarbonRenderWindowInteractor::Initialize()
{
  vtkCarbonRenderWindow *ren;
  int *size;

  // make sure we have a RenderWindow and camera
  if ( ! this->RenderWindow)
    {
    vtkErrorMacro(<<"No renderer defined!");
    return;
    }
  if (this->Initialized)
    {
    return;
    }
  this->Initialized = 1;
  // get the info we need from the RenderingWindow
  ren = (vtkCarbonRenderWindow *)(this->RenderWindow);
  ren->Start();
  size    = ren->GetSize();
  ren->GetPosition();
  this->WindowId = ren->GetWindowId();
  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

//--------------------------------------------------------------------------
void vtkCarbonRenderWindowInteractor::Enable()
{
  vtkCarbonRenderWindow *ren;
  vtkCarbonRenderWindow *tmp;
  if (this->Enabled)
    {
    return;
    }
  if (this->InstallMessageProc)
    {
    // add our callback
    ren = (vtkCarbonRenderWindow *)(this->RenderWindow);
    // set up the event handling
    // specify which events we want to hear about
    OSStatus   err = noErr;
    EventTypeSpec list[] = {{ kEventClassWindow, kEventWindowClose },
                            { kEventClassWindow, kEventWindowDrawContent },
                            { kEventClassWindow, kEventWindowBoundsChanging },
                            { kEventClassWindow, kEventWindowActivated },
                            { kEventClassWindow, kEventWindowClickContentRgn },
                            { kEventClassMouse, kEventMouseUp },
                            { kEventClassMouse, kEventMouseMoved },
                            { kEventClassMouse, kEventMouseDragged },
                            { kEventClassKeyboard, kEventRawKeyDown },
                            { kEventClassKeyboard, kEventRawKeyRepeat },
                            { kEventClassKeyboard, kEventRawKeyUp }};
    // if we haven't already made our window event handler UPP then do so now
    if(!this->OldProc)
      {
      this->OldProc = NewEventHandlerUPP(myWinEvtHndlr);
      if(!this->OldProc)
        err = memFullErr;
      }

    if(!err)
      {
      EventHandlerRef  ref;
      err = InstallWindowEventHandler(ren->GetWindowId(), this->OldProc,
                                      sizeof(list)/sizeof(EventTypeSpec),
                                      list, ren->GetWindowId(), &ref);
      }
    }
  this->Enabled = 1;
  this->Modified();
}

//--------------------------------------------------------------------------
void vtkCarbonRenderWindowInteractor::Disable()
{
  vtkCarbonRenderWindow *tmp;
  if (!this->Enabled)
    {
    return;
    }
  this->Enabled = 0;
  this->Modified();
}

//--------------------------------------------------------------------------
void vtkCarbonRenderWindowInteractor::TerminateApp(void)
{
  cout << "vtkCarbonRenderWindowInteractor::TerminateApp\n";
}

//--------------------------------------------------------------------------
pascal void TimerAction (EventLoopTimerRef theTimer, void* userData)
{
  if (NULL != userData)
    {
    ((vtkCarbonRenderWindowInteractor *)userData)->
      InvokeEvent(vtkCommand::TimerEvent,NULL);
    }
}

//--------------------------------------------------------------------------
int vtkCarbonRenderWindowInteractor::CreateTimer(int timertype)
{
  EventLoopRef       mainLoop = GetMainEventLoop();
  EventLoopTimerUPP  timerUPP = NewEventLoopTimerUPP(TimerAction);
  
  if (timertype == VTKI_TIMER_FIRST)
    {  
      InstallEventLoopTimer (mainLoop,
                             10*kEventDurationMillisecond,
                             10*kEventDurationMillisecond,
                             timerUPP,
                             this,
                             &this->TimerId);
    }
  return 1;
}

//--------------------------------------------------------------------------
int vtkCarbonRenderWindowInteractor::DestroyTimer(void)
{
  RemoveEventLoopTimer(this->TimerId);
  return 1;
}

//--------------------------------------------------------------------------
// Specify the default function to be called when an interactor needs to exit.
// This callback is overridden by an instance ExitMethod that is defined.
void vtkCarbonRenderWindowInteractor::SetClassExitMethod(void (*f)(void *),
                                                         void *arg)
{
  if (f != vtkCarbonRenderWindowInteractor::ClassExitMethod
      || arg != vtkCarbonRenderWindowInteractor::ClassExitMethodArg)
    {
    // delete the current arg if there is a delete method
    if ((vtkCarbonRenderWindowInteractor::ClassExitMethodArg) &&
        (vtkCarbonRenderWindowInteractor::ClassExitMethodArgDelete))
      {
      (*vtkCarbonRenderWindowInteractor::ClassExitMethodArgDelete)
        (vtkCarbonRenderWindowInteractor::ClassExitMethodArg);
      }
    vtkCarbonRenderWindowInteractor::ClassExitMethod = f;
    vtkCarbonRenderWindowInteractor::ClassExitMethodArg = arg;
    
    // no call to this->Modified() since this is a class member function
    }
}

//--------------------------------------------------------------------------
// Set the arg delete method.  This is used to free user memory.
void
vtkCarbonRenderWindowInteractor::SetClassExitMethodArgDelete(void (*f)(void *))
{
  if (f != vtkCarbonRenderWindowInteractor::ClassExitMethodArgDelete)
    {
    vtkCarbonRenderWindowInteractor::ClassExitMethodArgDelete = f;

    // no call to this->Modified() since this is a class member function
    }
}

//--------------------------------------------------------------------------
void vtkCarbonRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkRenderWindowInteractor::PrintSelf(os,indent);
  os << indent << "InstallMessageProc: " << this->InstallMessageProc << endl;
}

//--------------------------------------------------------------------------
void vtkCarbonRenderWindowInteractor::ExitCallback()
{
  if (this->ClassExitMethod)
    {
    (*this->ClassExitMethod)(this->ClassExitMethodArg);
    }
  else if (this->ClassExitMethod)
    {
    (*this->ClassExitMethod)(this->ClassExitMethodArg);
    }
  else
    {
    this->TerminateApp();
    }
}
