/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCommand.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCommand - superclass for callback/observer methods
// .SECTION Description
// vtkCommand is an implementation of the observer/command design pattern that is used
// in callbacks (such as StartMethod(), ProgressMethod(), and EndMethod()) in
// VTK. vtkObject implements a Subject/Observer pattern. When a subject needs
// to notify a observer, it does so using by invoking vtkObject::AddObserver() and
// using vtkCommand (or a derived class) to invoke the callback.
//
// Commands can be associated with vtkObject or any of its subclasses via the AddObserver()
// method. Note that event processing can be organized in priority lists, so it is possible
// to truncate the processing of a particular event by setting the AbortFlag variable.

// .SECTION See Also
// vtkObject

#ifndef __vtkCommand_h
#define __vtkCommand_h

#include "vtkWin32Header.h"

class vtkObject;

// The superclass that all commands should be subclasses of
class VTK_COMMON_EXPORT vtkCommand
{
public:
  // Description:
  // vtkCommand is not a subclass of vtkObject, but it has the same reference
  // counting semantics. So Delete(), Register(), and UnRegister() are implemented.
  void Delete() 
    { this->UnRegister(); }

  // Description:
  // Increase the reference count (mark as used by another object).
  void Register();
  void Register(vtkObject *) 
    { this->Register(); }
  
  // Description:
  // Decrease the reference count (release by another object). This has
  // the same effect as invoking Delete() (i.e., it reduces the reference
  // count by 1).
  void UnRegister();
  void UnRegister(vtkObject *) 
    { this->UnRegister(); }
  
  // Description:
  // All derived classes of vtkCommand must implement this method. This is the
  // method that actually triggers the callback.
  virtual void Execute(vtkObject *caller, unsigned long, void *callData) = 0;

  // Description:
  // Convenience methods for translating between event names and event ids.
  static const char *GetStringFromEventId(unsigned long event);
  static unsigned long GetEventIdFromString(const char *event);

  // Description:  
  // Set/Get the abort flag.  If this is set to true no further commands are
  // executed.
  void SetAbortFlag(int f)  
    { if(this->AbortFlag) *this->AbortFlag = f; }
  int GetAbortFlag() 
    { return (this->AbortFlag) ? *this->AbortFlag: 0; }
  void AbortFlagOn() 
    { this->SetAbortFlag(1); }
  void AbortFlagOff() 
    { this->SetAbortFlag(0); }
  
//BTX
  // All the currently defined events are listed here.
  // Developers can use -- vtkCommand::UserEvent + int to
  // specify their own event ids.
  // If this list is adjusted, be sure to adjust vtkCommandEventStrings
  // in vtkCommand.cxx to match.
  enum EventIds {
    NoEvent = 0,
    AnyEvent,
    DeleteEvent,
    StartEvent,
    EndEvent,
    ProgressEvent,
    PickEvent,
    StartPickEvent,
    EndPickEvent,
    AbortCheckEvent,
    ExitEvent,
    LeftButtonPressEvent,
    LeftButtonReleaseEvent,
    MiddleButtonPressEvent,
    MiddleButtonReleaseEvent,
    RightButtonPressEvent,
    RightButtonReleaseEvent,
    EnterEvent,
    LeaveEvent,
    KeyPressEvent,
    KeyReleaseEvent,
    CharEvent,
    ConfigureEvent,
    TimerEvent,
    MouseMoveEvent,
    ResetCameraEvent,
    ResetCameraClippingRangeEvent,
    ModifiedEvent,
    WindowLevelEvent,
    NextDataEvent,
    PushDataStartEvent,
    SetOutputEvent,
    EndOfDataEvent,
    ErrorEvent,
    WarningEvent,
    StartInteractionEvent,
    InteractionEvent,
    EndInteractionEvent,
    UserEvent = 1000
  };
//ETX

protected:
  int* AbortFlag;
  int ReferenceCount;      // Number of uses of this object by other objects
  vtkCommand():AbortFlag(0),ReferenceCount(1) {}
  virtual ~vtkCommand() {}

  //helper function for manipulating abort flag
  void SetAbortFlagPointer(int* f)  
    { this->AbortFlag = f; }

  friend class vtkSubjectHelper;
};

#endif /* __vtkCommand_h */
 
