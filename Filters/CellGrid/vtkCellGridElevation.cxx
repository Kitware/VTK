// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellGridElevation.h"

#include "vtkCellAttribute.h"
#include "vtkCellGridElevationQuery.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGridElevation);

void vtkCellGridElevation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkCellGridElevation::RequestData(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** inInfo, vtkInformationVector* ouInfo)
{
  auto* input = vtkCellGrid::GetData(inInfo[0]);
  auto* output = vtkCellGrid::GetData(ouInfo);
  if (!input)
  {
    vtkWarningMacro("Empty input.");
    return 1;
  }
  if (!output)
  {
    vtkErrorMacro("Empty output.");
    return 0;
  }

  // Copy the input; we'll add the new vtkCellAttribute later.
  output->ShallowCopy(input);

  // For now, copy the filter parameters to the request.
  this->Request->Name = this->AttributeName;
  for (int ii = 0; ii < 3; ++ii)
  {
    this->Request->Origin[ii] = this->Origin[ii];
    this->Request->Axis[ii] = this->Axis[ii];
  }
  this->Request->NumberOfAxes = this->NumberOfAxes;
  this->Request->Shock = this->Shock;

  // Run the query on the request.
  if (!output->Query(this->Request))
  {
    vtkErrorMacro("Input failed to respond to query.");
    return 0;
  }

  // We must copy the request's elevation attribute since, when we
  // add it to the output cell-grid, the attribute is modified.
  // We also don't want re-running the filter to modify some
  // grid created by this filter in the past. However, we should
  // not need to deep-copy the attribute (which might deep-copy
  // the large arrays).
  vtkNew<vtkCellAttribute> elevation;
  elevation->DeepCopy(this->Request->Elevation);
  output->AddCellAttribute(elevation);
  vtkIndent indent;
  output->PrintSelf(std::cout, indent);

  return 1;
}

VTK_ABI_NAMESPACE_END
