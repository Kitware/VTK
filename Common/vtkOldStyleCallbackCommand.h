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
// .NAME vtkOldStyleCallbackCommand - old syntax for callbacks

#ifndef __vtkOldStyleCallbackCommand_h
#define __vtkOldStyleCallbackCommand_h

#include "vtkCommand.h"

// the old style void fund(void *) callbacks
class VTK_COMMON_EXPORT vtkOldStyleCallbackCommand : public vtkCommand
{
public:
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
protected:
  vtkOldStyleCallbackCommand();
  ~vtkOldStyleCallbackCommand();
};


#endif /* __vtkOldStyleCallbackCommand_h */
 
