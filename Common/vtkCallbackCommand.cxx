/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCallbackCommand.cxx
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

#include <string.h>
#include <ctype.h>
#include "vtkCallbackCommand.h"

//----------------------------------------------------------------
vtkCallbackCommand::vtkCallbackCommand() 
{ 
  this->ClientData = NULL;
  this->Callback = NULL; 
  this->ClientDataDeleteCallback = NULL;
}

vtkCallbackCommand::~vtkCallbackCommand() 
{ 
  if (this->ClientDataDeleteCallback)
    {
    this->ClientDataDeleteCallback(this->ClientData);
    }
}

void vtkCallbackCommand::Execute(vtkObject *caller, unsigned long event,
                                 void *callData)
{
  if (this->Callback)
    {
    this->Callback(caller, event, this->ClientData, callData);
    }
}

