/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCarbonRenderWindowInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "vtkActor.h"
#include "vtkCarbonRenderWindow.h"
#include "vtkCarbonRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkInteractorStyle.h"
#include "vtkObjectFactory.h"

#import <Carbon/Carbon.h>

vtkCxxRevisionMacro(vtkCarbonRenderWindowInteractor, "1.15");
vtkStandardNewMacro(vtkCarbonRenderWindowInteractor);

void (*vtkCarbonRenderWindowInteractor::ClassExitMethod)(void *) 
  = (void (*)(void *))NULL;
void *vtkCarbonRenderWindowInteractor::ClassExitMethodArg = (void *)NULL;
void (*vtkCarbonRenderWindowInteractor::ClassExitMethodArgDelete)(void *) 
  = (void (*)(void *))NULL;

//--------------------------------------------------------------------------
// callback routine to handle all window-related events
// The WindowPtr of the associated window is passed in userData
static pascal OSStatus myWinEvtHndlr(EventHandlerCallRef,
                                     EventRef event, void* userData)
{
  OSStatus                         result = eventNotHandledErr;
  Point                            mouseLoc;
  vtkCarbonRenderWindow            *ren;
  vtkCarbonRenderWindowInteractor  *me;
  UInt32                           eventClass = GetEventClass(event);
  UInt32                           eventKind = GetEventKind (event);

  ren = (vtkCarbonRenderWindow *)userData;

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

  switch (eventClass)
    {
    case kEventClassControl:
      {
      switch (eventKind)
        {
        case kEventControlDraw:
          {
          ren->Render();
          result = noErr;
          break;
          }
        case kEventControlBoundsChanged:
          {
          if(ren->GetWindowId())
            {
            HIRect viewBounds;
            HIViewGetBounds(ren->GetWindowId(), &viewBounds);
            me->UpdateSize((int)viewBounds.size.width, (int)viewBounds.size.height);
            if (me->GetEnabled())
              {
              me->InvokeEvent(vtkCommand::ConfigureEvent,NULL);
              }
            result = noErr;
            }
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
                                       (int)charCode,1,(char*)&charCode);
            me->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
            me->InvokeEvent(vtkCommand::CharEvent, NULL);
            result = noErr;
            break;
            }
          case kEventRawKeyRepeat:
            {
            me->SetKeyEventInformation(controlDown, shiftDown,
                                       (int)charCode,1,(char*)&charCode);
            me->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
            me->InvokeEvent(vtkCommand::CharEvent, NULL);
            result = noErr;
            break;
            }
          case kEventRawKeyUp:
            {
            me->SetKeyEventInformation(controlDown, shiftDown,
                                       (int)charCode,1,(char*)&charCode);
            me->InvokeEvent(vtkCommand::KeyReleaseEvent, NULL);
            result = noErr;
            break;
            }
          }
        break;
      }

    case kEventClassMouse:
      {
      // see if the event is for this view
      HIViewRef view_for_mouse;
      HIViewGetViewForMouseEvent(HIViewGetRoot(ren->GetRootWindow()), event, &view_for_mouse);
      if(view_for_mouse != ren->GetWindowId())
        return eventNotHandledErr;

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
        case kEventMouseDown:
          {
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
        case kEventMouseWheelMoved:
          {
          EventMouseWheelAxis axis;
          SInt32 delta;
          GetEventParameter( event, kEventParamMouseWheelAxis, 
                         typeMouseWheelAxis, NULL, sizeof(axis), NULL, &axis );
          GetEventParameter( event, kEventParamMouseWheelDelta, 
                          typeLongInteger, NULL, sizeof(delta), NULL, &delta );
          if ( axis == kEventMouseWheelAxisY )
            {
             if( delta > 0)
              {
              me->InvokeEvent(vtkCommand::MouseWheelForwardEvent, NULL);
              }
            else
              {
              me->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, NULL);
              }
            }
          result = noErr;
          break;
          }
        default:
          {
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
  this->TimerId            = 0;
  this->InstallMessageProc = 1;
  this->ViewProcUPP        = NULL;
  this->WindowProcUPP      = NULL;
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
  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

//--------------------------------------------------------------------------
void vtkCarbonRenderWindowInteractor::Enable()
{
  if (this->Enabled)
    {
    return;
    }

  
  if (this->InstallMessageProc)
    {
    // set up the event handling
    // specify which events we want to hear about
    OSStatus   err = noErr;
    EventTypeSpec view_event_list[] = {{ kEventClassControl, kEventControlDraw },
                                       { kEventClassControl, kEventControlBoundsChanged }};
    
    EventTypeSpec window_event_list[] ={{ kEventClassMouse, kEventMouseDown },
                                        { kEventClassMouse, kEventMouseUp },
                                        { kEventClassMouse, kEventMouseMoved },
                                        { kEventClassMouse, kEventMouseDragged },
                                        { kEventClassMouse, kEventMouseWheelMoved },
                                        { kEventClassKeyboard, kEventRawKeyDown },
                                        { kEventClassKeyboard, kEventRawKeyRepeat },
                                        { kEventClassKeyboard, kEventRawKeyUp }};
    
    this->WindowProcUPP = NewEventHandlerUPP(myWinEvtHndlr);
    this->ViewProcUPP = NewEventHandlerUPP(myWinEvtHndlr);
    if(!this->WindowProcUPP || !ViewProcUPP)
      err = memFullErr;

    if(!err)
      {
      vtkCarbonRenderWindow* ren = static_cast<vtkCarbonRenderWindow*>(this->RenderWindow);
      err = InstallControlEventHandler(ren->GetWindowId(), this->ViewProcUPP,
                                       GetEventTypeCount(view_event_list), view_event_list, ren, NULL);
      err = InstallWindowEventHandler(ren->GetRootWindow(), this->WindowProcUPP,
                                       GetEventTypeCount(window_event_list), window_event_list, ren, NULL);
      }
    }
  this->Enabled = 1;
  this->Modified();
}

//--------------------------------------------------------------------------
void vtkCarbonRenderWindowInteractor::Disable()
{
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
pascal void TimerAction (EventLoopTimerRef, void* userData)
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
  if (this->HasObserver(vtkCommand::ExitEvent)) 
    {
    this->InvokeEvent(vtkCommand::ExitEvent,NULL);
    }
  else if (this->ClassExitMethod)
    {
    (*this->ClassExitMethod)(this->ClassExitMethodArg);
    }
  this->TerminateApp();
}
