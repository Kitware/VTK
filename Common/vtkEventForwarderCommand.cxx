/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEventForwarderCommand.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkEventForwarderCommand.h"
#include "vtkObject.h"

//----------------------------------------------------------------
vtkEventForwarderCommand::vtkEventForwarderCommand() 
{ 
  this->Target = NULL;
}

//----------------------------------------------------------------
void vtkEventForwarderCommand::Execute(vtkObject *, 
                                       unsigned long event,
                                       void *call_data)
{
  if (this->Target)
    {
    this->Target->InvokeEvent(event, call_data);
    }
}

