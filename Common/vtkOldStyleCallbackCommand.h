/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOldStyleCallbackCommand.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOldStyleCallbackCommand - supports legacy function callbacks for VTK
// .SECTION Description
// vtkOldStyleCallbackCommand is a callback that supports the legacy callback
// methods found in VTK. For example, the legacy method
// vtkProcessObject::SetStartMethod() is actually invoked using the
// command/observer design pattern of VTK, and the vtkOldStyleCallbackCommand
// is used to provide the legacy functionality. The callback function should
// have the form void func(void *clientdata), where clientdata is special data
// that should is associated with this instance of vtkCallbackCommand.
//
// .SECTION Caveats
// This is legacy glue. Please do not use; it will be eventually eliminated.

// .SECTION See Also
// vtkCommand vtkCallbackCommand

#ifndef __vtkOldStyleCallbackCommand_h
#define __vtkOldStyleCallbackCommand_h

#include "vtkCommand.h"

// the old style void fund(void *) callbacks
class VTK_COMMON_EXPORT vtkOldStyleCallbackCommand : public vtkCommand
{
public:
  static vtkOldStyleCallbackCommand *New() 
    {return new vtkOldStyleCallbackCommand;};

  // Description:
  // Satisfy the superclass API for callbacks.
  void Execute(vtkObject *invoker, unsigned long eid, void *calldata);

  // Description:
  // Methods to set and get client and callback information.
  void SetClientData(void *cd) 
    {this->ClientData = cd;};
  void SetCallback(void (*f)(void *clientdata)) 
    {this->Callback = f;};
  void SetClientDataDeleteCallback(void (*f)(void *))
    {this->ClientDataDeleteCallback = f;};
  
  void *ClientData;
  void (*Callback)(void *);
  void (*ClientDataDeleteCallback)(void *);

protected:
  vtkOldStyleCallbackCommand();
  ~vtkOldStyleCallbackCommand();
};


#endif /* __vtkOldStyleCallbackCommand_h */
 
