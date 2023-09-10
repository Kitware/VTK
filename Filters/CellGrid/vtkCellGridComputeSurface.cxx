// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellGridComputeSurface.h"

#include "vtkCellGridSidesQuery.h"
#include "vtkDataSetAttributes.h"
#include "vtkFiltersCellGrid.h" // for RegisterCellsAndResponders()
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals; // for ""_token

vtkStandardNewMacro(vtkCellGridComputeSurface);

vtkCellGridComputeSurface::vtkCellGridComputeSurface()
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();
}

void vtkCellGridComputeSurface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Request:\n"; // Print the request configuration.
  vtkIndent i2 = indent.GetNextIndent();
  this->Request->PrintSelf(os, i2);
}

void vtkCellGridComputeSurface::SetPreserveRenderableCells(bool preserve)
{
  this->Request->SetPreserveRenderableCells(preserve);
}

bool vtkCellGridComputeSurface::GetPreserveRenderableCells()
{
  return this->Request->GetPreserveRenderableCells();
}

vtkStringToken vtkCellGridComputeSurface::GetSideAttribute()
{
  return vtkStringToken("Sides");
}

int vtkCellGridComputeSurface::RequestData(
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
  output->ShallowCopy(input);
  if (!output->Query(this->Request))
  {
    vtkErrorMacro("Input failed to respond to query.");
    return 0;
  }
  return 1;
}

VTK_ABI_NAMESPACE_END
