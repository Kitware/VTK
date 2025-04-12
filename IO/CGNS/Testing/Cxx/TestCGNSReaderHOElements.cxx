// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCGNSReader.h"

#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkMathUtilities.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

int TestCGNSReaderHOElements(int argc, char* argv[])
{
  // Dataset contains one element of each HEXA_64 and HEXA_125
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/HO_hexa_elem.cgns");
  vtkNew<vtkCGNSReader> reader;
  reader->SetFileName(fname);
  delete[] fname;

  // Do the reading
  reader->LoadBndPatchOff();
  reader->UpdateInformation();
  reader->Update();

  vtkMultiBlockDataSet* dataset = reader->GetOutput();

  if (!dataset)
  {
    std::cerr << "Empty reader output!" << std::endl;
    return EXIT_FAILURE;
  }

  if (dataset->GetNumberOfBlocks() != 1)
  {
    std::cerr << "Dataset should have 1 blocks but got " << dataset->GetNumberOfBlocks() << "."
              << std::endl;
    return EXIT_FAILURE;
  }

  // Check main 3D mesh
  vtkUnstructuredGrid* internal_hexa_125 = vtkUnstructuredGrid::SafeDownCast(
    vtkMultiBlockDataSet::SafeDownCast(dataset->GetBlock(0))->GetBlock(0));

  vtkUnstructuredGrid* internal_hexa_64 = vtkUnstructuredGrid::SafeDownCast(
    vtkMultiBlockDataSet::SafeDownCast(dataset->GetBlock(0))->GetBlock(1));

  // Count number of cells and check cell types
  if (internal_hexa_125->GetNumberOfCells() != 1)
  {
    std::cerr << "Wrong number of cells in main mesh. Expected 1 but got "
              << internal_hexa_125->GetNumberOfCells() << "." << std::endl;
    return EXIT_FAILURE;
  }

  if (internal_hexa_125->GetCellType(0) != VTK_LAGRANGE_HEXAHEDRON)
  {
    std::cerr
      << "Wrong type of cell in main mesh. Expected VTK_LAGRANGE_HEXAHEDRON for cell 0 but got "
      << vtkCellTypes::GetClassNameFromTypeId(internal_hexa_125->GetCellType(0)) << "."
      << std::endl;
    return EXIT_FAILURE;
  }

  if (internal_hexa_64->GetCellType(0) != VTK_LAGRANGE_HEXAHEDRON)
  {
    std::cerr
      << "Wrong type of cell in main mesh. Expected VTK_LAGRANGE_HEXAHEDRON for cell 0 but got "
      << vtkCellTypes::GetClassNameFromTypeId(internal_hexa_125->GetCellType(0)) << "."
      << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
