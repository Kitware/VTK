// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkAffineArray.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkThresholdTable.h"
#include "vtkVariant.h"

#include "vtkSmartPointer.h"

//------------------------------------------------------------------------------
int TestIntArrayBetween(vtkThresholdTable* threshold)
{
  int errors = 0;
  threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "intArr");
  threshold->SetMinValue(vtkVariant(3));
  threshold->SetMaxValue(vtkVariant(5));
  threshold->SetMode(vtkThresholdTable::ACCEPT_BETWEEN);
  threshold->Update();
  vtkTable* output = threshold->GetOutput();
  vtkIntArray* intArrOut = vtkArrayDownCast<vtkIntArray>(output->GetColumnByName("intArr"));

  // Perform error checking
  if (!intArrOut)
  {
    cerr << "int array undefined in output" << endl;
    errors++;
  }
  else if (intArrOut->GetNumberOfTuples() != 2)
  {
    cerr << "int threshold should have 2 tuples, instead has " << intArrOut->GetNumberOfTuples()
         << endl;
    errors++;
  }
  else
  {
    if (intArrOut->GetValue(0) != 3)
    {
      cerr << "int array [0] should be 3 but is " << intArrOut->GetValue(0) << endl;
      errors++;
    }
    if (intArrOut->GetValue(1) != 4)
    {
      cerr << "int array [1] should be 4 but is " << intArrOut->GetValue(1) << endl;
      errors++;
    }
  }
  return errors;
}

//------------------------------------------------------------------------------
int TestDoubleArrayLess(vtkThresholdTable* threshold)
{
  int errors = 0;
  threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "doubleArr");
  threshold->SetMaxValue(vtkVariant(1.2));
  threshold->SetMode(vtkThresholdTable::ACCEPT_LESS_THAN);
  threshold->Update();
  auto output = threshold->GetOutput();
  vtkDoubleArray* doubleArrOut =
    vtkArrayDownCast<vtkDoubleArray>(output->GetColumnByName("doubleArr"));

  // Perform error checking
  if (!doubleArrOut)
  {
    cerr << "double array undefined in output" << endl;
    errors++;
  }
  else if (doubleArrOut->GetNumberOfTuples() != 3)
  {
    cerr << "double threshold should have 3 tuples, instead has "
         << doubleArrOut->GetNumberOfTuples() << endl;
    errors++;
  }
  else
  {
    if (doubleArrOut->GetValue(0) != 1.0)
    {
      cerr << "double array [0] should be 1.0 but is " << doubleArrOut->GetValue(0) << endl;
      errors++;
    }
    if (doubleArrOut->GetValue(1) != 1.1)
    {
      cerr << "double array [1] should be 1.1 but is " << doubleArrOut->GetValue(1) << endl;
      errors++;
    }
    if (doubleArrOut->GetValue(2) != 1.2)
    {
      cerr << "double array [2] should be 1.2 but is " << doubleArrOut->GetValue(2) << endl;
      errors++;
    }
  }

  return errors;
}

//------------------------------------------------------------------------------
int TestStringArrayOutside(vtkThresholdTable* threshold)
{
  int errors = 0;
  threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "stringArr");
  threshold->SetMinValue(vtkVariant("10"));
  threshold->SetMaxValue(vtkVariant("13"));
  threshold->SetMode(vtkThresholdTable::ACCEPT_OUTSIDE);
  threshold->Update();
  auto output = threshold->GetOutput();
  vtkStringArray* stringArrOut =
    vtkArrayDownCast<vtkStringArray>(output->GetColumnByName("stringArr"));

  // Perform error checking
  if (!stringArrOut)
  {
    cerr << "string array undefined in output" << endl;
    errors++;
  }
  else if (stringArrOut->GetNumberOfTuples() != 3)
  {
    cerr << "string threshold should have 3 tuples, instead has "
         << stringArrOut->GetNumberOfTuples() << endl;
    errors++;
  }
  else
  {
    if (stringArrOut->GetValue(0) != "10")
    {
      cerr << "string array [0] should be 10 but is " << stringArrOut->GetValue(0) << endl;
      errors++;
    }
    if (stringArrOut->GetValue(1) != "13")
    {
      cerr << "string array [1] should be 13 but is " << stringArrOut->GetValue(1) << endl;
      errors++;
    }
    if (stringArrOut->GetValue(2) != "14")
    {
      cerr << "string array [2] should be 14 but is " << stringArrOut->GetValue(2) << endl;
      errors++;
    }
  }
  return errors;
}

