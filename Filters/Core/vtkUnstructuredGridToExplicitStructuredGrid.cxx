/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridToExplicitStructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGridToExplicitStructuredGrid.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkExplicitStructuredGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkUnstructuredGridToExplicitStructuredGrid);

//-----------------------------------------------------------------------------
vtkUnstructuredGridToExplicitStructuredGrid::vtkUnstructuredGridToExplicitStructuredGrid()
{
  this->WholeExtent[0] = this->WholeExtent[1] = this->WholeExtent[2] = this->WholeExtent[3] =
    this->WholeExtent[4] = this->WholeExtent[5] = 0;
}

// ----------------------------------------------------------------------------
int vtkUnstructuredGridToExplicitStructuredGrid::RequestInformation(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->WholeExtent, 6);

  return 1;
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridToExplicitStructuredGrid::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Retrieve input and output
  vtkUnstructuredGrid* input = vtkUnstructuredGrid::GetData(inputVector[0], 0);
  vtkExplicitStructuredGrid* output = vtkExplicitStructuredGrid::GetData(outputVector, 0);

  if (!input)
  {
    vtkErrorMacro("No input!");
    return 0;
  }
  if (input->GetNumberOfPoints() == 0 || input->GetNumberOfCells() == 0)
  {
    return 1;
  }

  vtkDataArray* iArray = GetInputArrayToProcess(0, input);
  vtkDataArray* jArray = GetInputArrayToProcess(1, input);
  vtkDataArray* kArray = GetInputArrayToProcess(2, input);
  if (!iArray || !jArray || !kArray)
  {
    vtkErrorMacro("An ijk array has not be set using SetInputArrayToProcess, aborting.");
    return 0;
  }

  int extents[6];
  double r[2];
  iArray->GetRange(r);
  extents[0] = static_cast<int>(std::floor(r[0]));
  extents[1] = static_cast<int>(std::floor(r[1] + 1));
  jArray->GetRange(r);
  extents[2] = static_cast<int>(std::floor(r[0]));
  extents[3] = static_cast<int>(std::floor(r[1] + 1));
  kArray->GetRange(r);
  extents[4] = static_cast<int>(std::floor(r[0]));
  extents[5] = static_cast<int>(std::floor(r[1] + 1));

  vtkIdType expectedCells =
    (extents[1] - extents[0]) * (extents[3] - extents[2]) * (extents[5] - extents[4]);

  // Copy input point data to output
  output->GetCellData()->CopyAllocate(input->GetCellData(), expectedCells);
  output->GetPointData()->ShallowCopy(input->GetPointData());
  output->SetPoints(input->GetPoints());
  output->SetExtent(extents);

  vtkIdType nbCells = input->GetNumberOfCells();
  vtkNew<vtkCellArray> cells;
  output->SetCells(cells.Get());

  // Initialize the cell array
  cells->AllocateEstimate(expectedCells, 8);
  vtkIdType ids[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  for (vtkIdType i = 0; i < expectedCells; i++)
  {
    cells->InsertNextCell(8, ids);
    if (expectedCells != nbCells)
    {
      output->GetCellData()->CopyData(input->GetCellData(), 0, i);
      // Blank after copying the cell data to ensure it is not overwrited
      output->BlankCell(i);
    }
  }

  int progressCount = 0;
  int abort = 0;
  vtkIdType progressInterval = nbCells / 20 + 1;

  // Copy unstructured cells
  for (vtkIdType i = 0; i < nbCells && !abort; i++)
  {
    if (progressCount >= progressInterval)
    {
      vtkDebugMacro("Process cell #" << i);
      this->UpdateProgress(static_cast<double>(i) / nbCells);
      abort = this->GetAbortExecute();
      progressCount = 0;
    }
    progressCount++;

    int cellType = input->GetCellType(i);
    if (cellType != VTK_HEXAHEDRON && cellType != VTK_VOXEL)
    {
      vtkErrorMacro("Cell " << i << " is of type " << cellType << " while "
                            << "hexahedron or voxel is expected!");
      continue;
    }

    // Compute the structured cell index from IJK indices
    vtkIdType cellId = output->ComputeCellId(static_cast<int>(std::floor(iArray->GetTuple1(i))),
      static_cast<int>(std::floor(jArray->GetTuple1(i))),
      static_cast<int>(std::floor(kArray->GetTuple1(i))));
    if (cellId < 0)
    {
      vtkErrorMacro("Incorrect CellId, something went wrong");
      return 0;
    }

    vtkIdType npts;
    const vtkIdType* pts;
    input->GetCellPoints(i, npts, pts);
    if (cellType == VTK_VOXEL)
    {
      // Change point order: voxels and hexahedron don't have same connectivity.
      ids[0] = pts[0];
      ids[1] = pts[1];
      ids[2] = pts[3];
      ids[3] = pts[2];
      ids[4] = pts[4];
      ids[5] = pts[5];
      ids[6] = pts[7];
      ids[7] = pts[6];
      cells->ReplaceCellAtId(cellId, 8, ids);
    }
    else
    {
      cells->ReplaceCellAtId(cellId, 8, pts);
    }
    output->GetCellData()->CopyData(input->GetCellData(), i, cellId);
    if (expectedCells != nbCells)
    {
      // Unblank after copying the cell data to ensure it is not overwrited
      output->UnBlankCell(cellId);
    }
  }

  output->CheckAndReorderFaces();
  output->ComputeFacesConnectivityFlagsArray();
  return 1;
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridToExplicitStructuredGrid::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}
