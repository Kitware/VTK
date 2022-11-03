/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDelimitedTextReaderWithBOM.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkDelimitedTextReader.h"
#include "vtkNew.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"

#include <cstdlib>

int TestDelimitedTextReaderWithBOM(int argc, char* argv[])
{
  int res = EXIT_SUCCESS;

  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/csvWithBOM.csv");

  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetFileName(filename);
  reader->SetHaveHeaders(true);
  reader->SetDetectNumericColumns(true);
  reader->Update();

  vtkTable* table = vtkTable::SafeDownCast(reader->GetOutput());
  table->Dump();
  if (table->GetNumberOfRows() != 2)
  {
    std::cout << "CSV with BOM does not generate correct number of rows: 2 != "
              << table->GetNumberOfRows() << std::endl;
    res = EXIT_FAILURE;
  }
  if (table->GetNumberOfColumns() != 3)
  {
    std::cout << "CSV with BOM does not generate correct number of columns: 3 != "
              << table->GetNumberOfColumns() << std::endl;
    res = EXIT_FAILURE;
  }

  vtkDataArray* xArr = vtkArrayDownCast<vtkDataArray>(table->GetRowData()->GetAbstractArray("x"));
  if (!xArr)
  {
    std::cout << "CSV with BOM does not generate an x column" << std::endl;
    res = EXIT_FAILURE;
  }

  if (!(xArr->GetComponent(0, 0) == 1 && xArr->GetComponent(1, 0) == 2))
  {
    std::cout << "CSV with BOM does not have correct x values" << std::endl;
    res = EXIT_FAILURE;
  }

  vtkDataArray* yArr = vtkArrayDownCast<vtkDataArray>(table->GetRowData()->GetAbstractArray("y"));
  if (!yArr)
  {
    std::cout << "CSV with BOM does not generate an y column" << std::endl;
    res = EXIT_FAILURE;
  }

  if (!(yArr->GetComponent(0, 0) == 1 && yArr->GetComponent(1, 0) == 3.14))
  {
    std::cout << "CSV with BOM does not have correct y values" << std::endl;
    res = EXIT_FAILURE;
  }

  vtkDataArray* zArr = vtkArrayDownCast<vtkDataArray>(table->GetRowData()->GetAbstractArray("z"));
  if (!zArr)
  {
    std::cout << "CSV with BOM does not generate an z column" << std::endl;
    res = EXIT_FAILURE;
  }

  if (!(zArr->GetComponent(0, 0) == 1 && zArr->GetComponent(1, 0) == 42))
  {
    std::cout << "CSV with BOM does not have correct z values" << std::endl;
    res = EXIT_FAILURE;
  }

  return res;
}
