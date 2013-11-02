/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPProgressObserver.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPProgressObserver.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMPProgressObserver);

vtkSMPProgressObserver::vtkSMPProgressObserver()
{
}

vtkSMPProgressObserver::~vtkSMPProgressObserver()
{
}

void vtkSMPProgressObserver::UpdateProgress(double progress)
{
  vtkProgressObserver* observer = this->Observers.Local();
  observer->UpdateProgress(progress);
}

void vtkSMPProgressObserver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
