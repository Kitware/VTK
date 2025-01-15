// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPAxisAlignedReflectionFilter.h"

#include "vtkBoundingBox.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPAxisAlignedReflectionFilter);
vtkCxxSetObjectMacro(vtkPAxisAlignedReflectionFilter, Controller, vtkMultiProcessController);
//------------------------------------------------------------------------------
vtkPAxisAlignedReflectionFilter::vtkPAxisAlignedReflectionFilter()
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkPAxisAlignedReflectionFilter::~vtkPAxisAlignedReflectionFilter()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
void vtkPAxisAlignedReflectionFilter::ComputeBounds(vtkDataObject* input, double bounds[6])
{
  vtkBoundingBox bbox;
  this->Superclass::ComputeBounds(input, bounds);
  bbox.SetBounds(bounds);

  if (this->Controller)
  {
    this->Controller->GetCommunicator()->ComputeGlobalBounds(
      this->Controller->GetLocalProcessId(), this->Controller->GetNumberOfProcesses(), &bbox);
    bbox.GetBounds(bounds);
  }
}

//------------------------------------------------------------------------------
void vtkPAxisAlignedReflectionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
VTK_ABI_NAMESPACE_END
