// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProcessGroup.cxx

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

#include "vtkProcessGroup.h"

#include "vtkCommunicator.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

#include <algorithm>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkProcessGroup);

//-----------------------------------------------------------------------------
vtkProcessGroup::vtkProcessGroup()
{
  this->Communicator = NULL;

  this->ProcessIds = NULL;
  this->NumberOfProcessIds = 0;
}

vtkProcessGroup::~vtkProcessGroup()
{
  this->SetCommunicator(NULL);
}

void vtkProcessGroup::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Communicator: " << this->Communicator << endl;
  os << indent << "ProcessIds:";
  for (int i = 0; i < this->NumberOfProcessIds; i++)
  {
    os << " " << this->ProcessIds[i];
  }
  os << endl;
}

//-----------------------------------------------------------------------------
void vtkProcessGroup::Initialize(vtkMultiProcessController *controller)
{
  this->Initialize(controller->GetCommunicator());
}

void vtkProcessGroup::Initialize(vtkCommunicator *communicator)
{
  this->SetCommunicator(communicator);

  this->NumberOfProcessIds = this->Communicator->GetNumberOfProcesses();
  for (int i = 0; i < this->NumberOfProcessIds; i++)
  {
    this->ProcessIds[i] = i;
  }
}

//-----------------------------------------------------------------------------
void vtkProcessGroup::SetCommunicator(vtkCommunicator *communicator)
{
  // Adjust ProcessIds array.
  int *newProcessIds = NULL;
  int newNumberOfProcessIds = 0;
  if (communicator)
  {
    newProcessIds = new int[communicator->GetNumberOfProcesses()];
    newNumberOfProcessIds = communicator->GetNumberOfProcesses();
    if (newNumberOfProcessIds > this->NumberOfProcessIds)
    {
      newNumberOfProcessIds = this->NumberOfProcessIds;
    }
  }
  if (this->ProcessIds)
  {
    std::copy(newProcessIds, newProcessIds+newNumberOfProcessIds,
               this->ProcessIds);
  }
  if (this->Communicator) delete[] this->ProcessIds;
  this->ProcessIds = newProcessIds;
  this->NumberOfProcessIds = newNumberOfProcessIds;

  vtkSetObjectBodyMacro(Communicator, vtkCommunicator, communicator);
}

//-----------------------------------------------------------------------------
int vtkProcessGroup::GetLocalProcessId()
{
  if (this->Communicator)
  {
    return this->FindProcessId(this->Communicator->GetLocalProcessId());
  }
  else
  {
    return -1;
  }
}

//-----------------------------------------------------------------------------
int vtkProcessGroup::FindProcessId(int processId)
{
  for (int i = 0; i < this->NumberOfProcessIds; i++)
  {
    if (this->ProcessIds[i] == processId) return i;
  }
  return -1;
}

//-----------------------------------------------------------------------------
int vtkProcessGroup::AddProcessId(int processId)
{
  int loc = this->FindProcessId(processId);
  if (loc < 0)
  {
    loc = this->NumberOfProcessIds++;
    this->ProcessIds[loc] = processId;
    this->Modified();
  }
  return loc;
}

//-----------------------------------------------------------------------------
int vtkProcessGroup::RemoveProcessId(int processId)
{
  int loc = this->FindProcessId(processId);
  if (loc < 0) return 0;

  this->NumberOfProcessIds--;
  for (int i = loc; i < this->NumberOfProcessIds; i++)
  {
    this->ProcessIds[i] = this->ProcessIds[i+1];
  }
  this->Modified();
  return 1;
}

//-----------------------------------------------------------------------------
void vtkProcessGroup::RemoveAllProcessIds()
{
  if (this->NumberOfProcessIds > 0)
  {
    this->NumberOfProcessIds = 0;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkProcessGroup::Copy(vtkProcessGroup *group)
{
  this->SetCommunicator(group->Communicator);
  this->NumberOfProcessIds = group->NumberOfProcessIds;
  for (int i = 0; i < this->NumberOfProcessIds; i++)
  {
    this->ProcessIds[i] = group->ProcessIds[i];
  }
}
