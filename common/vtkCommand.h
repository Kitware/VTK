/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCommand.h
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
// .NAME vtkCommand - super class for callback/ observer commands
// .SECTION Description
// vtkCommand is an implementation of the command design pattern which
// is used in callbacks (such as Start, End, Progress) in VTK. vtkObject
// implements a Subject/Observer pattern. When a subject needs to notify a
// observer, it does so using a vtkCommand. 


//BTX - begin tcl exclude

class vtkObject;

// The superclass which all commands should be subclasses of
class vtkCommand
{
public:
  virtual ~vtkCommand() {};
  void Delete() {delete this;};
  virtual void Execute(vtkObject *caller, void *callData) = 0;

  // all the currently defined events
  // developers can use -- vtkCommand::UserEvent + int to
  // specify their own events. 
  enum EventIds {
    NoEvent = 0,
    DeleteEvent,
    StartEvent,
    EndEvent,
    ProgressEvent,
    PickEvent,
    AbortCheckEvent,
    UserEvent = 1000
  };
    
  static unsigned long GetEventIdFromString(const char *event)
    {  
      if (!strcmp("DeleteEvent",event))
        {
        return vtkCommand::DeleteEvent;
        }
      if (!strcmp("StartEvent",event))
        {
        return vtkCommand::StartEvent;
        }
      if (!strcmp("EndEvent",event))
        {
        return vtkCommand::EndEvent;
        }
      if (!strcmp("ProgressEvent",event))
        {
        return vtkCommand::ProgressEvent;
        }
      if (!strcmp("PickEvent",event))
        {
        return vtkCommand::PickEvent;
        }
      if (!strcmp("AbortCheckEvent",event))
        {
        return vtkCommand::AbortCheckEvent;
        }
      if (!strcmp("UserEvent",event))
        {
        return vtkCommand::UserEvent;
        }
      return vtkCommand::NoEvent;
    };
};

// a good command to use for generic function callbacks
// the function should have the format 
// void func(vtkObject *,void *clientdata, void *calldata)
class vtkCallbackCommand : public vtkCommand
{
public:
  vtkCallbackCommand() { this->ClientData = NULL;
  this->Callback = NULL; this->ClientDataDeleteCallback = NULL;};
  ~vtkCallbackCommand() 
    { 
      if (this->ClientDataDeleteCallback)
        {
        this->ClientDataDeleteCallback(this->ClientData);
        }
    };
  void SetClientData(void *cd) {this->ClientData = cd;};
  void SetCallback(void (*f)(vtkObject *, void *, void *)) 
    {this->Callback = f;};
  void SetClientDataDeleteCallback(void (*f)(void *))
    {this->ClientDataDeleteCallback = f;};
  
  void Execute(vtkObject *caller, void *callData)
    {
      if (this->Callback)
        {
        this->Callback(caller, this->ClientData, callData);
        }
    };
  
  void *ClientData;
  void (*Callback)(vtkObject *, void *, void *);
  void (*ClientDataDeleteCallback)(void *);
};


// the old style void fund(void *) callbacks
class vtkOldStyleCallbackCommand : public vtkCommand
{
public:
  vtkOldStyleCallbackCommand() { this->ClientData = NULL;
  this->Callback = NULL; this->ClientDataDeleteCallback = NULL;};
  ~vtkOldStyleCallbackCommand() 
    { 
      if (this->ClientDataDeleteCallback)
        {
        this->ClientDataDeleteCallback(this->ClientData);
        }
    };
  void SetClientData(void *cd) {this->ClientData = cd;};
  void SetCallback(void (*f)(void *)) {this->Callback = f;};
  void SetClientDataDeleteCallback(void (*f)(void *))
    {this->ClientDataDeleteCallback = f;};
  
  void Execute(vtkObject *,void *)
    {
      if (this->Callback)
        {
        this->Callback(this->ClientData);
        }
    };
  
  void *ClientData;
  void (*Callback)(void *);
  void (*ClientDataDeleteCallback)(void *);
};


//ETX end tcl exclude
