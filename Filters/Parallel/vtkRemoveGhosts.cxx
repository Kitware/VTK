/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRemoveGhosts.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

#include "vtkRemoveGhosts.h"

#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkThreshold.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <vector>

vtkStandardNewMacro(vtkRemoveGhosts);

//-----------------------------------------------------------------------------
vtkRemoveGhosts::vtkRemoveGhosts()
{
}

//-----------------------------------------------------------------------------
vtkRemoveGhosts::~vtkRemoveGhosts()
{
}

//-----------------------------------------------------------------------------
void vtkRemoveGhosts::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkRemoveGhosts::RequestUpdateExtent(vtkInformation *vtkNotUsed(request),
                                         vtkInformationVector **vtkNotUsed(inputVector),
                                         vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

  return 1;
}

//-----------------------------------------------------------------------------
int vtkRemoveGhosts::RequestData(vtkInformation *vtkNotUsed(request),
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector)
{
  vtkDebugMacro("RequestData");

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkDataSet *input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkUnsignedCharArray* ghostArray = vtkUnsignedCharArray::SafeDownCast(
    input->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName()));
  if (ghostArray == nullptr)
  {
    // no ghost information so can just shallow copy input
    output->ShallowCopy(input);
    output->GetPointData()->RemoveArray(vtkDataSetAttributes::GhostArrayName());
    return 1;
  }

  if (ghostArray && ghostArray->GetValueRange()[1] == 0)
  {
    // we have ghost cell arrays but there are no ghost entities so we just need
    // to remove those arrays and can skip modifying the data set itself
    output->ShallowCopy(input);
    output->GetPointData()->RemoveArray(vtkDataSetAttributes::GhostArrayName());
    output->GetCellData()->RemoveArray(vtkDataSetAttributes::GhostArrayName());
    return 1;
  }

  if (vtkUnstructuredGrid* ugInput = vtkUnstructuredGrid::SafeDownCast(input))
  {
    vtkNew<vtkThreshold> threshold;
    threshold->SetInputData(ugInput);
    threshold->ThresholdByLower(0);
    threshold->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, vtkDataSetAttributes::GhostArrayName());
    threshold->Update();
    output->ShallowCopy(threshold->GetOutput());
    output->GetCellData()->RemoveArray(vtkDataSetAttributes::GhostArrayName());
    output->GetPointData()->RemoveArray(vtkDataSetAttributes::GhostArrayName());
    return 1;
  }

  return this->ProcessPolyData(
    vtkPolyData::SafeDownCast(input), vtkPolyData::SafeDownCast(output), ghostArray);
}

//----------------------------------------------------------------------------
int vtkRemoveGhosts::ProcessPolyData(
  vtkPolyData* input, vtkPolyData* output, vtkUnsignedCharArray* ghostCells)
{
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData(), *outCD=output->GetCellData();

  outPD->CopyGlobalIdsOn();
  outPD->CopyAllocate(pd);
  outCD->CopyGlobalIdsOn();
  outCD->CopyAllocate(cd);

  vtkIdType numPts = input->GetNumberOfPoints();
  output->Allocate(input->GetNumberOfCells());

  vtkNew<vtkPoints> newPoints;
  newPoints->SetDataType(input->GetPoints()->GetDataType());

  newPoints->Allocate(numPts);

  std::vector<vtkIdType> pointMap(numPts, -1);

  vtkNew<vtkIdList> newCellPts;

  // Check that the scalars of each cell satisfy the threshold criterion
  for (vtkIdType cellId=0; cellId < input->GetNumberOfCells(); cellId++)
  {
    vtkCell* cell = input->GetCell(cellId);
    vtkIdList* cellPts = cell->GetPointIds();
    int numCellPts = cell->GetNumberOfPoints();
    if (numCellPts > 0 && ghostCells->GetValue(cellId) == 0)
    {
      for (int i=0; i < numCellPts; i++)
      {
        int ptId = cellPts->GetId(i);
        vtkIdType newId = pointMap[ptId];
        if ( newId < 0 )
        {
          double x[3];
          input->GetPoint(ptId, x);
          newId = newPoints->InsertNextPoint(x);
          pointMap[ptId] = newId;
          outPD->CopyData(pd,ptId,newId);
        }
        newCellPts->InsertId(i,newId);
      }
      vtkIdType newCellId = output->InsertNextCell(cell->GetCellType(),newCellPts.GetPointer());
      outCD->CopyData(cd,cellId,newCellId);
      newCellPts->Reset();
    }
  }
  output->SetPoints(newPoints.GetPointer());
  output->Squeeze();
  outPD->RemoveArray(vtkDataSetAttributes::GhostArrayName());
  outCD->RemoveArray(vtkDataSetAttributes::GhostArrayName());

  return 1;
}

//----------------------------------------------------------------------------
int vtkRemoveGhosts::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}
