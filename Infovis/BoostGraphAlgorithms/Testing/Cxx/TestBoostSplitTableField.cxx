/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBoostSplitTableField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBoostSplitTableField.h"
#include "vtkDelimitedTextReader.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"
#include "vtkVariant.h"

template <typename value_t>
void TestValue(const value_t& Value, const value_t& ExpectedValue,
  const std::string& ValueDescription, int& ErrorCount)
{
  if (Value == ExpectedValue)
    return;

  cerr << ValueDescription << " is [" << Value << "] - expected [" << ExpectedValue << "]" << endl;

  ++ErrorCount;
}

int TestBoostSplitTableField(int argc, char* argv[])
{
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/publications.csv");

  cerr << "file: " << file << endl;

  vtkSmartPointer<vtkDelimitedTextReader> reader = vtkSmartPointer<vtkDelimitedTextReader>::New();
  reader->SetFileName(file);
  delete[] file;
  reader->SetHaveHeaders(true);

  vtkSmartPointer<vtkBoostSplitTableField> split = vtkSmartPointer<vtkBoostSplitTableField>::New();
  split->AddInputConnection(reader->GetOutputPort());
  split->AddField("Author", ";");

  split->Update();
  vtkTable* const table = split->GetOutput();

  int error_count = 0;

  // Test the size of the output table ...
  TestValue(table->GetNumberOfColumns(), vtkIdType(5), "Column count", error_count);
  TestValue(table->GetNumberOfRows(), vtkIdType(9), "Row count", error_count);

  // Test a sampling of the table columns ...
  TestValue<std::string>(table->GetColumnName(0), "PubID", "Column 0", error_count);
  TestValue<std::string>(table->GetColumnName(1), "Author", "Column 1", error_count);
  TestValue<std::string>(table->GetColumnName(2), "Journal", "Column 2", error_count);
  TestValue<std::string>(table->GetColumnName(3), "Categories", "Column 3", error_count);
  TestValue<std::string>(table->GetColumnName(4), "Accuracy", "Column 4", error_count);

  // Test a sampling of the table values ...
  TestValue<std::string>(table->GetValue(0, 0).ToString(), "P001", "Value 0, 0", error_count);
  TestValue<std::string>(table->GetValue(0, 1).ToString(), "Biff", "Value 0, 1", error_count);
  TestValue<std::string>(table->GetValue(0, 2).ToString(), "American Journal of Spacecraft Music",
    "Value 0, 2", error_count);

  TestValue<std::string>(table->GetValue(7, 0).ToString(), "P008", "value 7, 0", error_count);
  TestValue<std::string>(table->GetValue(7, 1).ToString(), "Biff", "value 7, 1", error_count);
  TestValue<std::string>(table->GetValue(7, 2).ToString(),
    "American Crafts and Holistic Medicine Quarterly", "value 7, 2", error_count);

  TestValue<std::string>(table->GetValue(8, 0).ToString(), "P008", "value 8, 0", error_count);
  TestValue<std::string>(table->GetValue(8, 1).ToString(), "Bob", "value 8, 1", error_count);
  TestValue<std::string>(table->GetValue(8, 2).ToString(),
    "American Crafts and Holistic Medicine Quarterly", "value 8, 2", error_count);

  return error_count;
}
