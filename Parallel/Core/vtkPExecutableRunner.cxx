// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPExecutableRunner.h"

#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPExecutableRunner);

void vtkPExecutableRunner::Execute()
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  int localProcessId = controller->GetLocalProcessId();
  if (localProcessId != this->ExecutionProcessId && this->ExecutionProcessId != -1)
  {
    return;
  }

  vtkLogF(TRACE, "Executing command %s on rank %i", this->GetCommand(), localProcessId);
  this->Superclass::Execute();
}

void vtkPExecutableRunner::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ExecutionProcessId: " << this->ExecutionProcessId << std::endl;
}

VTK_ABI_NAMESPACE_END
