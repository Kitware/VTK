/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestScaleDimension.cxx
  
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
#include <vtkDenseArray.h>
#include <vtkDiagonalMatrixSource.h>
#include <vtkScaleDimension.h>
#include <vtkSmartPointer.h>

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

int TestScaleDimension(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  try
    {
    vtkSmartPointer<vtkDiagonalMatrixSource> array = vtkSmartPointer<vtkDiagonalMatrixSource>::New();
    array->SetExtents(4);
    array->SetSuperDiagonal(1);
    array->SetDiagonal(2);
    array->SetSubDiagonal(3);

    vtkSmartPointer<vtkDenseArray<double> > vector = vtkSmartPointer<vtkDenseArray<double> >::New();
    vector->Resize(4);
    vector->SetValue(0, 0.1);
    vector->SetValue(1, 1);
    vector->SetValue(2, 10);
    vector->SetValue(3, 100);

    vtkSmartPointer<vtkArrayData> vector_data = vtkSmartPointer<vtkArrayData>::New();
    vector_data->AddArray(vector);

    vtkSmartPointer<vtkScaleDimension> scale = vtkSmartPointer<vtkScaleDimension>::New();
    scale->SetInputConnection(0, array->GetOutputPort());
    scale->SetInputConnection(1, vector_data->GetProducerPort());

    scale->Update();
    vtkTypedArray<double>* const output = vtkTypedArray<double>::SafeDownCast(scale->GetOutput()->GetArray(0));

    cout << "Input array:\n";
    vtkPrintMatrixFormat(cout, vtkTypedArray<double>::SafeDownCast(array->GetOutput()->GetArray(0)));

    cout << "Scaled array:\n";
    vtkPrintMatrixFormat(cout, vtkTypedArray<double>::SafeDownCast(output));

    test_expression(output->GetValue(0, 0) == 0.2);
    test_expression(output->GetValue(1, 0) == 3);
    test_expression(output->GetValue(2, 2) == 20);
    test_expression(output->GetValue(3, 3) == 200);
    
    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

