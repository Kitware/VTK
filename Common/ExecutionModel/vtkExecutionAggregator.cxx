// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkExecutionAggregator.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
void vtkExecutionAggregator::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkExecutionAggregator::RequestDataObject(vtkDataObject* input)
{
  if (!input)
  {
    return nullptr;
  }
  return vtk::TakeSmartPointer(input->NewInstance());
}

VTK_ABI_NAMESPACE_END
