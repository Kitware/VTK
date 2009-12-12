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

vtkCxxRevisionMacro(vtkCarbonRenderWindowInteractor, "1.27");
vtkStandardNewMacro(vtkCarbonRenderWindowInteractor);

void (*vtkCarbonRenderWindowInteractor::ClassExitMethod)(void *) 
  = (void (*)(void *))NULL;
void *vtkCarbonRenderWindowInteractor::ClassExitMethodArg = (void *)NULL;
void (*vtkCarbonRenderWindowInteractor::ClassExitMethodArgDelete)(void *) 
  = (void (*)(void *))NULL;

//--------------------------------------------------------------------------
static const char *vtkMacCharCodeToKeySymTable[128] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  "space", "exclam", "quotedbl", "numbersign", 
  "dollar", "percent", "ampersand", "quoteright", 
  "parenleft", "parenright", "asterisk", "plus", 
  "comma", "minus", "period", "slash",
  "0", "1", "2", "3", "4", "5", "6", "7", 
  "8", "9", "colon", "semicolon", "less", "equal", "greater", "question",
  "at", "A", "B", "C", "D", "E", "F", "G", 
  "H", "I", "J", "K", "L", "M", "N", "O",
  "P", "Q", "R", "S", "T", "U", "V", "W",
  "X", "Y", "Z", "bracketleft", 
  "backslash", "bracketright", "asciicircum", "underscore",
  "quoteleft", "a", "b", "c", "d", "e", "f", "g",
  "h", "i", "j", "k", "l", "m", "n", "o",
  "p", "q", "r", "s", "t", "u", "v", "w",
  "x", "y", "z", "braceleft", "bar", "braceright", "asciitilde", "Delete",
};

//--------------------------------------------------------------------------
static const char *vtkMacKeyCodeToKeySymTable[128] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, "Return", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  "Tab", 0, 0, "Backspace", 0, "Escape", 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, "period", 0, "asterisk", 0, "plus", 0, "Clear",
  0, 0, 0, "slash", "KP_Enter", 0, "minus", 0,
  0, 0, "KP_0", "KP_1", "KP_2", "KP_3", "KP_4", "KP_5",
  "KP_6", "KP_7", 0, "KP_8", "KP_9", 0, 0, 0,
  "F5", "F6", "F7", "F3", "F8", 0, 0, 0,
  0, "Snapshot", 0, 0, 0, 0, 0, 0,
  0, 0, "Help", "Home", "Prior", "Delete", "F4", "End",
  "F2", "Next", "F1", "Left", "Right", "Down", "Up", 0,
};

