/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCommand.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
 
