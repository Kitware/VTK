/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayTableToSparseArray.cxx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkArrayData.h>
#include <vtkArrayPrint.h>
#include <vtkDoubleArray.h>
#include <vtkIdTypeArray.h>
#include <vtkSmartPointer.h>
#include <vtkSparseArray.h>
#include <vtkTable.h>
#include <vtkTableToSparseArray.h>

#include <vtksys/ios/iostream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    throw vtkstd::runtime_error("Expression failed: " #expression); \
}

int ArrayTableToSparseArray(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    vtkSmartPointer<vtkIdTypeArray> i = vtkSmartPointer<vtkIdTypeArray>::New();
    i->SetName("i");
    
    vtkSmartPointer<vtkIdTypeArray> j = vtkSmartPointer<vtkIdTypeArray>::New();
    j->SetName("j");
    
    vtkSmartPointer<vtkIdTypeArray> k = vtkSmartPointer<vtkIdTypeArray>::New();
    k->SetName("k");

    vtkSmartPointer<vtkDoubleArray> value = vtkSmartPointer<vtkDoubleArray>::New();
    value->SetName("value");
    
    vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
    table->AddColumn(i);
    table->AddColumn(j);
    table->AddColumn(k);
    table->AddColumn(value);

    table->InsertNextBlankRow();
    table->SetValue(0, 0, 0);
    table->SetValue(0, 1, 0);
    table->SetValue(0, 2, 0);
    table->SetValue(0, 3, 1);

    table->InsertNextBlankRow();
    table->SetValue(1, 0, 1);
    table->SetValue(1, 1, 2);
    table->SetValue(1, 2, 3);
    table->SetValue(1, 3, 2);
      
    table->InsertNextBlankRow();
    table->SetValue(2, 0, 4);
    table->SetValue(2, 1, 5);
    table->SetValue(2, 2, 6);
    table->SetValue(2, 3, 3);
      
    vtkSmartPointer<vtkTableToSparseArray> source = vtkSmartPointer<vtkTableToSparseArray>::New();
    source->AddInputConnection(table->GetProducerPort());
    source->AddCoordinateColumn("i");
    source->AddCoordinateColumn("j");
    source->AddCoordinateColumn("k");
    source->SetValueColumn("value");
    source->Update();

    vtkSparseArray<double>* const sparse_array = vtkSparseArray<double>::SafeDownCast(
      source->GetOutput()->GetArray(static_cast<vtkIdType>(0)));
    test_expression(sparse_array);

    cout << "sparse array:\n";
    vtkPrintCoordinateFormat(cout, sparse_array);
    
    test_expression(sparse_array->GetValue(vtkArrayCoordinates(0, 0, 0)) == 1);
    test_expression(sparse_array->GetValue(vtkArrayCoordinates(1, 2, 3)) == 2);
    test_expression(sparse_array->GetValue(vtkArrayCoordinates(4, 5, 6)) == 3);

    test_expression(sparse_array->GetValue(vtkArrayCoordinates(0, 0, 1)) == 0);

    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

