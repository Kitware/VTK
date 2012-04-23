/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStringToNumeric.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkDelimitedTextReader.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkStringArray.h"
#include "vtkStringToNumeric.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"
#include "vtkMath.h"

#include "vtkNew.h"

namespace
{

int ArrayTypesTest(int argc, char* argv[])
{
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                    "Data/authors.csv");

  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetFileName(file);
  reader->SetHaveHeaders(true);

  delete [] file;

  vtkNew<vtkStringToNumeric> numeric;
  numeric->SetInputConnection(reader->GetOutputPort());
  numeric->Update();

  vtkTable* table = vtkTable::SafeDownCast(numeric->GetOutput());

  cerr << "Testing array types..." << endl;
  int errors = 0;
  if (!vtkStringArray::SafeDownCast(table->GetColumnByName("Author")))
    {
    cerr << "ERROR: Author array missing" << endl;
    ++errors;
    }
  if (!vtkStringArray::SafeDownCast(table->GetColumnByName("Affiliation")))
    {
    cerr << "ERROR: Affiliation array missing" << endl;
    ++errors;
    }
  if (!vtkStringArray::SafeDownCast(table->GetColumnByName("Alma Mater")))
    {
    cerr << "ERROR: Alma Mater array missing" << endl;
    ++errors;
    }
  if (!vtkStringArray::SafeDownCast(table->GetColumnByName("Categories")))
    {
    cerr << "ERROR: Categories array missing" << endl;
    ++errors;
    }
  if (!vtkIntArray::SafeDownCast(table->GetColumnByName("Age")))
    {
    cerr << "ERROR: Age array missing or not converted to int" << endl;
    ++errors;
    }
  else
    {
    vtkIntArray* age = vtkIntArray::SafeDownCast(table->GetColumnByName("Age"));
    int sum = 0;
    for (vtkIdType i = 0; i < age->GetNumberOfTuples(); i++)
      {
      sum += age->GetValue(i);
      }
    if (sum != 181)
      {
      cerr << "ERROR: Age sum is incorrect" << endl;
      ++errors;
      }
    }
  if (!vtkDoubleArray::SafeDownCast(table->GetColumnByName("Coolness")))
    {
    cerr << "ERROR: Coolness array missing or not converted to double" << endl;
    ++errors;
    }
  else
    {
    vtkDoubleArray* cool = vtkDoubleArray::SafeDownCast(table->GetColumnByName("Coolness"));
    double sum = 0;
    for (vtkIdType i = 0; i < cool->GetNumberOfTuples(); i++)
      {
      sum += cool->GetValue(i);
      }
    double eps = 10e-8;
    double diff = (2.35 > sum) ? (2.35 - sum) : (sum - 2.35);
    if (diff > eps)
      {
      cerr << "ERROR: Coolness sum is incorrect" << endl;
      ++errors;
      }
    }

  cerr << "Testing force double..." << endl;
  numeric->ForceDoubleOn();
  numeric->Update();
  table = vtkTable::SafeDownCast(numeric->GetOutput());
  if (!vtkDoubleArray::SafeDownCast(table->GetColumnByName("Age")))
    {
    cerr << "ERROR: Arrays should have been forced to double" << endl;
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

  inputTable->AddColumn(integerColumn.GetPointer());
  inputTable->AddColumn(doubleColumn.GetPointer());

  // Setup the vtkStringToNumeric which is under test
  vtkNew<vtkStringToNumeric> numeric;
  int const defaultIntValue = 100;
  numeric->SetDefaultIntegerValue(defaultIntValue);
  numeric->SetDefaultDoubleValue(vtkMath::Nan());
  numeric->SetTrimWhitespacePriorToNumericConversion(true);
  numeric->SetInputData(inputTable.GetPointer());
  numeric->Update();
  vtkTable* table = vtkTable::SafeDownCast(numeric->GetOutput());
  table->Dump();

  cerr << "Testing handling whitespace and empty cells..." << endl;
  int errors = 0;
  if (!vtkIntArray::SafeDownCast(table->GetColumnByName("IntegerColumn")))
    {
    cerr << "ERROR: IntegerColumn array missing or not converted to int" << endl;
    ++errors;
    }
  else
    {
    vtkIntArray* column =
        vtkIntArray::SafeDownCast(table->GetColumnByName("IntegerColumn"));
    if (defaultIntValue != column->GetValue(0))
      {
      cerr << "ERROR: Empty cell value is: " << column->GetValue(0)
           << ". Expected: " << defaultIntValue;
      ++errors;
      }
    if (1 != column->GetValue(1))
      {
      cerr << "ERROR: Cell with whitespace value is: "
           << column->GetValue(1) << ". Expected: 1";
      ++errors;
      }
    }

  if (!vtkDoubleArray::SafeDownCast(table->GetColumnByName("DoubleColumn")))
    {
    cerr << "ERROR: DoubleColumn array missing or not converted to double"
         << endl;
    ++errors;
    }
  else
    {
    vtkDoubleArray* column =
        vtkDoubleArray::SafeDownCast(table->GetColumnByName("DoubleColumn"));
    if (!vtkMath::IsNan(column->GetValue(0)))
      {
      cerr << "ERROR: Empty cell value is: " << column->GetValue(0)
           << ". Expected: " << vtkMath::Nan();
      ++errors;
      }
    if (1.1 != column->GetValue(1))
      {
      cerr << "ERROR: Cell with whitespace value is: "
           << column->GetValue(1) << ". Expected: 1.1";
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

  cerr << "...done testing" << endl;
  cerr << errors << " errors found." << endl;

  return errors;
}
