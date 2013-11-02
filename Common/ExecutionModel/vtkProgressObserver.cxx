/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgressObserver.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProgressObserver.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkProgressObserver);

vtkProgressObserver::vtkProgressObserver()
{
}

vtkProgressObserver::~vtkProgressObserver()
{
}

void vtkProgressObserver::UpdateProgress(double amount)
{
  this->Progress = amount;
  this->InvokeEvent(vtkCommand::ProgressEvent,static_cast<void *>(&amount));
}

void vtkProgressObserver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
