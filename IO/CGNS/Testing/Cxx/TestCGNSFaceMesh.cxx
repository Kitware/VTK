// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCGNSReader.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"

int TestCGNSFaceMesh(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/BoxWithFaceData.cgns");
  vtkNew<vtkCGNSReader> reader;
  reader->SetFileName(fname);
  delete[] fname;

  reader->UpdateInformation();
  reader->EnableAllCellArrays();
  reader->EnableAllFaceArrays();
  reader->EnableAllPointArrays();

  // Read cell data - should be one cube cell
  reader->Update();
  vtkMultiBlockDataSet* cube = reader->GetOutput();
  vtkDataSet* ds =
    vtkDataSet::SafeDownCast(vtkMultiBlockDataSet::SafeDownCast(cube->GetBlock(0))->GetBlock(0));

  if (!ds)
  {
    std::cerr << "Empty reader output!" << std::endl;
    return EXIT_FAILURE;
  }

  if (ds->GetNumberOfCells() != 1)
  {
    std::cerr << "Wrong number of cells for cell mesh! Got " << ds->GetNumberOfCells()
              << " cells instead of 1." << std::endl;
    return EXIT_FAILURE;
  }

  if (!ds->GetCellData()->GetArray("CellValue"))
  {
    std::cerr << "Cell array 'CellValue' missing!" << std::endl;
    return EXIT_FAILURE;
  }

  // Read face data - should be 6 quad cells
  reader->SetDataLocation(vtkCGNSReader::FACE_DATA);
  reader->Update();
  vtkMultiBlockDataSet* cubeFaces = reader->GetOutput();
  ds = vtkDataSet::SafeDownCast(
    vtkMultiBlockDataSet::SafeDownCast(cubeFaces->GetBlock(0))->GetBlock(0));

  if (!ds)
  {
    std::cerr << "Empty reader output!" << std::endl;
    return EXIT_FAILURE;
  }

  if (ds->GetNumberOfCells() != 6)
  {
    std::cerr << "Wrong number of cells for face mesh! Got " << ds->GetNumberOfCells()
              << " cells instead of 6." << std::endl;
    return EXIT_FAILURE;
  }

  if (!ds->GetCellData()->GetArray("FaceValue"))
  {
    std::cerr << "Cell array 'FaceValue' missing!" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
