/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPIGroup.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMPIGroup.h"
#include "vtkMPIController.h"
#include "vtkObjectFactory.h"
#include "vtkProcessGroup.h"

#ifndef VTK_REMOVE_LEGACY_CODE

vtkStandardNewMacro(vtkMPIGroup);

void vtkMPIGroup::PrintSelf(ostream& os, vtkIndent indent)
{
  VTK_LEGACY_BODY(PrintSelf, "5.2");

  this->Superclass::PrintSelf(os,indent);
  os << indent << "Initialized : ";
  if (this->Initialized)
    {
    os << "(yes)" << endl;
    }
  else
    {
    os << "(no)" << endl;
    }
  os << indent << "Maximum number of processe ids: " 
     << this->MaximumNumberOfProcessIds << endl;
  os << indent << "First available position: " 
     << this->CurrentPosition << endl;
  for(int i=0; i < this->CurrentPosition; i++)
    {
    os << indent << "Process id at " << i << " is " 
       << this->ProcessIds[i]  << endl;
    }
  return;
}

vtkMPIGroup::vtkMPIGroup()
{
  VTK_LEGACY_BODY(vtkMPIGroup, "5.2");

  this->ProcessIds = 0;
  this->MaximumNumberOfProcessIds = 0;
  this->CurrentPosition = 0;
  this->Initialized = 0;
}

vtkMPIGroup::~vtkMPIGroup()
{
  VTK_LEGACY_BODY(~vtkMPIGroup, "5.2");

  delete[] this->ProcessIds;
  this->MaximumNumberOfProcessIds = 0;
}

void vtkMPIGroup::Initialize(int numProcIds)
{
  VTK_LEGACY_BODY(Initialize, "5.2");

  if (this->Initialized)
    {
    return;
    }

  if (this->ProcessIds)
    {
    delete[] this->ProcessIds;
    }

  this->MaximumNumberOfProcessIds = numProcIds;
  if ( this->MaximumNumberOfProcessIds > 0 )
    {
    this->ProcessIds = new int[this->MaximumNumberOfProcessIds];
    }
  else
    {
    return;
    }

  this->Initialized = 1;
  this->Modified();
  return;
}

void vtkMPIGroup::Initialize(vtkMPIController* controller)
{
  VTK_LEGACY_BODY(Initialize, "5.2");

  this->Initialize(controller->GetNumberOfProcesses());
  return;
}

int vtkMPIGroup::AddProcessId(int processId)
{
  VTK_LEGACY_BODY(AddProcessId, "5.2");

  if ( this->CurrentPosition >= this->MaximumNumberOfProcessIds )
    {
    vtkWarningMacro("Can not add any more process ids. The group is full.");
    return 0;
    }
  if ( processId >= this->MaximumNumberOfProcessIds )
    {
    vtkWarningMacro("Process id " << processId << " is not valid.");
    return 0;
    }
  if ( this->FindProcessId(processId) >= 0 )
    {
    vtkWarningMacro("This process id is already in the group.");
    return 0;
    }
  
  this->ProcessIds[this->CurrentPosition] = processId;
  this->Modified();
  return ++this->CurrentPosition;
}

void vtkMPIGroup::RemoveProcessId(int processId)
{
  VTK_LEGACY_BODY(RemoveProcessId, "5.2");

  int pos = this->FindProcessId(processId);
  if ( pos >= 0 )
    {
    for(int i=pos; i < this->CurrentPosition - 1; i++)
      {
      this->ProcessIds[i] = this->ProcessIds[i+1];
      }
    this->CurrentPosition--;
    this->Modified();
    }
  return;
}

int vtkMPIGroup::FindProcessId(int processId)
{
  VTK_LEGACY_BODY(FindProcessId, "5.2");

  for (int i=0; i < this->CurrentPosition; i++)
    {
    if ( this->ProcessIds[i] == processId )
      {
      return i;
      }
    }
  return -1;
}

int vtkMPIGroup::GetProcessId(int pos)
{
  VTK_LEGACY_BODY(GetProcessId, "5.2");

  if ( pos >= this->CurrentPosition)
    {
    return -1;
    }
  return this->ProcessIds[pos];
}

void vtkMPIGroup::CopyProcessIdsFrom(vtkMPIGroup* group)
{
  VTK_LEGACY_BODY(CopyProcessIdsFrom, "5.2");

  int max;
  // Find which MaximumNumberOfProcessIds is smallest and use that.
  if ( this->MaximumNumberOfProcessIds > group->MaximumNumberOfProcessIds )
    {
    max = group->MaximumNumberOfProcessIds;
    }
  else
    {
    max = this->MaximumNumberOfProcessIds;
    }
  // Copy.
  for(int i=0; i < max; i++)
    {
    this->ProcessIds[i] = group->ProcessIds[i];
    }

  // If the current position of group is smaller than 
  // this->MaximumNumberOfProcessIds use it.
  if ( group->CurrentPosition <= this->MaximumNumberOfProcessIds)
    {
    this->CurrentPosition = group->CurrentPosition;
    }
  else
    {
    this->CurrentPosition = this->MaximumNumberOfProcessIds;
    }

  this->Modified();
  return;
}

void vtkMPIGroup::CopyFrom(vtkMPIGroup* source)
{
  this->Initialized = 0;
  this->Initialize(source->MaximumNumberOfProcessIds);
  this->CopyProcessIdsFrom(source);
}

int vtkMPIGroup::GetNumberOfProcessIds()
{
  VTK_LEGACY_BODY(GetNumberOfProcessIds, "5.2");
  return this->CurrentPosition;
}

void vtkMPIGroup::CopyInto(vtkProcessGroup *destination,
                           vtkMPICommunicator *mpiComm)
{
  destination->SetCommunicator(mpiComm);
  for (int i = 0; i < this->GetNumberOfProcessIds(); i++)
    {
    destination->AddProcessId(this->GetProcessId(i));
    }
}

#endif //VTK_REMOVE_LEGACY_CODE
