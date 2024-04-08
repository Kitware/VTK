// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGenerateIds.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkHyperTreeGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkGenerateIds);

//------------------------------------------------------------------------------
int vtkGenerateIds::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataSet* inputDS = vtkDataSet::GetData(inInfo);
  vtkDataSet* outputDS = vtkDataSet::GetData(outInfo);

  if (inputDS && outputDS)
  {
    // First, copy the input to the output as a starting point
    outputDS->ShallowCopy(inputDS);

    // Loop over points (if requested) and generate ids
    vtkIdType numPts = inputDS->GetNumberOfPoints();
    if (this->PointIds && numPts > 0)
    {
      vtkPointData* outputPD = outputDS->GetPointData();
      this->GeneratePointIds(outputPD, numPts);
    }

    // Loop over cells (if requested) and generate ids
    vtkIdType numCells = inputDS->GetNumberOfCells();
    if (this->CellIds && numCells > 0)
    {
      vtkCellData* outputCD = outputDS->GetCellData();
      this->GenerateCellIds(outputCD, numCells);
    }

    return 1;
  }

  vtkHyperTreeGrid* inputHTG = vtkHyperTreeGrid::GetData(inInfo);
  vtkHyperTreeGrid* outputHTG = vtkHyperTreeGrid::GetData(outInfo);

  if (inputHTG && outputHTG)
  {
    // First, copy the input to the output as a starting point;
    outputHTG->ShallowCopy(inputHTG);

    // Loop over cells and generate ids
    vtkIdType numCells = inputHTG->GetNumberOfCells();
    if (numCells > 0)
    {
      vtkCellData* outputCD = outputHTG->GetCellData();
      this->GenerateCellIds(outputCD, numCells);
    }

    return 1;
  }

  vtkErrorMacro(<< "Unable to retrieve input / output as supported type.\n");
  return 0;
}

//------------------------------------------------------------------------------
int vtkGenerateIds::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}

//------------------------------------------------------------------------------
void vtkGenerateIds::GeneratePointIds(vtkPointData* outputPD, vtkIdType numPts)
{
  vtkNew<vtkIdTypeArray> ptIds;
  ptIds->SetNumberOfValues(numPts);

  for (vtkIdType id = 0; id < numPts; id++)
  {
    ptIds->SetValue(id, id);
  }

  ptIds->SetName(this->PointIdsArrayName.c_str());
  if (!this->FieldData)
  {
    int idx = outputPD->AddArray(ptIds);
    outputPD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    outputPD->CopyScalarsOff();
  }
  else
  {
    outputPD->AddArray(ptIds);
    outputPD->CopyFieldOff(this->PointIdsArrayName.c_str());
  }
}

//------------------------------------------------------------------------------
void vtkGenerateIds::GenerateCellIds(vtkCellData* outputCD, vtkIdType numCells)
{
  vtkNew<vtkIdTypeArray> cellIds;
  cellIds->SetNumberOfValues(numCells);

  for (vtkIdType id = 0; id < numCells; id++)
  {
    cellIds->SetValue(id, id);
  }

  cellIds->SetName(this->CellIdsArrayName.c_str());
  if (!this->FieldData)
  {
    int idx = outputCD->AddArray(cellIds);
    outputCD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    outputCD->CopyScalarsOff();
  }
  else
  {
    outputCD->AddArray(cellIds);
    outputCD->CopyFieldOff(this->CellIdsArrayName.c_str());
  }
}

//------------------------------------------------------------------------------
void vtkGenerateIds::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Point Ids: " << (this->PointIds ? "On\n" : "Off\n");
  os << indent << "Cell Ids: " << (this->CellIds ? "On\n" : "Off\n");
  os << indent << "Field Data: " << (this->FieldData ? "On\n" : "Off\n");
  os << indent << "PointIdsArrayName: "
     << (this->PointIdsArrayName.empty() ? "(none)" : this->PointIdsArrayName) << "\n";
  os << indent
     << "CellIdsArrayName: " << (this->CellIdsArrayName.empty() ? "(none)" : this->CellIdsArrayName)
     << "\n";
}
VTK_ABI_NAMESPACE_END
