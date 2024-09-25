// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellGridCellSource.h"

#include "vtkCellAttribute.h"
#include "vtkDataSetAttributes.h"
#include "vtkFiltersCellGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGridCellSource);
vtkStandardNewMacro(vtkCellGridCellSource::Query);

void vtkCellGridCellSource::Query::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "CellType:" << this->CellType << "\n";
}

vtkCellGridCellSource::vtkCellGridCellSource()
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();
  this->SetNumberOfInputPorts(0);
}

void vtkCellGridCellSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  vtkIndent i2 = indent.GetNextIndent();
  os << "Request:" << this->Request;
  this->Request->PrintSelf(os, i2);
}

void vtkCellGridCellSource::SetCellType(const char* cellType)
{
  if (this->Request->GetCellTypeString() != cellType)
  {
    this->Request->SetCellType(cellType);
    this->Modified();
  }
}

const char* vtkCellGridCellSource::GetCellType() const
{
  return this->Request->GetCellType();
}

int vtkCellGridCellSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inInfo), vtkInformationVector* ouInfo)
{
  auto* output = vtkCellGrid::GetData(ouInfo);
  if (!output)
  {
    vtkErrorMacro("Empty output.");
    return 0;
  }

  output->Initialize();
  output->AddAllCellMetadata();
  // Run the query on the request.
  if (!output->Query(this->Request))
  {
    vtkErrorMacro("Failed to respond to query.");
    return 0;
  }
  output->RemoveUnusedCellMetadata();

  return 1;
}

VTK_ABI_NAMESPACE_END
