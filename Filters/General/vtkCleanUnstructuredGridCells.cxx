/*=========================================================================

Program:   ParaView
Module:    vtkCleanUnstructuredGridCells.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCleanUnstructuredGridCells.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCollection.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"

#include <set>

vtkStandardNewMacro(vtkCleanUnstructuredGridCells);

//----------------------------------------------------------------------------
vtkCleanUnstructuredGridCells::vtkCleanUnstructuredGridCells() = default;

//----------------------------------------------------------------------------
vtkCleanUnstructuredGridCells::~vtkCleanUnstructuredGridCells() = default;

//----------------------------------------------------------------------------
void vtkCleanUnstructuredGridCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkCleanUnstructuredGridCells::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkUnstructuredGrid* input =
    vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (input->GetNumberOfCells() == 0)
  {
    // set up a ugrid with same data arrays as input, but
    // no points, cells or data.
    output->Allocate(1);
    output->GetPointData()->CopyAllocate(input->GetPointData(), VTK_CELL_SIZE);
    vtkPoints* pts = vtkPoints::New();
    pts->SetDataTypeToDouble();

    output->SetPoints(pts);
    pts->Delete();
    return 1;
  }

  // Copy over the original points. Assume there are no degenerate points.
  output->SetPoints(input->GetPoints());

  output->GetPointData()->ShallowCopy(input->GetPointData());

  vtkCellData* outCD = output->GetCellData();
  outCD->CopyGlobalIdsOn();
  outCD->CopyAllocate(input->GetCellData());

  // remove duplicate cells
  std::set<std::set<int>> cellSet;
  std::set<std::set<int>>::iterator cellIter;

  // Now copy the cells.
  vtkNew<vtkIdList> cellPoints;
  const vtkIdType numberOfCells = input->GetNumberOfCells();
  vtkIdType progressStep = numberOfCells / 100;
  if (progressStep == 0)
  {
    progressStep = 1;
  }

  output->Allocate(numberOfCells);
  int ndeg = 0;
  int ndup = 0;

  for (vtkIdType id = 0; id < numberOfCells; id++)
  {
    if (id % progressStep == 0)
    {
      this->UpdateProgress(0.8 + 0.2 * (static_cast<float>(id) / numberOfCells));
    }

    // duplicate points do not make poly vertices or triangle
    // strips degenerate so don't remove them
    int cellType = input->GetCellType(id);
    if (cellType == VTK_POLY_VERTEX || cellType == VTK_TRIANGLE_STRIP)
    {
      input->GetCellPoints(id, cellPoints);
      vtkIdType newCellId = output->InsertNextCell(cellType, cellPoints);
      outCD->CopyData(input->GetCellData(), id, newCellId);
      continue;
    }

    input->GetCellPoints(id, cellPoints);
    std::set<int> nn;
    for (int i = 0; i < cellPoints->GetNumberOfIds(); i++)
    {
      int cellPtId = cellPoints->GetId(i);
      nn.insert(cellPtId);
    }

    // this conditional may generate non-referenced nodes
    cellIter = cellSet.find(nn);

    // only copy a cell to the output if it is neither degenerate nor duplicate
    if (nn.size() == static_cast<unsigned int>(cellPoints->GetNumberOfIds()) &&
      cellIter == cellSet.end())
    {
      vtkIdType newCellId = output->InsertNextCell(input->GetCellType(id), cellPoints);
      outCD->CopyData(input->GetCellData(), id, newCellId);
      cellSet.insert(nn);
    }
    else if (nn.size() != static_cast<unsigned int>(cellPoints->GetNumberOfIds()))
    {
      ndeg++; // a node appeared more than once in a cell
    }
    else if (cellIter != cellSet.end())
    {
      ndup++; // cell has duplicate(s)
    }
  }
  if (ndeg)
  {
    vtkDebugMacro(<< "vtkCleanUnstructuredGridCells : WARNING, " << ndeg
                  << " degenerated cells (i.e. cells with coincident nodes) have been"
                  << " removed, which may result in disconnected nodes. It is"
                  << " recommended to clean the grid." << endl);
  }

  if (ndup)
  {
    vtkDebugMacro(<< "vtkCleanUnstructuredGridCells : " << ndup
                  << " duplicate cells (multiple instances of a cell) have been"
                  << " removed." << endl);

    output->Squeeze();
  }

  return 1;
}

int vtkCleanUnstructuredGridCells::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}
