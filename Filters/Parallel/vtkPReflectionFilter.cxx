// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPReflectionFilter.h"

#include "vtkBoundingBox.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPReflectionFilter);
vtkCxxSetObjectMacro(vtkPReflectionFilter, Controller, vtkMultiProcessController);
//------------------------------------------------------------------------------
vtkPReflectionFilter::vtkPReflectionFilter()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkPReflectionFilter::~vtkPReflectionFilter()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
int vtkPReflectionFilter::ComputeBounds(vtkDataObject* input, double bounds[6])
{
  vtkBoundingBox bbox;

  if (this->Superclass::ComputeBounds(input, bounds))
  {
    bbox.SetBounds(bounds);
  }

  if (this->Controller)
  {
    this->Controller->GetCommunicator()->ComputeGlobalBounds(
      this->Controller->GetLocalProcessId(), this->Controller->GetNumberOfProcesses(), &bbox);
    bbox.GetBounds(bounds);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkPReflectionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
VTK_ABI_NAMESPACE_END
