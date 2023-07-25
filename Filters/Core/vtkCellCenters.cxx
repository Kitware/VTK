// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellCenters.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkUnsignedCharArray.h"

#include <atomic>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCellCenters);

namespace
{

class CellCenterFunctor
{
  vtkSMPThreadLocalObject<vtkGenericCell> TLCell;
  vtkSMPThreadLocal<std::vector<double>> TLWeigths;
  vtkDataSet* DataSet;
  vtkDoubleArray* CellCenters;
  vtkIdType MaxCellSize;

public:
  CellCenterFunctor(vtkDataSet* ds, vtkDoubleArray* cellCenters)
    : DataSet(ds)
    , CellCenters(cellCenters)
    , MaxCellSize(ds->GetMaxCellSize())
  {
  }

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

    auto& weights = this->TLWeigths.Local();
    weights.resize(this->MaxCellSize);
    auto cell = this->TLCell.Local();
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
};

//==============================================================================
struct InputGhostCellFinder
{
  InputGhostCellFinder(vtkUnsignedCharArray* ghostCells, vtkIdList* cellIdList)
    : GhostCells(ghostCells)
    , CellIdList(cellIdList)
    , HasInputGhostCells(false)
  {
  }

  void operator()(vtkIdType startId, vtkIdType endId)
  {
    auto ghosts = vtk::DataArrayValueRange<1>(this->GhostCells);
    for (vtkIdType id = startId; id < endId; ++id)
    {
      if (this->HasInputGhostCells)
      {
        return;
      }
      if (ghosts[this->CellIdList->GetId(id)] &
        (vtkDataSetAttributes::DUPLICATECELL | vtkDataSetAttributes::HIDDENCELL |
          vtkDataSetAttributes::REFINEDCELL))
      {
        this->HasInputGhostCells = true;
      }
    }
  }

  vtkUnsignedCharArray* GhostCells;
  vtkIdList* CellIdList;
  std::atomic<bool> HasInputGhostCells;
};

//==============================================================================
struct GhostCellsToGhostPointsConverter
{
  GhostCellsToGhostPointsConverter(
    vtkUnsignedCharArray* ghostCells, vtkUnsignedCharArray* ghostPoints, vtkIdList* cellIdList)
    : GhostCells(ghostCells)
    , GhostPoints(ghostPoints)
    , CellIdList(cellIdList)
  {
  }

  void operator()(vtkIdType startId, vtkIdType endId)
  {
    auto ghostPoints = vtk::DataArrayValueRange<1>(this->GhostPoints);
    auto ghostCells = vtk::DataArrayValueRange<1>(this->GhostCells);
    for (vtkIdType id = startId; id < endId; ++id)
    {
      unsigned char ghost = ghostCells[this->CellIdList->GetId(id)];
      ghostPoints[id] = 0;
      if (ghost & vtkDataSetAttributes::DUPLICATECELL)
      {
        ghostPoints[id] |= vtkDataSetAttributes::DUPLICATEPOINT;
      }
      if (ghost & (vtkDataSetAttributes::HIDDENCELL | vtkDataSetAttributes::REFINEDCELL))
      {
        ghostPoints[id] |= vtkDataSetAttributes::HIDDENPOINT;
      }
    }
  }

  vtkUnsignedCharArray* GhostCells;
  vtkUnsignedCharArray* GhostPoints;
  vtkIdList* CellIdList;
};

} // end anonymous namespace

//------------------------------------------------------------------------------
void vtkCellCenters::ComputeCellCenters(vtkDataSet* dataset, vtkDoubleArray* centers)
{
  CellCenterFunctor functor(dataset, centers);

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

//------------------------------------------------------------------------------
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
      abort = this->CheckAbort();
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
    return 1;
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

  if (vtkUnsignedCharArray* inputGhostCells = input->GetCellData()->GetGhostArray())
  {
    ::InputGhostCellFinder finder(inputGhostCells, cellIdList);
    vtkSMPTools::For(0, numPoints, finder);
    if (finder.HasInputGhostCells)
    {
      vtkNew<vtkUnsignedCharArray> ghostPoints;
      ghostPoints->SetNumberOfValues(numPoints);
      ghostPoints->SetName(vtkDataSetAttributes::GhostArrayName());

      ::GhostCellsToGhostPointsConverter worker(inputGhostCells, ghostPoints, cellIdList);
      vtkSMPTools::For(0, numPoints, worker);
      outPD->AddArray(ghostPoints);
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

//------------------------------------------------------------------------------
int vtkCellCenters::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkCellCenters::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Vertex Cells: " << (this->VertexCells ? "On\n" : "Off\n");
  os << indent << "CopyArrays: " << (this->CopyArrays ? "On" : "Off") << endl;
}
VTK_ABI_NAMESPACE_END
