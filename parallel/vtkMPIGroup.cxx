/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPIGroup.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkMPIGroup.h"
#include "vtkMPIController.h"
#include "vtkObjectFactory.h"

vtkMPIGroup* vtkMPIGroup::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMPIGroup");
  if(ret)
    {
    return (vtkMPIGroup*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMPIGroup;
}

void vtkMPIGroup::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent);
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
  this->ProcessIds = 0;
  this->MaximumNumberOfProcessIds = 0;
  this->CurrentPosition = 0;
  this->Initialized = 0;
}

vtkMPIGroup::~vtkMPIGroup()
{
  delete[] this->ProcessIds;
  this->MaximumNumberOfProcessIds = 0;
}

void vtkMPIGroup::Initialize(int numProcIds)
{
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
  this->Initialize(controller->GetNumberOfProcesses());
  return;
}

int vtkMPIGroup::AddProcessId(int processId)
{
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
  else
    {
    this->ProcessIds[this->CurrentPosition] = processId;
    return ++this->CurrentPosition;
    }

  this->Modified();
}

void vtkMPIGroup::RemoveProcessId(int processId)
{
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
  if ( pos >= this->CurrentPosition)
    {
    return -1;
    }
  return this->ProcessIds[pos];
}

void vtkMPIGroup::CopyProcessIdsFrom(vtkMPIGroup* group)
{
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
