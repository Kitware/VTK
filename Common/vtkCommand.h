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
class VTK_EXPORT vtkCommand
{
public:
  vtkCommand() {};
  virtual ~vtkCommand() {};
  static vtkCommand *New();
  void Delete() {delete this;};

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
    UserEvent = 1000
  };
//ETX
};

//BTX - begin tcl exclude

// a good command to use for generic function callbacks
// the function should have the format 
// void func(vtkObject *,void *clientdata, void *calldata)
class VTK_EXPORT vtkCallbackCommand : public vtkCommand
{
public:
  vtkCallbackCommand();
  ~vtkCallbackCommand();
  static vtkCallbackCommand *New() 
    {return new vtkCallbackCommand;};

  void SetClientData(void *cd) 
    {this->ClientData = cd;};
  void SetCallback(void (*f)(vtkObject *, unsigned long, void *, void *)) 
    {this->Callback = f;};
  void SetClientDataDeleteCallback(void (*f)(void *))
    {this->ClientDataDeleteCallback = f;};
  
  void Execute(vtkObject *caller, unsigned long event, void *callData);

  void *ClientData;
  void (*Callback)(vtkObject *, unsigned long, void *, void *);
  void (*ClientDataDeleteCallback)(void *);
};


// the old style void fund(void *) callbacks
class VTK_EXPORT vtkOldStyleCallbackCommand : public vtkCommand
{
public:
  vtkOldStyleCallbackCommand();
  ~vtkOldStyleCallbackCommand();
  static vtkOldStyleCallbackCommand *New() 
    {return new vtkOldStyleCallbackCommand;};

  void SetClientData(void *cd) 
    {this->ClientData = cd;};
  void SetCallback(void (*f)(void *)) 
    {this->Callback = f;};
  void SetClientDataDeleteCallback(void (*f)(void *))
    {this->ClientDataDeleteCallback = f;};
  
  void Execute(vtkObject *,unsigned long, void *);

  void *ClientData;
  void (*Callback)(void *);
  void (*ClientDataDeleteCallback)(void *);
};


//ETX end tcl exclude

#endif /* __vtkCommand_h */
 
