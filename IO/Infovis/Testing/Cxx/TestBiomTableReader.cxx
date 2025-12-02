// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBiomTableReader.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"

#include <iostream>

int TestBiomTableReader(int argc, char* argv[])
{
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/otu_table.biom");

  std::cerr << "file: " << file << std::endl;

  vtkSmartPointer<vtkBiomTableReader> reader = vtkSmartPointer<vtkBiomTableReader>::New();
  reader->SetFileName(file);
  delete[] file;
  reader->Update();
  vtkTable* table = reader->GetOutput();

  int error_count = 0;

  if (table->GetNumberOfRows() != 419)
  {
    ++error_count;
  }

  if (table->GetNumberOfColumns() != 10)
  {
    ++error_count;
  }

  std::cerr << error_count << " errors" << std::endl;
  return error_count;
}
