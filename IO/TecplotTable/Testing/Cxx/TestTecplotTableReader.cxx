// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkTable.h>
#include <vtkTecplotTableReader.h>
#include <vtkTestUtilities.h>

#include <iostream>

// This tests the ability to read a Tecplot table. The test file contains residuals from a CFD
// calculation.
int TestTecplotTableReader(int argc, char* argv[])
{
  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/residuals.dat");
  vtkTecplotTableReader* reader = vtkTecplotTableReader::New();
  reader->SetFileName(filename);
  delete[] filename;
  reader->OutputPedigreeIdsOn();
  reader->Update();

  vtkTable* table = reader->GetOutput();
  table->Dump();
  std::cout << "Printing reader info..." << std::endl;
  reader->Print(std::cout);

  if (table->GetNumberOfRows() != 171)
  {
    std::cout << "ERROR: Wrong number of rows: " << table->GetNumberOfRows() << std::endl;
    return 1;
  }
  if (table->GetNumberOfColumns() != 11 + 1) // one extra for pedigree ids
  {
    std::cout << "ERROR: Wrong number of columns: " << table->GetNumberOfColumns() << std::endl;
    return 1;
  }

  reader->Delete();

  return 0;
}
