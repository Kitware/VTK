/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestThresholdTable.cxx

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

#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkThresholdTable.h"
#include "vtkVariant.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestThresholdTable(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{                                                        
  // Create the test input
  VTK_CREATE(vtkTable, table);
  VTK_CREATE(vtkIntArray, intArr);
  intArr->SetName("intArr");
  intArr->InsertNextValue(0);
  intArr->InsertNextValue(1);
  intArr->InsertNextValue(2);
  intArr->InsertNextValue(3);
  intArr->InsertNextValue(4);
  table->AddColumn(intArr);
  VTK_CREATE(vtkDoubleArray, doubleArr);
  doubleArr->SetName("doubleArr");
  doubleArr->InsertNextValue(1.0);
  doubleArr->InsertNextValue(1.1);
  doubleArr->InsertNextValue(1.2);
  doubleArr->InsertNextValue(1.3);
  doubleArr->InsertNextValue(1.4);
  table->AddColumn(doubleArr);
  VTK_CREATE(vtkStringArray, stringArr);
  stringArr->SetName("stringArr");
  stringArr->InsertNextValue("10");
  stringArr->InsertNextValue("11");
  stringArr->InsertNextValue("12");
  stringArr->InsertNextValue("13");
  stringArr->InsertNextValue("14");
  table->AddColumn(stringArr);
  
  // Use the ThresholdTable
  VTK_CREATE(vtkThresholdTable, threshold);
  threshold->SetInput(table);
  
  int errors = 0;
  threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "intArr");
  threshold->SetMinValue(vtkVariant(3));
  threshold->SetMaxValue(vtkVariant(5));
  threshold->SetMode(vtkThresholdTable::ACCEPT_BETWEEN);
  threshold->Update();
  vtkTable* output = threshold->GetOutput();
  vtkIntArray* intArrOut = vtkIntArray::SafeDownCast(output->GetColumnByName("intArr"));
  
  // Perform error checking
  if (!intArrOut)
    {
    cerr << "int array undefined in output" << endl;
    errors++;
    }
  else if (intArrOut->GetNumberOfTuples() != 2)
    {
    cerr << "int threshold should have 2 tuples, instead has " << intArrOut->GetNumberOfTuples() << endl;
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
  
  threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "doubleArr");
  threshold->SetMaxValue(vtkVariant(1.2));
  threshold->SetMode(vtkThresholdTable::ACCEPT_LESS_THAN);
  threshold->Update();
  output = threshold->GetOutput();
  vtkDoubleArray* doubleArrOut = vtkDoubleArray::SafeDownCast(output->GetColumnByName("doubleArr"));
  
  // Perform error checking
  if (!doubleArrOut)
    {
    cerr << "double array undefined in output" << endl;
    errors++;
    }
  else if (doubleArrOut->GetNumberOfTuples() != 3)
    {
    cerr << "double threshold should have 3 tuples, instead has " << intArrOut->GetNumberOfTuples() << endl;
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
  
  threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "stringArr");
  threshold->SetMinValue(vtkVariant("10"));
  threshold->SetMaxValue(vtkVariant("13"));
  threshold->SetMode(vtkThresholdTable::ACCEPT_OUTSIDE);
  threshold->Update();
  output = threshold->GetOutput();
  vtkStringArray* stringArrOut = vtkStringArray::SafeDownCast(output->GetColumnByName("stringArr"));
  
  // Perform error checking
  if (!stringArrOut)
    {
    cerr << "string array undefined in output" << endl;
    errors++;
    }
  else if (stringArrOut->GetNumberOfTuples() != 3)
    {
    cerr << "string threshold should have 3 tuples, instead has " << stringArrOut->GetNumberOfTuples() << endl;
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
