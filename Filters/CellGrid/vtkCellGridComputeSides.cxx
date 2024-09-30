// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellGridComputeSides.h"

#include "vtkCellGridSidesCache.h"
#include "vtkCellGridSidesQuery.h"
#include "vtkDataSetAttributes.h"
#include "vtkFiltersCellGrid.h" // for RegisterCellsAndResponders()
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals; // for ""_token

vtkStandardNewMacro(vtkCellGridComputeSides);

vtkCellGridComputeSides::vtkCellGridComputeSides()
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();
  // For now, just keep the cached data on the filter.
  // Eventually, a side-cache should be stored in the
  // vtkCellGridResponders map of cached objects with
  // a key appropriate to the input data object.
  vtkNew<vtkCellGridSidesCache> sideCache;
  this->Request->SetSideCache(sideCache);
}

void vtkCellGridComputeSides::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Request:\n"; // Print the request configuration.
  vtkIndent i2 = indent.GetNextIndent();
  this->Request->PrintSelf(os, i2);
}

void vtkCellGridComputeSides::SetPreserveRenderableInputs(bool preserve)
{
  bool amPreserving = this->Request->GetPreserveRenderableInputs();
  if (amPreserving == preserve)
  {
    return;
  }
  this->Request->SetPreserveRenderableInputs(preserve);
  this->Modified();
}

bool vtkCellGridComputeSides::GetPreserveRenderableInputs()
{
  return this->Request->GetPreserveRenderableInputs();
}

void vtkCellGridComputeSides::SetOmitSidesForRenderableInputs(bool omit)
{
  bool amOmitting = this->Request->GetOmitSidesForRenderableInputs();
  if (amOmitting == omit)
  {
    return;
  }
  this->Request->SetOmitSidesForRenderableInputs(omit);
  this->Modified();
}

bool vtkCellGridComputeSides::GetOmitSidesForRenderableInputs()
{
  return this->Request->GetOmitSidesForRenderableInputs();
}

void vtkCellGridComputeSides::SetOutputDimensionControl(int flags)
{
  if (this->Request->GetOutputDimensionControl() == flags)
  {
    return;
  }
  this->Request->SetOutputDimensionControl(flags);
  this->Modified();
}

int vtkCellGridComputeSides::GetOutputDimensionControl()
{
  return this->Request->GetOutputDimensionControl();
}

void vtkCellGridComputeSides::SetStrategy(SummaryStrategy strategy)
{
  this->Request->SetStrategy(strategy);
}

vtkCellGridComputeSides::SummaryStrategy vtkCellGridComputeSides::GetStrategy()
{
  return this->Request->GetStrategy();
}

void vtkCellGridComputeSides::SetSelectionType(SelectionMode selectionType)
{
  this->Request->SetSelectionType(selectionType);
}

vtkCellGridComputeSides::SelectionMode vtkCellGridComputeSides::GetSelectionType()
{
  return this->Request->GetSelectionType();
}

void vtkCellGridComputeSides::SetStrategy(int strategy)
{
  auto strat = static_cast<SummaryStrategy>(strategy);
  if (strat == this->GetStrategy())
  {
    return;
  }
  this->SetStrategy(strat);
  this->Modified();
}

void vtkCellGridComputeSides::SetSelectionType(int selnType)
{
  auto stype = static_cast<SelectionMode>(selnType);
  if (stype == this->GetSelectionType())
  {
    return;
  }
  this->SetSelectionType(stype);
  this->Modified();
}

vtkStringToken vtkCellGridComputeSides::GetSideAttribute()
{
  return vtkStringToken("Sides");
}

int vtkCellGridComputeSides::RequestData(
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
  // TODO: For now, always reset the side cache.
  // In the future, the cache should invalidate
  // itself as the query parameters are modified.
  this->Request->GetSideCache()->Initialize();
  if (!output->Query(this->Request))
  {
    vtkErrorMacro("Input failed to respond to query.");
    return 0;
  }
  return 1;
}

VTK_ABI_NAMESPACE_END
