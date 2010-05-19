// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSubCommunicator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkSubCommunicator.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProcessGroup.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSubCommunicator);

//-----------------------------------------------------------------------------
vtkSubCommunicator::vtkSubCommunicator()
{
  this->Group = NULL;
}

vtkSubCommunicator::~vtkSubCommunicator()
{
  this->SetGroup(NULL);
}

void vtkSubCommunicator::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Group: " << this->Group << endl;
}

//-----------------------------------------------------------------------------
int vtkSubCommunicator::SendVoidArray(const void *data, vtkIdType length,
                                      int type, int remoteHandle, int tag)
{
  int realHandle = this->Group->GetProcessId(remoteHandle);
  return this->Group->GetCommunicator()->SendVoidArray(data, length, type,
                                                       realHandle, tag);
}

//-----------------------------------------------------------------------------
int vtkSubCommunicator::ReceiveVoidArray(void *data, vtkIdType length,
                                         int type, int remoteHandle, int tag)
{
  int realHandle;
  if (remoteHandle == vtkMultiProcessController::ANY_SOURCE)
    {
    realHandle = vtkMultiProcessController::ANY_SOURCE;
    }
  else
    {
    realHandle = this->Group->GetProcessId(remoteHandle);
    }
  return this->Group->GetCommunicator()->ReceiveVoidArray(data, length, type,
                                                          realHandle, tag);
}

//-----------------------------------------------------------------------------
void vtkSubCommunicator::SetGroup(vtkProcessGroup *group)
{
  vtkSetObjectBodyMacro(Group, vtkProcessGroup, group);

  if (this->Group)
    {
    this->LocalProcessId = this->Group->GetLocalProcessId();
    if (this->MaximumNumberOfProcesses != this->Group->GetNumberOfProcessIds())
      {
      this->NumberOfProcesses = this->MaximumNumberOfProcesses
        = this->Group->GetNumberOfProcessIds();
      }
    }
  else
    {
    this->LocalProcessId = -1;
    this->NumberOfProcesses = 0;
    this->MaximumNumberOfProcesses = 0;
    }
}
