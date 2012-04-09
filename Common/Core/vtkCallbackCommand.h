/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCallbackCommand.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCallbackCommand - supports function callbacks
// .SECTION Description
// Use vtkCallbackCommand for generic function callbacks. That is, this class
// can be used when you wish to execute a function (of the signature
// described below) using the Command/Observer design pattern in VTK.
// The callback function should have the form
// <pre>
// void func(vtkObject*, unsigned long eid, void* clientdata, void *calldata)
// </pre>
// where the parameter vtkObject* is the object invoking the event; eid is
// the event id (see vtkCommand.h); clientdata is special data that should
// is associated with this instance of vtkCallbackCommand; and calldata is
// data that the vtkObject::InvokeEvent() may send with the callback. For
// example, the invocation of the ProgressEvent sends along the progress
// value as calldata.
//

// .SECTION See Also
// vtkCommand vtkOldStyleCallbackCommand

#ifndef __vtkCallbackCommand_h
#define __vtkCallbackCommand_h

#include "vtkCommand.h"

class VTK_COMMON_EXPORT vtkCallbackCommand : public vtkCommand
{
public:
  vtkTypeMacro(vtkCallbackCommand,vtkCommand);

  static vtkCallbackCommand *New()
    {return new vtkCallbackCommand;};

  // Description:
  // Satisfy the superclass API for callbacks. Recall that the caller is
  // the instance invoking the event; eid is the event id (see
  // vtkCommand.h); and calldata is information sent when the callback
  // was invoked (e.g., progress value in the vtkCommand::ProgressEvent).
  virtual void Execute(vtkObject *caller, unsigned long eid, void *callData);

  // Description:
  // Methods to set and get client and callback information, and the callback
  // function.
  virtual void SetClientData(void *cd)
    { this->ClientData = cd; }
  virtual void* GetClientData()
    { return this->ClientData; }
  virtual void SetCallback(void (*f)(vtkObject *caller, unsigned long eid,
                                     void *clientdata, void *calldata))
    { this->Callback = f; }
  virtual void SetClientDataDeleteCallback(void (*f)(void *))
    { this->ClientDataDeleteCallback = f; }

  // Description:
  // Set/Get the abort flag on execute. If this is set to true the AbortFlag
  // will be set to On automatically when the Execute method is triggered *and*
  // a callback is set.
  void SetAbortFlagOnExecute(int f)
    { this->AbortFlagOnExecute = f; }
  int GetAbortFlagOnExecute()
    { return this->AbortFlagOnExecute; }
  void AbortFlagOnExecuteOn()
    { this->SetAbortFlagOnExecute(1); }
  void AbortFlagOnExecuteOff()
    { this->SetAbortFlagOnExecute(0); }

  void (*Callback)(vtkObject *, unsigned long, void *, void *);
  void (*ClientDataDeleteCallback)(void *);

protected:

  int AbortFlagOnExecute;
  void *ClientData;

  vtkCallbackCommand();
  ~vtkCallbackCommand();
};

#endif

