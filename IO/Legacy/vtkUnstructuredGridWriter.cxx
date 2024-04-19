// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkUnstructuredGridWriter.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkCellIterator.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <iterator>
#include <vector>

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkUnstructuredGridWriter);

void vtkUnstructuredGridWriter::WriteData()
{
  ostream* fp;
  vtkUnstructuredGridBase* input = vtkUnstructuredGridBase::SafeDownCast(this->GetInput());
  int *types, ncells, cellId;

  vtkDebugMacro(<< "Writing vtk unstructured grid data...");

  if (!(fp = this->OpenVTKFile()) || !this->WriteHeader(fp))
  {
    if (fp)
    {
      vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
      this->CloseVTKFile(fp);
      unlink(this->FileName);
    }
    return;
  }
  //
  // Write unstructured grid specific stuff
  //
  *fp << "DATASET UNSTRUCTURED_GRID\n";

  // Write data owned by the dataset
  if (!this->WriteDataSetData(fp, input))
  {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
  }

  if (!this->WritePoints(fp, input->GetPoints()))
  {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
  }

  // Handle face data:
  if (!this->WriteCellsAndFaces(fp, input, "CELLS"))
  {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
  }

  //
  // Cell types are a little more work
  //

  ncells = input->GetNumberOfCells();
  if (ncells > 0)
  {
    types = new int[ncells];
    for (cellId = 0; cellId < ncells; cellId++)
    {
      types[cellId] = input->GetCellType(cellId);
    }

    *fp << "CELL_TYPES " << ncells << "\n";
    if (this->FileType == VTK_ASCII)
    {
      for (cellId = 0; cellId < ncells; cellId++)
      {
        *fp << types[cellId] << "\n";
      }
    }
    else
    {
      // swap the bytes if necessary
      vtkByteSwap::SwapWrite4BERange(types, ncells, fp);
    }
    *fp << "\n";
    delete[] types;
  }

  if (!this->WriteCellData(fp, input))
  {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
  }
  if (!this->WritePointData(fp, input))
  {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
  }

  this->CloseVTKFile(fp);
}

int vtkUnstructuredGridWriter::WriteCellsAndFaces(
  ostream* fp, vtkUnstructuredGridBase* grid, const char* label)
{
  if (grid->GetNumberOfCells() == 0)
  {
    return 1;
  }

  // Create a copy of the cell data with the face streams expanded.
  // Do this before writing anything so that we know the size.
  // Use ints to represent vtkIdTypes, since that's what the superclass does.
  vtkNew<vtkCellArray> expandedCells;
  expandedCells->AllocateEstimate(grid->GetNumberOfCells(), grid->GetMaxCellSize());

  vtkSmartPointer<vtkCellIterator> it =
    vtkSmartPointer<vtkCellIterator>::Take(grid->NewCellIterator());

  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    if (it->GetCellType() != VTK_POLYHEDRON)
    {
      expandedCells->InsertNextCell(it->GetPointIds());
    }
    else
    {
      expandedCells->InsertNextCell(it->GetFaces());
    }
  }

  if (expandedCells->GetNumberOfCells() == 0)
  { // Nothing to do.
    return 1;
  }

  if (!this->WriteCells(fp, expandedCells, label))
  {
    vtkErrorMacro("Error while writing expanded face stream.");
    return 0;
  }

  fp->flush();
  if (fp->fail())
  {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return 0;
  }

  return 1;
}

int vtkUnstructuredGridWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGridBase");
  return 1;
}

vtkUnstructuredGridBase* vtkUnstructuredGridWriter::GetInput()
{
  return vtkUnstructuredGridBase::SafeDownCast(this->Superclass::GetInput());
}

vtkUnstructuredGridBase* vtkUnstructuredGridWriter::GetInput(int port)
{
  return vtkUnstructuredGridBase::SafeDownCast(this->Superclass::GetInput(port));
}

void vtkUnstructuredGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
