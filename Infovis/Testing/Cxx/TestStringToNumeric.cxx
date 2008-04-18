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

#include "vtkSmartPointer.h"
#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestStringToNumeric(int argc, char* argv[])
{
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                    "Data/authors.csv");
  
  VTK_CREATE(vtkDelimitedTextReader, reader);
  reader->SetFileName(file);
  reader->SetHaveHeaders(true);

  delete [] file;

  VTK_CREATE(vtkStringToNumeric, numeric);
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
  cerr << "...done testing" << endl;
  cerr << errors << " errors found." << endl;
  
  return errors;
}

