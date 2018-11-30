/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellCenters.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellCenters.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkCellCenters);

//----------------------------------------------------------------------------
// Generate points
int vtkCellCenters::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input and output
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkPolyData* output = vtkPolyData::GetData(outputVector);

  vtkCellData* inCD = input->GetCellData();
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* outCD = output->GetCellData();
  vtkIdType numCells = input->GetNumberOfCells();

  if (numCells == 0)
  {
    vtkDebugMacro(<< "No cells to generate center points for");
    return 1;
  }

  vtkNew<vtkPoints> newPts;
  newPts->SetNumberOfPoints(numCells);

  vtkNew<vtkIdList> pointIdList;
  pointIdList->SetNumberOfIds(numCells);

  vtkNew<vtkIdList> cellIdList;
  cellIdList->SetNumberOfIds(numCells);

  vtkIdType pointId = 0;
  vtkIdType numPoints = numCells;
  std::vector<double> weights(input->GetMaxCellSize());
  bool hasEmptyCells = false;
  vtkTypeBool abort = 0;
  vtkIdType progressInterval = numCells / 10 + 1;
  for (vtkIdType cellId = 0; cellId < numCells && !abort; cellId++)
  {
    if (!(cellId % progressInterval))
    {
      vtkDebugMacro(<< "Processing #" << cellId);
      this->UpdateProgress(0.9 * cellId / numCells);
      abort = this->GetAbortExecute();
    }

    vtkCell* cell = input->GetCell(cellId);
    if (cell->GetCellType() != VTK_EMPTY_CELL)
    {
      double x[3], pcoords[3];
      int subId = cell->GetParametricCenter(pcoords);
      cell->EvaluateLocation(subId, pcoords, x, weights.data());
      newPts->SetPoint(pointId, x);
      pointIdList->SetId(pointId, pointId);
      cellIdList->SetId(pointId, cellId);
      pointId++;
    }
    else
    {
      hasEmptyCells = true;
      numPoints--;
    }
  }
  if(abort)
  {
    return 0;
  }

  newPts->Resize(numPoints);
  pointIdList->Resize(numPoints);
  cellIdList->Resize(numPoints);
  output->SetPoints(newPts);

  if (hasEmptyCells)
  {
    outPD->CopyAllocate(inCD, numPoints);
    outPD->CopyData(inCD, cellIdList, pointIdList);
  }
  else
  {
    outPD->PassData(inCD); // because number of points == number of cells
  }

  if (this->VertexCells)
  {
    vtkNew<vtkIdTypeArray> iArray;
    iArray->SetNumberOfComponents(1);
    iArray->SetNumberOfTuples(numPoints * 2);
    for (vtkIdType i = 0; i < numPoints; i++)
    {
      iArray->SetValue(2 * i, 1);
      iArray->SetValue(2 * i + 1, i);
    }

    vtkNew<vtkCellArray> verts;
    verts->SetCells(numPoints, iArray);
    output->SetVerts(verts);
    outCD->ShallowCopy(outPD);
  }

  output->Squeeze();
  this->UpdateProgress(1.0);
  return 1;
}

//----------------------------------------------------------------------------
int vtkCellCenters::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkCellCenters::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Vertex Cells: " << (this->VertexCells ? "On\n" : "Off\n");
}
