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
// vtkCommand is an implementation of the command design pattern that is used
// in callbacks (such as StartMethod(), ProgressMethod(), and EndMethod()) in
// VTK. vtkObject implements a Subject/Observer pattern. When a subject needs
// to notify a observer, it does so using a vtkCommand.

#ifndef __vtkCommand_h
#define __vtkCommand_h

#include "vtkWin32Header.h"

class vtkObject;

// The superclass that all commands should be subclasses of
class VTK_COMMON_EXPORT vtkCommand
{
public:
  void Delete() {
    this->UnRegister(); }

  // Description:
  // Increase the reference count (mark as used by another object).
  void Register();
  void Register(vtkObject *) {
    this->Register(); }
  
  // Description:
  // Decrease the reference count (release by another object). This has
  // the same effect as invoking Delete() (i.e., it reduces the reference
  // count by 1).
  void UnRegister();
  void UnRegister(vtkObject *) {
    this->UnRegister(); }
  
  virtual void Execute(vtkObject *caller, unsigned long, void *callData) = 0;
  static const char *GetStringFromEventId(unsigned long event);
  static unsigned long GetEventIdFromString(const char *event);

//BTX
  // all the currently defined events
  // developers can use -- vtkCommand::UserEvent + int to
  // specify their own events. 
  // if this list is adjusted, be sure to adjust vtkCommandEventStrings
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
    UserEvent = 1000
  };
//ETX
protected:
  int ReferenceCount;      // Number of uses of this object by other objects
  vtkCommand() { this->ReferenceCount = 1;};
  virtual ~vtkCommand() {};
};

#endif /* __vtkCommand_h */
 
