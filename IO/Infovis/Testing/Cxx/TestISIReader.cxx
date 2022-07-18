/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestISIReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkISIReader.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"
#include "vtkVariant.h"

#include <string>

template <typename value_t>
void TestValue(const value_t& Value, const value_t& ExpectedValue,
  const std::string& ValueDescription, int& ErrorCount)
{
  if (Value == ExpectedValue)
    return;

  cerr << ValueDescription << " is [" << Value << "] - expected [" << ExpectedValue << "]" << endl;

  ++ErrorCount;
}

int TestISIReader(int argc, char* argv[])
{
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/eg2.isi");

  cerr << "file: " << file << endl;

  vtkSmartPointer<vtkISIReader> reader = vtkSmartPointer<vtkISIReader>::New();
  reader->SetFileName(file);
  delete[] file;
  reader->Update();
  vtkTable* const table = reader->GetOutput();

  int error_count = 0;

  // Test the size of the output table ...
  TestValue(table->GetNumberOfColumns(), vtkIdType(37), "Column count", error_count);
  TestValue(table->GetNumberOfRows(), vtkIdType(501), "Row count", error_count);

  // Test a sampling of the table columns ...
  TestValue<std::string>(table->GetColumnName(0), "PT", "Column 0", error_count);
  TestValue<std::string>(table->GetColumnName(1), "AU", "Column 1", error_count);
  TestValue<std::string>(table->GetColumnName(2), "TI", "Column 2", error_count);
  TestValue<std::string>(table->GetColumnName(20), "PD", "Column 20", error_count);
  TestValue<std::string>(table->GetColumnName(21), "PY", "Column 21", error_count);
  TestValue<std::string>(table->GetColumnName(22), "VL", "Column 22", error_count);
  TestValue<std::string>(table->GetColumnName(34), "DE", "Column 34", error_count);
  TestValue<std::string>(table->GetColumnName(35), "SI", "Column 35", error_count);
  TestValue<std::string>(table->GetColumnName(36), "PN", "Column 36", error_count);

  // Test a sampling of the table values ...
  TestValue<std::string>(table->GetValue(0, 0).ToString(), "J", "Value 0, 0", error_count);
  TestValue<std::string>(
    table->GetValue(0, 1).ToString(), "Arantes, GM;Chaimovich, H", "Value 0, 1", error_count);
  TestValue<std::string>(table->GetValue(0, 2).ToString(),
    "Thiolysis and alcoholysis of phosphate tri- and monoesters with alkyl;and aryl "
    "leaving groups. An ab initio study in the gas phase",
    "Value 0, 2", error_count);

  TestValue<std::string>(
    table->GetValue(499, 20).ToString(), "JAN 30", "value 499, 20", error_count);
  TestValue<std::string>(table->GetValue(499, 21).ToString(), "1996", "value 499, 21", error_count);
  TestValue<std::string>(table->GetValue(499, 22).ToString(), "17", "value 499, 22", error_count);

  return error_count;
}
