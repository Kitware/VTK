/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProcess.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProcess.h"
#include "vtkMultiProcessController.h"


// ----------------------------------------------------------------------------
vtkProcess::vtkProcess()
{
  this->Controller=0;
  this->ReturnValue=0;
}

// ----------------------------------------------------------------------------
vtkMultiProcessController *vtkProcess::GetController()
{
  return this->Controller;
}

// ----------------------------------------------------------------------------
void vtkProcess::SetController(vtkMultiProcessController *aController)
{
  this->Controller=aController;
}

// ----------------------------------------------------------------------------
int vtkProcess::GetReturnValue()
{
  return this->ReturnValue;
}

//----------------------------------------------------------------------------
void vtkProcess::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ReturnValue: " << this->ReturnValue << endl;
  os << indent << "Controller: ";
  if(this->Controller)
    {
    os << endl;
    this->Controller->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}