//------------------------------------------------------------------------------
int TestImplicitArrayGreater(vtkThresholdTable* threshold)
{
  int errors = 0;
  threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "affineArr");
  threshold->SetMinValue(vtkVariant(4));
  threshold->SetMaxValue(vtkVariant(8));
  threshold->SetMode(vtkThresholdTable::ACCEPT_GREATER_THAN);
  threshold->Update();
  auto output = threshold->GetOutput();
  vtkIntArray* intArrOut = vtkArrayDownCast<vtkIntArray>(output->GetColumnByName("affineArr"));

  // Perform error checking
  if (!intArrOut)
  {
    cerr << "affine array undefined in output" << endl;
    errors++;
  }
  else if (intArrOut->GetNumberOfTuples() != 3)
  {
    cerr << "affine threshold should have 3 tuples, instead has " << intArrOut->GetNumberOfTuples()
         << endl;
    errors++;
  }
  else
  {
    if (intArrOut->GetValue(0) != 5)
    {
      cerr << "affine array [0] should be 5 but is " << intArrOut->GetValue(0) << endl;
      errors++;
    }
    if (intArrOut->GetValue(1) != 7)
    {
      cerr << "affine array [1] should be 7 but is " << intArrOut->GetValue(1) << endl;
      errors++;
    }
    if (intArrOut->GetValue(2) != 9)
    {
      cerr << "affine array [2] should be 9 but is " << intArrOut->GetValue(2) << endl;
      errors++;
    }
  }
  return errors;
}

//------------------------------------------------------------------------------
int TestThresholdTable(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Create the test input
  vtkNew<vtkTable> table;
  vtkNew<vtkIntArray> intArr;
  intArr->SetName("intArr");
  intArr->InsertNextValue(0);
  intArr->InsertNextValue(1);
  intArr->InsertNextValue(2);
  intArr->InsertNextValue(3);
  intArr->InsertNextValue(4);
  table->AddColumn(intArr);
  vtkNew<vtkDoubleArray> doubleArr;
  doubleArr->SetName("doubleArr");
  doubleArr->InsertNextValue(1.0);
  doubleArr->InsertNextValue(1.1);
  doubleArr->InsertNextValue(1.2);
  doubleArr->InsertNextValue(1.3);
  doubleArr->InsertNextValue(1.4);
  table->AddColumn(doubleArr);
  vtkNew<vtkStringArray> stringArr;
  stringArr->SetName("stringArr");
  stringArr->InsertNextValue("10");
  stringArr->InsertNextValue("11");
  stringArr->InsertNextValue("12");
  stringArr->InsertNextValue("13");
  stringArr->InsertNextValue("14");
  table->AddColumn(stringArr);
  vtkNew<vtkAffineArray<int>> oddIntArr;
  oddIntArr->SetName("affineArr");
  // value = 2*idx + 1
  oddIntArr->SetBackend(std::make_shared<vtkAffineImplicitBackend<int>>(2, 1));
  oddIntArr->SetNumberOfTuples(5);
  oddIntArr->SetNumberOfComponents(1);
  table->AddColumn(oddIntArr);

  // Use the ThresholdTable
  vtkNew<vtkThresholdTable> threshold;
  threshold->SetInputData(table);

  std::cout << "test int between" << std::endl;
  int errors = TestIntArrayBetween(threshold);
  std::cout << "test double less" << std::endl;
  errors += TestDoubleArrayLess(threshold);
  std::cout << "test string outside" << std::endl;
  errors += TestStringArrayOutside(threshold);
  std::cout << "test implicit greater" << std::endl;
  errors += TestImplicitArrayGreater(threshold);

  return errors;
}