//--------------------------------------------------------------------------
// callback routine to handle all window-related events
// The WindowPtr of the associated window is passed in userData
static pascal OSStatus myWinEvtHndlr(EventHandlerCallRef,
                                     EventRef event, void* userData)
{
  OSStatus                         result = eventNotHandledErr;
  HIPoint                          mouseLoc;
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
  int controlDown = ((modifierKeys & controlKey) != 0);
  int shiftDown = ((modifierKeys & shiftKey) != 0);
 
  // Even though the option key is the one with a small 'alt' label on top
  // of it, VNC (as well as some Mac users) uses the command key as 'alt'.
  // Let's use both then. 
  UInt32 altKey = cmdKey | optionKey;
  int altDown = ((modifierKeys & altKey) != 0);
  
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
            me->UpdateSize(int(viewBounds.size.width),
                           int(viewBounds.size.height));
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
      const char *keySym = NULL;
      char charCode = '\0';

      UInt32 macKeyCode;
      GetEventParameter(event, kEventParamKeyCode, typeUInt32, NULL,
                        sizeof(macKeyCode), NULL, &macKeyCode);

      if (macKeyCode < 128)
        {
        keySym = vtkMacKeyCodeToKeySymTable[macKeyCode];
        }

      SInt8 macCharCode;
      GetEventParameter(event, kEventParamKeyMacCharCodes, typeChar, NULL,
                        sizeof(macCharCode), NULL, &macCharCode);

      charCode = static_cast<char>(macCharCode);
      if (keySym == NULL && static_cast<unsigned char>(macCharCode) < 128)
        {
        keySym = vtkMacCharCodeToKeySymTable[macCharCode];
        }
      if (keySym == NULL)
        {
        keySym = "None";
        }

      switch (GetEventKind(event))
        {
        case kEventRawKeyDown:
          {
          me->SetKeyEventInformation(controlDown, shiftDown,
                                     charCode, 1, keySym);
          me->SetAltKey(altDown);
          me->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
          if (charCode)
            {
            me->InvokeEvent(vtkCommand::CharEvent, NULL);
            }
          result = noErr;
          break;
          }
        case kEventRawKeyRepeat:
          {
          me->SetKeyEventInformation(controlDown, shiftDown,
                                     charCode, 1, keySym);
          me->SetAltKey(altDown);
          me->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
          if (charCode)
            {
            me->InvokeEvent(vtkCommand::CharEvent, NULL);
            }
          result = noErr;
          break;
          }
        case kEventRawKeyUp:
          {
          me->SetKeyEventInformation(controlDown, shiftDown,
                                     charCode, 1, keySym);
          me->SetAltKey(altDown);
          me->InvokeEvent(vtkCommand::KeyReleaseEvent, NULL);
          result = noErr;
          break;
          }
        case kEventRawKeyModifiersChanged:
          {
          int oldControlDown = me->GetControlKey();
          int oldShiftDown = me->GetShiftKey();
          int oldAltDown = me->GetAltKey();

          int keyPress = 0;
          charCode = '\0';
          if (controlDown != oldControlDown)
            {
            keySym = "Control_L";
            keyPress = oldControlDown = controlDown;
            }
          else if (shiftDown != oldShiftDown)
            {
            keySym = "Shift_L";
            keyPress = oldShiftDown = shiftDown;
            }
          else if (altDown != oldAltDown)
            {
            keySym = "Alt_L";
            keyPress = oldAltDown = altDown;
            }
          else
            {
            break;
            }

          me->SetKeyEventInformation(oldControlDown, oldShiftDown,
                                     charCode, 1, keySym);
          me->SetAltKey(oldAltDown);

          if (keyPress)
            {
            me->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
            }
          else
            {
            me->InvokeEvent(vtkCommand::KeyReleaseEvent, NULL);
            }
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
      HIViewRef root_window = HIViewGetRoot(ren->GetRootWindow());
      HIViewGetViewForMouseEvent(root_window, event, &view_for_mouse);
      if(view_for_mouse != ren->GetWindowId())
        return eventNotHandledErr;

      GetEventParameter(event, kEventParamWindowMouseLocation, typeHIPoint,
                        NULL, sizeof(HIPoint), NULL, &mouseLoc);
      
      HIViewConvertPoint(&mouseLoc, root_window, ren->GetWindowId());

      UInt16 buttonNumber;
      GetEventParameter(event, kEventParamMouseButton, typeMouseButton, NULL,
                        sizeof(buttonNumber), NULL, &buttonNumber);
      
      me->SetEventInformationFlipY(int(mouseLoc.x), int(mouseLoc.y),
                                   controlDown, shiftDown);
      me->SetAltKey(altDown);
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
  if (this->HasObserver(vtkCommand::StartEvent) && !this->HandleEventLoop)
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
  ren = static_cast<vtkCarbonRenderWindow *>(this->RenderWindow);

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
    EventTypeSpec viewEventList[] = {
      { kEventClassControl, kEventControlDraw },
      { kEventClassControl, kEventControlBoundsChanged },
    };
    
    EventTypeSpec windowEventList[] = {
      { kEventClassMouse, kEventMouseDown },
      { kEventClassMouse, kEventMouseUp },
      { kEventClassMouse, kEventMouseMoved },
      { kEventClassMouse, kEventMouseDragged },
      { kEventClassMouse, kEventMouseWheelMoved },
      { kEventClassKeyboard, kEventRawKeyDown },
      { kEventClassKeyboard, kEventRawKeyRepeat },
      { kEventClassKeyboard, kEventRawKeyUp },
      { kEventClassKeyboard, kEventRawKeyModifiersChanged },
    };
    
    this->WindowProcUPP = NewEventHandlerUPP(myWinEvtHndlr);
    this->ViewProcUPP = NewEventHandlerUPP(myWinEvtHndlr);
    if (!this->WindowProcUPP || !ViewProcUPP)
      {
      err = memFullErr;
      }

    if (!err)
      {
      vtkCarbonRenderWindow* renWin =
        static_cast<vtkCarbonRenderWindow*>(this->RenderWindow);

      err = InstallControlEventHandler(
        renWin->GetWindowId(), this->ViewProcUPP,
        GetEventTypeCount(viewEventList), viewEventList, renWin, NULL);

      err = InstallWindowEventHandler(
        renWin->GetRootWindow(), this->WindowProcUPP,
        GetEventTypeCount(windowEventList), windowEventList, renWin, NULL);
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
  QuitApplicationEventLoop();
}

//--------------------------------------------------------------------------
pascal void TimerAction(EventLoopTimerRef platformTimerId, void* userData)
{
  if (NULL != userData)
    {
    vtkCarbonRenderWindowInteractor *rwi =
      static_cast<vtkCarbonRenderWindowInteractor *>(userData);
    int vtkTimerId = rwi->GetVTKTimerId((int) platformTimerId);
    rwi->InvokeEvent(vtkCommand::TimerEvent, (void *) &vtkTimerId);
    }
}

//--------------------------------------------------------------------------
int vtkCarbonRenderWindowInteractor::InternalCreateTimer(
  int vtkNotUsed(timerId), int timerType, unsigned long duration)
{
  EventLoopTimerRef  platformTimerId;
  EventLoopRef       mainLoop = GetMainEventLoop();
  EventLoopTimerUPP  timerUPP = NewEventLoopTimerUPP(TimerAction);
  EventTimerInterval interval = 0;

  // Carbon's InstallEventLoopTimer can create either one-shot or repeating
  // timers... interval == 0 indicates a one-shot timer.

  if (RepeatingTimer == timerType)
    {
    interval = duration*kEventDurationMillisecond;
    }

  InstallEventLoopTimer(mainLoop,
                        duration*kEventDurationMillisecond,
                        interval,
                        timerUPP,
                        this,
                        &platformTimerId);

  return (int) platformTimerId;
}

//--------------------------------------------------------------------------
int vtkCarbonRenderWindowInteractor::InternalDestroyTimer(int platformTimerId)
{
  RemoveEventLoopTimer((EventLoopTimerRef) platformTimerId);
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
