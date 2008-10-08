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
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"
#include "vtkVariant.h"

template<typename value_t>
void TestValue(const value_t& Value, const value_t& ExpectedValue, const vtkStdString& ValueDescription, int& ErrorCount)
{
  if(Value == ExpectedValue)
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
  TestValue(vtkStdString(table->GetColumnName(0)), vtkStdString("PT"), "Column 0", error_count);
  TestValue(vtkStdString(table->GetColumnName(1)), vtkStdString("AU"), "Column 1", error_count);
  TestValue(vtkStdString(table->GetColumnName(2)), vtkStdString("TI"), "Column 2", error_count);
  TestValue(vtkStdString(table->GetColumnName(20)), vtkStdString("PD"), "Column 20", error_count);
  TestValue(vtkStdString(table->GetColumnName(21)), vtkStdString("PY"), "Column 21", error_count);
  TestValue(vtkStdString(table->GetColumnName(22)), vtkStdString("VL"), "Column 22", error_count);
  TestValue(vtkStdString(table->GetColumnName(34)), vtkStdString("DE"), "Column 34", error_count);
  TestValue(vtkStdString(table->GetColumnName(35)), vtkStdString("SI"), "Column 35", error_count);
  TestValue(vtkStdString(table->GetColumnName(36)), vtkStdString("PN"), "Column 36", error_count);

  // Test a sampling of the table values ...
  TestValue(table->GetValue(0, 0).ToString(), vtkStdString("J"), "Value 0, 0", error_count);
  TestValue(table->GetValue(0, 1).ToString(), vtkStdString("Arantes, GM;Chaimovich, H"), "Value 0, 1", error_count);
  TestValue(table->GetValue(0, 2).ToString(), vtkStdString("Thiolysis and alcoholysis of phosphate tri- and monoesters with alkyl;and aryl leaving groups. An ab initio study in the gas phase"), "Value 0, 2", error_count);

  TestValue(table->GetValue(499, 20).ToString(), vtkStdString("JAN 30"), "value 499, 20", error_count);
  TestValue(table->GetValue(499, 21).ToString(), vtkStdString("1996"), "value 499, 21", error_count);
  TestValue(table->GetValue(499, 22).ToString(), vtkStdString("17"), "value 499, 22", error_count);
  
  return error_count;
}
