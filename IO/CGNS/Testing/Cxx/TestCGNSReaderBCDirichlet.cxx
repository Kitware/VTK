// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCGNSReader.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkStructuredGrid.h"
#include "vtkTestUtilities.h"

int TestCGNSReaderBCDirichlet(int argc, char* argv[])
{
  // Structured dataset with Dirichlet boundary conditions
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/BCDirichletStructured.cgns");
  vtkNew<vtkCGNSReader> reader;
  reader->SetFileName(fname);
  delete[] fname;
  reader->SetLoadBndPatch(true);
  reader->Update();

  vtkMultiBlockDataSet* dataset = reader->GetOutput();

  if (!dataset)
  {
    std::cerr << "Empty reader output!" << std::endl;
    return EXIT_FAILURE;
  }

  vtkMultiBlockDataSet* base = vtkMultiBlockDataSet::SafeDownCast(dataset->GetBlock(0u));

  if (!base)
  {
    std::cerr << "Could not find base block." << std::endl;
    return EXIT_FAILURE;
  }

  vtkMultiBlockDataSet* zone = vtkMultiBlockDataSet::SafeDownCast(base->GetBlock(0u));

  if (!zone)
  {
    std::cerr << "Could not find zone block." << std::endl;
    return EXIT_FAILURE;
  }

  vtkMultiBlockDataSet* patches = vtkMultiBlockDataSet::SafeDownCast(zone->GetBlock(1));

  if (!patches)
  {
    std::cerr << "Could not find boundary condition patches." << std::endl;
    return EXIT_FAILURE;
  }

  if (patches->GetNumberOfBlocks() != 2)
  {
    std::cerr << "There should be 2 boundary blocks. Found " << patches->GetNumberOfBlocks()
              << " instead." << std::endl;
    return EXIT_FAILURE;
  }

  // Check boundary values
  vtkStructuredGrid* patch = vtkStructuredGrid::SafeDownCast(patches->GetBlock(0u));

  if (!patch)
  {
    std::cerr << "Could not find first boundary patch." << std::endl;
    return EXIT_FAILURE;
  }

  if (!patch->GetCellData()->HasArray("DirichletValues"))
  {
    std::cerr << "Missing 'DirichletValues' array in first boundary patch." << std::endl;
    return EXIT_FAILURE;
  }

  vtkDoubleArray* array =
    vtkDoubleArray::SafeDownCast(patch->GetCellData()->GetArray("DirichletValues"));

  if (array->GetTuple1(0) != 1)
  {
    std::cerr << "Wrong value for 'DirichletValues' array. Expected 1 but got "
              << array->GetTuple1(0) << "." << std::endl;
    return EXIT_FAILURE;
  }

  patch = vtkStructuredGrid::SafeDownCast(patches->GetBlock(1));

  if (!patch)
  {
    std::cerr << "Could not find second boundary patch." << std::endl;
    return EXIT_FAILURE;
  }

  if (!patch->GetCellData()->HasArray("DirichletValues"))
  {
    std::cerr << "Missing 'DirichletValues' array in second boundary patch." << std::endl;
    return EXIT_FAILURE;
  }

  array = vtkDoubleArray::SafeDownCast(patch->GetCellData()->GetArray("DirichletValues"));

  if (array->GetTuple1(0) != 1.5)
  {
    std::cerr << "Wrong value for 'DirichletValues' array. Expected 1.5 but got "
              << array->GetTuple1(0) << "." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
