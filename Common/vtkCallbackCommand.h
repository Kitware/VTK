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
// .NAME vtkCallbackCommand - generic callback

#ifndef __vtkCallbackCommand_h
#define __vtkCallbackCommand_h

#include "vtkCommand.h"

// a good command to use for generic function callbacks
// the function should have the format 
// void func(vtkObject *,void *clientdata, void *calldata)
class VTK_COMMON_EXPORT vtkCallbackCommand : public vtkCommand
{
public:
  static vtkCallbackCommand *New() 
    {return new vtkCallbackCommand;};

  void SetClientData(void *cd) 
    {this->ClientData = cd;};
  void* GetClientData()
    {return this->ClientData; }
  void SetCallback(void (*f)(vtkObject *, unsigned long, void *, void *)) 
    {this->Callback = f;};
  void SetClientDataDeleteCallback(void (*f)(void *))
    {this->ClientDataDeleteCallback = f;};
  
  void Execute(vtkObject *caller, unsigned long event, void *callData);

  void *ClientData;
  void (*Callback)(vtkObject *, unsigned long, void *, void *);
  void (*ClientDataDeleteCallback)(void *);
protected:
  vtkCallbackCommand();
  ~vtkCallbackCommand();
};



#endif /* __vtkCallbackCommand_h */
 
