/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCallbackCommand.h
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
  static vtkCallbackCommand *New() 
    {return new vtkCallbackCommand;};

  // Description:
  // Satisfy the superclass API for callbacks. Recall that the caller is
  // the instance invoking the event; eid is the event id (see 
  // vtkCommand.h); and calldata is information sent when the callback
  // was invoked (e.g., progress value in the vtkCommand::ProgressEvent).
  void Execute(vtkObject *caller, unsigned long eid, void *callData);

  // Description:
  // Methods to set and get client and callback information, and the callback
  // function.
  void SetClientData(void *cd) 
    {this->ClientData = cd;}
  void* GetClientData()
    {return this->ClientData; }
  void SetCallback(void (*f)(vtkObject *caller, unsigned long eid, 
                             void *clientdata, void *calldata)) 
    {this->Callback = f;}
  void SetClientDataDeleteCallback(void (*f)(void *))
    {this->ClientDataDeleteCallback = f;}
  
  void *ClientData;
  void (*Callback)(vtkObject *, unsigned long, void *, void *);
  void (*ClientDataDeleteCallback)(void *);

protected:
  vtkCallbackCommand();
  ~vtkCallbackCommand();
};



#endif /* __vtkCallbackCommand_h */
 
