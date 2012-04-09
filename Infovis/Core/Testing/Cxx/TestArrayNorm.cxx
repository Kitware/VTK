/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestArrayNorm.cxx

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
#include <vtkArrayNorm.h>
#include <vtkDenseArray.h>
#include <vtkDiagonalMatrixSource.h>
#include <vtkSmartPointer.h>
#include <vtkSparseArray.h>

#include <vtksys/ios/iostream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    throw std::runtime_error("Expression failed: " #expression); \
}

static bool close_enough(const double lhs, const double rhs)
{
  return fabs(lhs - rhs) < 1.0e-12;
}

int TestArrayNorm(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  cout << setprecision(17);

  try
    {
    vtkSmartPointer<vtkDiagonalMatrixSource> source = vtkSmartPointer<vtkDiagonalMatrixSource>::New();
    source->SetExtents(3);
    source->SetArrayType(vtkDiagonalMatrixSource::SPARSE);
    source->SetDiagonal(1.0);
    source->SetSuperDiagonal(0.5);
    source->SetSubDiagonal(-0.5);
    source->Update();

    cout << "diagonal source:\n";
    vtkPrintMatrixFormat(cout, vtkSparseArray<double>::SafeDownCast(
        source->GetOutput()->GetArray(static_cast<vtkIdType>(0))));

    vtkSmartPointer<vtkArrayNorm> vector_norm = vtkSmartPointer<vtkArrayNorm>::New();
    vector_norm->AddInputConnection(source->GetOutputPort());
    vector_norm->SetDimension(1); // Column-vectors
    vector_norm->SetL(2);
    vector_norm->Update();

    vtkDenseArray<double>* const l2_norm = vtkDenseArray<double>::SafeDownCast(
      vector_norm->GetOutput()->GetArray(static_cast<vtkIdType>(0)));

    cout << "L2-norm:\n";
    vtkPrintVectorFormat(cout, l2_norm);

    test_expression(l2_norm);
    test_expression(close_enough(l2_norm->GetValueN(0), 1.1180339887498949));
    test_expression(close_enough(l2_norm->GetValueN(1), 1.2247448713915889));
    test_expression(close_enough(l2_norm->GetValueN(2), 1.1180339887498949));

    vector_norm->SetL(1);
    vector_norm->Update();

    vtkDenseArray<double>* const l1_norm = vtkDenseArray<double>::SafeDownCast(
      vector_norm->GetOutput()->GetArray(static_cast<vtkIdType>(0)));

    cout << "L1-norm:\n";
    vtkPrintVectorFormat(cout, l1_norm);

    test_expression(l1_norm);
    test_expression(close_enough(l1_norm->GetValueN(0), 0.5));
    test_expression(close_enough(l1_norm->GetValueN(1), 1.0));
    test_expression(close_enough(l1_norm->GetValueN(2), 1.5));

    vector_norm->SetInvert(true);
    vector_norm->Update();

    vtkDenseArray<double>* const inverse_l1_norm = vtkDenseArray<double>::SafeDownCast(
      vector_norm->GetOutput()->GetArray(static_cast<vtkIdType>(0)));

    cout << "Inverse L1-norm:\n";
    vtkPrintVectorFormat(cout, inverse_l1_norm);

    test_expression(inverse_l1_norm);
    test_expression(close_enough(inverse_l1_norm->GetValueN(0), 2.0));
    test_expression(close_enough(inverse_l1_norm->GetValueN(1), 1.0));
    test_expression(close_enough(inverse_l1_norm->GetValueN(2), 0.666666666666666));

    vector_norm->SetInvert(false);
    vector_norm->SetWindow(vtkArrayRange(0, 2));
    vector_norm->Update();

    vtkDenseArray<double>* const window_l1_norm = vtkDenseArray<double>::SafeDownCast(
      vector_norm->GetOutput()->GetArray(static_cast<vtkIdType>(0)));

    cout << "Windowed L1-norm:\n";
    vtkPrintVectorFormat(cout, window_l1_norm);

    test_expression(window_l1_norm);
    test_expression(close_enough(window_l1_norm->GetValueN(0), 0.5));
    test_expression(close_enough(window_l1_norm->GetValueN(1), 1.5));
    test_expression(close_enough(window_l1_norm->GetValueN(2), 0.5));
    return 0;
    }
  catch(std::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

