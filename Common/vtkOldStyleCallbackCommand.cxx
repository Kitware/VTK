/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOldStyleCallbackCommand.cxx
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
#include "vtkOldStyleCallbackCommand.h"

#include <string.h>
#include <ctype.h>

//----------------------------------------------------------------
vtkOldStyleCallbackCommand::vtkOldStyleCallbackCommand() 
{ 
  this->ClientData = NULL;
  this->Callback = NULL; 
  this->ClientDataDeleteCallback = NULL;
}
  
vtkOldStyleCallbackCommand::~vtkOldStyleCallbackCommand() 
{ 
  if (this->ClientDataDeleteCallback)
    {
    this->ClientDataDeleteCallback(this->ClientData);
    }
}
 
void vtkOldStyleCallbackCommand::Execute(vtkObject *,unsigned long, void *)
{
  if (this->Callback)
    {
    this->Callback(this->ClientData);
    }
}
