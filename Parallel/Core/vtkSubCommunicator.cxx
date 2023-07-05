// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSubCommunicator.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProcessGroup.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSubCommunicator);

//------------------------------------------------------------------------------
vtkSubCommunicator::vtkSubCommunicator()
{
  this->Group = nullptr;
}

vtkSubCommunicator::~vtkSubCommunicator()
{
  this->SetGroup(nullptr);
}

void vtkSubCommunicator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Group: " << this->Group << endl;
}

//------------------------------------------------------------------------------
int vtkSubCommunicator::SendVoidArray(
  const void* data, vtkIdType length, int type, int remoteHandle, int tag)
{
  int realHandle = this->Group->GetProcessId(remoteHandle);
  return this->Group->GetCommunicator()->SendVoidArray(data, length, type, realHandle, tag);
}

//------------------------------------------------------------------------------
int vtkSubCommunicator::ReceiveVoidArray(
  void* data, vtkIdType length, int type, int remoteHandle, int tag)
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
  return this->Group->GetCommunicator()->ReceiveVoidArray(data, length, type, realHandle, tag);
}

//------------------------------------------------------------------------------
void vtkSubCommunicator::SetGroup(vtkProcessGroup* group)
{
  vtkSetObjectBodyMacro(Group, vtkProcessGroup, group);

  if (this->Group)
  {
    this->LocalProcessId = this->Group->GetLocalProcessId();
    if (this->MaximumNumberOfProcesses != this->Group->GetNumberOfProcessIds())
    {
      this->NumberOfProcesses = this->MaximumNumberOfProcesses =
        this->Group->GetNumberOfProcessIds();
    }
  }
  else
  {
    this->LocalProcessId = -1;
    this->NumberOfProcesses = 0;
    this->MaximumNumberOfProcesses = 0;
  }
}
VTK_ABI_NAMESPACE_END
