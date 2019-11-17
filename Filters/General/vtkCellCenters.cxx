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
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"

vtkStandardNewMacro(vtkCellCenters);

namespace
{

class CellCenterFunctor
{
public:
  void operator()(vtkIdType begin, vtkIdType end)
  {
    if (this->DataSet == nullptr)
    {
      return;
    }

    if (this->CellCenters == nullptr)
    {
      return;
    }

    std::vector<double> weights(this->DataSet->GetMaxCellSize());
    vtkNew<vtkGenericCell> cell;
    for (vtkIdType cellId = begin; cellId < end; ++cellId)
    {
      this->DataSet->GetCell(cellId, cell);
      double x[3] = { 0.0 };
      if (cell->GetCellType() != VTK_EMPTY_CELL)
      {
        double pcoords[3];
        int subId = cell->GetParametricCenter(pcoords);
        cell->EvaluateLocation(subId, pcoords, x, weights.data());
      }
      else
      {
        x[0] = 0.0;
        x[1] = 0.0;
        x[2] = 0.0;
      }
      this->CellCenters->SetTypedTuple(cellId, x);
    }
  }

  vtkDataSet* DataSet;
  vtkDoubleArray* CellCenters;
};

} // end anonymous namespace

//----------------------------------------------------------------------------
void vtkCellCenters::ComputeCellCenters(vtkDataSet* dataset, vtkDoubleArray* centers)
{
  CellCenterFunctor functor;
  functor.DataSet = dataset;
  functor.CellCenters = centers;

  // Call this once one the main thread before calling on multiple threads.
  // According to the documentation for vtkDataSet::GetCell(vtkIdType, vtkGenericCell*),
  // this is required to make this call subsequently thread safe
  if (dataset->GetNumberOfCells() > 0)
  {
    vtkNew<vtkGenericCell> cell;
    dataset->GetCell(0, cell);
  }

  // Now split the work among threads.
  vtkSMPTools::For(0, dataset->GetNumberOfCells(), functor);
}

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
  newPts->SetDataTypeToDouble();
  newPts->SetNumberOfPoints(numCells);
  vtkDoubleArray* pointArray = vtkDoubleArray::SafeDownCast(newPts->GetData());

  vtkNew<vtkIdList> pointIdList;
  pointIdList->SetNumberOfIds(numCells);

  vtkNew<vtkIdList> cellIdList;
  cellIdList->SetNumberOfIds(numCells);

  vtkCellCenters::ComputeCellCenters(input, pointArray);

  // Remove points that would have been produced by empty cells
  // This should be multithreaded someday
  bool hasEmptyCells = false;
  vtkTypeBool abort = 0;
  vtkIdType progressInterval = numCells / 10 + 1;
  vtkIdType numPoints = 0;
  for (vtkIdType cellId = 0; cellId < numCells && !abort; ++cellId)
  {
    if (!(cellId % progressInterval))
    {
      vtkDebugMacro(<< "Processing #" << cellId);
      this->UpdateProgress((0.5 * cellId / numCells) + 0.5);
      abort = this->GetAbortExecute();
    }

    if (input->GetCellType(cellId) != VTK_EMPTY_CELL)
    {
      newPts->SetPoint(numPoints, newPts->GetPoint(cellId));
      pointIdList->SetId(numPoints, numPoints);
      cellIdList->SetId(numPoints, cellId);
      numPoints++;
    }
    else
    {
      hasEmptyCells = true;
    }
  }

  if (abort)
  {
    return 0;
  }

  newPts->Resize(numPoints);
  pointIdList->Resize(numPoints);
  cellIdList->Resize(numPoints);
  output->SetPoints(newPts);

  if (this->CopyArrays)
  {
    if (hasEmptyCells)
    {
      outPD->CopyAllocate(inCD, numPoints);
      outPD->CopyData(inCD, cellIdList, pointIdList);
    }
    else
    {
      outPD->PassData(inCD); // because number of points == number of cells
    }
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
    verts->AllocateEstimate(numPoints, 1);
    verts->ImportLegacyFormat(iArray);
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
  os << indent << "CopyArrays: " << (this->CopyArrays ? "On" : "Off") << endl;
}
