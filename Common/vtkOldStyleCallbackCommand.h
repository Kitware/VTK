/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOldStyleCallbackCommand.h
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
// .NAME vtkOldStyleCallbackCommand - supports old-style function callbacks for VTK
// .SECTION Description
// vtkOldStyleCallbackCommand is a callback that supports the legacy callbcak
// methods found in VTK. For example, the legacy method
// vtkProcessObject::SetStartMethod() is actually invoked using the
// command/observer design pattern of VTK, and the vtkOldStyleCallbackCommand
// is used to provide the legacy functionality. The function should have the
// format void func(vtkObject *,void *clientdata, void *calldata).

// .SECTION See Also
// vtkCommand vtkCallCommand

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
  void Execute(vtkObject *,unsigned long, void *);

  // Description:
  // Methods to set and get client and callback information.
  void SetClientData(void *cd) 
    {this->ClientData = cd;};
  void SetCallback(void (*f)(void *)) 
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
 
