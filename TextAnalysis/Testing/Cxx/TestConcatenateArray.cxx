/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestConcatenateArray.cxx
  
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

#include <vtkArrayPrint.h>
#include <vtkConcatenateArray.h>
#include <vtkDenseArray.h>
#include <vtkSmartPointer.h>
#include <vtkSparseArray.h>

#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    vtkstd::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw vtkstd::runtime_error(buffer.str()); \
    } \
}

int TestConcatenateArray(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  try
    {
    vtkSmartPointer<vtkArrayData> a_data = vtkSmartPointer<vtkArrayData>::New();
    vtkSmartPointer<vtkArrayData> b_data = vtkSmartPointer<vtkArrayData>::New();

    vtkSmartPointer<vtkSparseArray<double> > a = vtkSmartPointer<vtkSparseArray<double> >::New();
    vtkSmartPointer<vtkSparseArray<double> > b = vtkSmartPointer<vtkSparseArray<double> >::New();

    a->Resize(2, 2);
    a->SetValue(0, 0, 1);
    a->SetValue(1, 1, 2);
    
    b->Resize(2, 2);
    b->SetValue(0, 0, 3);
    b->SetValue(1, 1, 4);

    a_data->AddArray(a);
    b_data->AddArray(b);

    vtkSmartPointer<vtkConcatenateArray> concatenate = vtkSmartPointer<vtkConcatenateArray>::New();
    concatenate->SetInputConnection(0, a_data->GetProducerPort());
    concatenate->SetInputConnection(1, b_data->GetProducerPort());
    concatenate->Update();
    vtkSparseArray<double>* const array = vtkSparseArray<double>::SafeDownCast(concatenate->GetOutput()->GetArray(0));
  
    vtkPrintCoordinateFormat(cerr, array);
  
    test_expression(array->GetValue(0, 0) == 1);
    test_expression(array->GetValue(1, 1) == 2);
    test_expression(array->GetValue(2, 0) == 3);
    test_expression(array->GetValue(3, 1) == 4);
    test_expression(array->GetValue(2, 1) == 0);
   
    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

