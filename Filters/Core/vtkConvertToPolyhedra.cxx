// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkConvertToPolyhedra.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkConvertToPolyhedra);

//------------------------------------------------------------------------------
vtkConvertToPolyhedra::vtkConvertToPolyhedra()
{
  this->OutputAllCells = false;
}

//------------------------------------------------------------------------------
// A simple method that converts linear 3D cells into polyhedra.
int vtkConvertToPolyhedra::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkUnstructuredGrid* input =
    vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();

  if (numPts <= 0 || numCells <= 0)
  {
    vtkLog(INFO, "Empty input");
    return 1;
  }

  // Points and attribute data are simply copied through
  output->SetPoints(input->GetPoints());
  output->GetPointData()->ShallowCopy(input->GetPointData());

  // Because of potential selective output of cells, we cannot
  // shallow copy the cell data.
  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();
  outCD->CopyAllocate(inCD);

  // Now loop over all cells and those that are appropriate are converted to
  // polyhedron.
  output->Allocate(numCells);
  vtkNew<vtkGenericCell> cell;
  std::vector<vtkIdType> faces;
  int cellType = VTK_POLYHEDRON;
  vtkIdType outCellId;
  vtkIdType checkAbortInterval = std::min(numCells / 10 + 1, (vtkIdType)1000);
  for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
  {
    if (cellId % checkAbortInterval == 0 && this->CheckAbort())
    {
      break;
    }
    // Grab the input cell.
    input->GetCell(cellId, cell);

    // Identify cells that cannot be readily converted to polyhedra.
    // Depending on the user specification, either skip them, or trivially
    // copy them to the output.
    if (cell->GetCellDimension() < 3 || !cell->IsLinear())
    {
      if (!this->OutputAllCells)
      {
        continue;
      }
      else
      {
        outCellId = output->InsertNextCell(cell->GetCellType(), cell->PointIds);
        outCD->CopyData(inCD, cellId, outCellId);
      }
    }

    // Process faces. Use the original cell's point ids to create the new
    // polyhedral cell, and add in the cell's faces.
    int numFaces = cell->GetNumberOfFaces();
    faces.clear();
    for (int faceNum = 0; faceNum < numFaces; ++faceNum)
    {
      vtkCell* face = cell->GetFace(faceNum);
      vtkIdType numFacePts = face->PointIds->GetNumberOfIds();
      vtkIdType* fptr = face->PointIds->GetPointer(0);
      faces.push_back(numFacePts);
      faces.insert(faces.end(), fptr, fptr + numFacePts);
    }
    outCellId = output->InsertNextCell(cellType, cell->PointIds->GetNumberOfIds(),
      cell->PointIds->GetPointer(0), numFaces, faces.data());
    outCD->CopyData(inCD, cellId, outCellId);
  } // for all input cells

  return 1;
}

//------------------------------------------------------------------------------
void vtkConvertToPolyhedra::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Output All Cells: " << (this->OutputAllCells ? "true\n" : "false\n");
}
VTK_ABI_NAMESPACE_END
