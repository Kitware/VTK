// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRotationFilter.h"

#include "vtkCellData.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTransform.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkRotationFilter);

//------------------------------------------------------------------------------
vtkRotationFilter::vtkRotationFilter()
{
  this->Axis = 2;
  this->CopyInput = 0;
  this->Center[0] = this->Center[1] = this->Center[2] = 0;
  this->NumberOfCopies = 0;
  this->Angle = 0;
}

//------------------------------------------------------------------------------
vtkRotationFilter::~vtkRotationFilter() = default;

//------------------------------------------------------------------------------
void vtkRotationFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Axis: " << this->Axis << endl;
  os << indent << "CopyInput: " << this->CopyInput << endl;
  os << indent << "Center: (" << this->Center[0] << "," << this->Center[1] << "," << this->Center[2]
     << ")" << endl;
  os << indent << "NumberOfCopies: " << this->NumberOfCopies << endl;
  os << indent << "Angle: " << this->Angle << endl;
}

//------------------------------------------------------------------------------
int vtkRotationFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();

  if (!this->GetNumberOfCopies())
  {
    vtkErrorMacro("No number of copy set!");
    return 1;
  }

  double tuple[3];
  vtkPoints* outPoints;
  double point[3], center[3], negativCenter[3];
  vtkIdType ptId, cellId;
  vtkGenericCell* cell = vtkGenericCell::New();
  vtkIdList* ptIds = vtkIdList::New();

  outPoints = vtkPoints::New();

  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();

  if (this->CopyInput)
  {
    outPoints->Allocate((this->CopyInput + this->GetNumberOfCopies()) * numPts);
    output->Allocate((this->CopyInput + this->GetNumberOfCopies()) * numPts);
  }
  else
  {
    outPoints->Allocate(this->GetNumberOfCopies() * numPts);
    output->Allocate(this->GetNumberOfCopies() * numPts);
  }

  outPD->CopyAllocate(inPD);
  outCD->CopyAllocate(inCD);

  vtkDataArray *inPtVectors, *outPtVectors;
  vtkDataArray *inCellVectors, *outCellVectors;

  inPtVectors = inPD->GetVectors();
  outPtVectors = outPD->GetVectors();
  inCellVectors = inCD->GetVectors();
  outCellVectors = outCD->GetVectors();

  // Copy first points.
  if (this->CopyInput)
  {
    for (vtkIdType i = 0; i < numPts; ++i)
    {
      input->GetPoint(i, point);
      ptId = outPoints->InsertNextPoint(point);
      outPD->CopyData(inPD, i, ptId);
    }
  }
  vtkTransform* localTransform = vtkTransform::New();
  // Rotate points.
  this->GetCenter(center);
  negativCenter[0] = -center[0];
  negativCenter[1] = -center[1];
  negativCenter[2] = -center[2];

  for (int k = 0; k < this->GetNumberOfCopies(); ++k)
  {
    localTransform->Identity();
    localTransform->Translate(center);
    switch (this->Axis)
    {
      case USE_X:
        localTransform->RotateX((k + 1) * this->GetAngle());
        break;

      case USE_Y:
        localTransform->RotateY((k + 1) * this->GetAngle());
        break;

      case USE_Z:
        localTransform->RotateZ((k + 1) * this->GetAngle());
        break;
    }
    localTransform->Translate(negativCenter);
    for (vtkIdType i = 0; i < numPts; ++i)
    {
      input->GetPoint(i, point);
      localTransform->TransformPoint(point, point);
      ptId = outPoints->InsertNextPoint(point);
      outPD->CopyData(inPD, i, ptId);
      if (inPtVectors)
      {
        inPtVectors->GetTuple(i, tuple);
        outPtVectors->SetTuple(ptId, tuple);
      }
    }
  }

  localTransform->Delete();

  // Copy original cells.
  if (this->CopyInput)
  {
    for (vtkIdType i = 0; i < numCells; ++i)
    {
      input->GetCellPoints(i, ptIds);
      output->InsertNextCell(input->GetCellType(i), ptIds);
      outCD->CopyData(inCD, i, i);
    }
  }

  vtkIdType* newCellPts;
  vtkIdList* cellPts;

  // Generate rotated cells.
  for (int k = 0; k < this->GetNumberOfCopies() && !this->CheckAbort(); ++k)
  {
    for (vtkIdType i = 0; i < numCells; ++i)
    {
      if (this->CheckAbort())
      {
        break;
      }
      input->GetCellPoints(i, ptIds);
      input->GetCell(i, cell);
      vtkIdType numCellPts = cell->GetNumberOfPoints();
      int cellType = cell->GetCellType();
      cellPts = cell->GetPointIds();

      vtkDebugMacro(<< "celltype " << cellType << " numCellPts " << numCellPts);
      newCellPts = new vtkIdType[numCellPts];
      for (vtkIdType j = 0; j < numCellPts; ++j)
      {
        newCellPts[j] = cellPts->GetId(j) + numPts * k;
        if (this->CopyInput)
        {
          newCellPts[j] += numPts;
        }
      }

      cellId = output->InsertNextCell(cellType, numCellPts, newCellPts);
      delete[] newCellPts;
      outCD->CopyData(inCD, i, cellId);
      if (inCellVectors)
      {
        inCellVectors->GetTuple(i, tuple);
        outCellVectors->SetTuple(cellId, tuple);
      }
    }
  }

  cell->Delete();
  ptIds->Delete();
  output->SetPoints(outPoints);
  outPoints->Delete();
  output->CheckAttributes();

  return 1;
}

int vtkRotationFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}
VTK_ABI_NAMESPACE_END
