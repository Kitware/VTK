/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDummyCommunicator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDummyCommunicator.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkDummyCommunicator);

//-----------------------------------------------------------------------------
vtkDummyCommunicator::vtkDummyCommunicator()
{
  this->MaximumNumberOfProcesses = 1;
}

vtkDummyCommunicator::~vtkDummyCommunicator()
{
}

void vtkDummyCommunicator::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
