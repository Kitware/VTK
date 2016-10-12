/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDelimitedTextReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkTecplotTableReader.h>
#include <vtkTable.h>
#include <vtkTestUtilities.h>

// This tests the ability to read a Tecplot table. The test file contains residuals from a CFD calculation.
int TestTecplotTableReader(int argc, char *argv[])
{
  //------------  test the reader with an input file-----------------
  if (argc != 3) return 0; // for some reason we get called twice, once with 5 arguments that are not pointing to the file

  char* filename = argv[2];
  std::cout << filename << std::endl;
  vtkTecplotTableReader *reader = vtkTecplotTableReader::New();
  reader->SetFileName(filename);
  reader->OutputPedigreeIdsOn();
  reader->Update();

  vtkTable* table = reader->GetOutput();
  table->Dump();
  cout << "Printing reader info..." << endl;
  reader->Print(cout);

  if (table->GetNumberOfRows() != 171)
  {
    cout << "ERROR: Wrong number of rows: " << table->GetNumberOfRows()<<endl;
    return 1;
  }
  if (table->GetNumberOfColumns() != 11 + 1) // one extra for pedigree ids
  {
    cout << "ERROR: Wrong number of columns: " << table->GetNumberOfColumns()<<endl;
    return 1;
  }

  reader->Delete();

  return 0;
}
