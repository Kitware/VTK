// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkDelimitedTextReader.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkStringArray.h"
#include "vtkStringToNumeric.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"

#include "vtkNew.h"

#include <iostream>

namespace
{

int ArrayTypesTest(int argc, char* argv[])
{
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/authors.csv");

  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetFileName(file);
  reader->SetHaveHeaders(true);

  delete[] file;

  vtkNew<vtkStringToNumeric> numeric;
  numeric->SetInputConnection(reader->GetOutputPort());
  numeric->Update();

  vtkTable* table = vtkTable::SafeDownCast(numeric->GetOutput());

  std::cerr << "Testing array types..." << std::endl;
  int errors = 0;
  if (!vtkArrayDownCast<vtkStringArray>(table->GetColumnByName("Author")))
  {
    std::cerr << "ERROR: Author array missing" << std::endl;
    ++errors;
  }
  if (!vtkArrayDownCast<vtkStringArray>(table->GetColumnByName("Affiliation")))
  {
    std::cerr << "ERROR: Affiliation array missing" << std::endl;
    ++errors;
  }
  if (!vtkArrayDownCast<vtkStringArray>(table->GetColumnByName("Alma Mater")))
  {
    std::cerr << "ERROR: Alma Mater array missing" << std::endl;
    ++errors;
  }
  if (!vtkArrayDownCast<vtkStringArray>(table->GetColumnByName("Categories")))
  {
    std::cerr << "ERROR: Categories array missing" << std::endl;
    ++errors;
  }
  if (!vtkArrayDownCast<vtkIntArray>(table->GetColumnByName("Age")))
  {
    std::cerr << "ERROR: Age array missing or not converted to int" << std::endl;
    ++errors;
  }
  else
  {
    vtkIntArray* age = vtkArrayDownCast<vtkIntArray>(table->GetColumnByName("Age"));
    int sum = 0;
    for (vtkIdType i = 0; i < age->GetNumberOfTuples(); i++)
    {
      sum += age->GetValue(i);
    }
    if (sum != 181)
    {
      std::cerr << "ERROR: Age sum is incorrect" << std::endl;
      ++errors;
    }
  }
  if (!vtkArrayDownCast<vtkDoubleArray>(table->GetColumnByName("Coolness")))
  {
    std::cerr << "ERROR: Coolness array missing or not converted to double" << std::endl;
    ++errors;
  }
  else
  {
    vtkDoubleArray* cool = vtkArrayDownCast<vtkDoubleArray>(table->GetColumnByName("Coolness"));
    double sum = 0;
    for (vtkIdType i = 0; i < cool->GetNumberOfTuples(); i++)
    {
      sum += cool->GetValue(i);
    }
    double eps = 10e-8;
    double diff = (2.35 > sum) ? (2.35 - sum) : (sum - 2.35);
    if (diff > eps)
    {
      std::cerr << "ERROR: Coolness sum is incorrect" << std::endl;
      ++errors;
    }
  }

  std::cerr << "Testing force double..." << std::endl;
  numeric->ForceDoubleOn();
  numeric->Update();
  table = vtkTable::SafeDownCast(numeric->GetOutput());
  if (!vtkArrayDownCast<vtkDoubleArray>(table->GetColumnByName("Age")))
  {
    std::cerr << "ERROR: Arrays should have been forced to double" << std::endl;
    ++errors;
  }

  return errors;
}

int WhitespaceAndEmptyCellsTest()
{
  // Setup a table of string columns, which is to get converted to numeric
  vtkNew<vtkTable> inputTable;
  vtkNew<vtkStringArray> integerColumn;
  integerColumn->SetName("IntegerColumn");
  integerColumn->SetNumberOfTuples(2);
  integerColumn->SetValue(0, " ");
  integerColumn->SetValue(1, " 1 ");

  vtkNew<vtkStringArray> doubleColumn;
  doubleColumn->SetName("DoubleColumn");
  doubleColumn->SetNumberOfTuples(2);
  doubleColumn->SetValue(0, " ");
  doubleColumn->SetValue(1, " 1.1 ");

  inputTable->AddColumn(integerColumn);
  inputTable->AddColumn(doubleColumn);

  // Setup the vtkStringToNumeric which is under test
  vtkNew<vtkStringToNumeric> numeric;
  constexpr int defaultIntValue = 100;
  numeric->SetDefaultIntegerValue(defaultIntValue);
  numeric->SetDefaultDoubleValue(vtkMath::Nan());
  numeric->SetTrimWhitespacePriorToNumericConversion(true);
  numeric->SetInputData(inputTable);
  numeric->Update();
  vtkTable* table = vtkTable::SafeDownCast(numeric->GetOutput());
  table->Dump();

  std::cerr << "Testing handling whitespace and empty cells..." << std::endl;
  int errors = 0;
  if (!vtkArrayDownCast<vtkIntArray>(table->GetColumnByName("IntegerColumn")))
  {
    std::cerr << "ERROR: IntegerColumn array missing or not converted to int" << std::endl;
    ++errors;
  }
  else
  {
    vtkIntArray* column = vtkArrayDownCast<vtkIntArray>(table->GetColumnByName("IntegerColumn"));
    if (defaultIntValue != column->GetValue(0))
    {
      std::cerr << "ERROR: Empty cell value is: " << column->GetValue(0)
                << ". Expected: " << defaultIntValue;
      ++errors;
    }
    if (1 != column->GetValue(1))
    {
      std::cerr << "ERROR: Cell with whitespace value is: " << column->GetValue(1)
                << ". Expected: 1";
      ++errors;
    }
  }

  if (!vtkArrayDownCast<vtkDoubleArray>(table->GetColumnByName("DoubleColumn")))
  {
    std::cerr << "ERROR: DoubleColumn array missing or not converted to double" << std::endl;
    ++errors;
  }
  else
  {
    vtkDoubleArray* column =
      vtkArrayDownCast<vtkDoubleArray>(table->GetColumnByName("DoubleColumn"));
    if (!vtkMath::IsNan(column->GetValue(0)))
    {
      std::cerr << "ERROR: Empty cell value is: " << column->GetValue(0)
                << ". Expected: " << vtkMath::Nan();
      ++errors;
    }
    if (1.1 != column->GetValue(1))
    {
      std::cerr << "ERROR: Cell with whitespace value is: " << column->GetValue(1)
                << ". Expected: 1.1";
      ++errors;
    }
  }
  return errors;
}
}

int TestStringToNumeric(int argc, char* argv[])
{
  int errors = ArrayTypesTest(argc, argv);
  errors += WhitespaceAndEmptyCellsTest();

  std::cerr << "...done testing" << std::endl;
  std::cerr << errors << " errors found." << std::endl;

  return errors;
}
