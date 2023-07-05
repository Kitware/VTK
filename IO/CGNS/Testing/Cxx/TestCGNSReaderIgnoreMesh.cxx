// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCGNSReader.h"

#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"

int TestCGNSReaderIgnoreMesh(int argc, char* argv[])
{
  // Dataset is a structured grid with boundary patches
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bc_struct.cgns");
  vtkNew<vtkCGNSReader> reader;
  reader->SetFileName(fname);
  delete[] fname;
  reader->SetLoadMesh(false);
  reader->Update();

  vtkMultiBlockDataSet* dataset = reader->GetOutput();

  if (!dataset)
  {
    std::cerr << "Empty reader output!" << std::endl;
    return EXIT_FAILURE;
  }

  vtkMultiBlockDataSet* base = vtkMultiBlockDataSet::SafeDownCast(dataset->GetBlock(0));

  if (!base)
  {
    std::cerr << "Could not find base block." << std::endl;
    return EXIT_FAILURE;
  }

  // Check that mesh is effectively not read
  if (base->GetBlock(0))
  {
    std::cerr << "Mesh block should have been nullptr." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
