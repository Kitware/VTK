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
// vtkCallbackCommand is a good command to use for generic function 
// callbacks. The function should have the format 
// void func(vtkObject *,void *clientdata, void *calldata). 

// .SECTION See Also
// vtkCommand vtkCallCommand

#ifndef __vtkCallbackCommand_h
#define __vtkCallbackCommand_h

#include "vtkCommand.h"

class VTK_COMMON_EXPORT vtkCallbackCommand : public vtkCommand
{
public:
  static vtkCallbackCommand *New() 
    {return new vtkCallbackCommand;};

  // Description:
  // Satisfy the superclass API for callbacks.
  void Execute(vtkObject *caller, unsigned long event, void *callData);

  // Description:
  // Methods to set and get client and callback information.
  void SetClientData(void *cd) 
    {this->ClientData = cd;}
  void* GetClientData()
    {return this->ClientData; }
  void SetCallback(void (*f)(vtkObject *, unsigned long, void *, void *)) 
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
 
