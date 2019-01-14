/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayNormalizeMatrixVectors.cxx

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
#include <vtkNormalizeMatrixVectors.h>
#include <vtkDiagonalMatrixSource.h>
#include <vtkSmartPointer.h>
#include <vtkSparseArray.h>

#include <iostream>
#include <stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    throw std::runtime_error("Expression failed: " #expression); \
}

static bool close_enough(const double lhs, const double rhs)
{
  return fabs(lhs - rhs) < 1.0e-12;
}

int ArrayNormalizeMatrixVectors(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
  {
    vtkSmartPointer<vtkDiagonalMatrixSource> source = vtkSmartPointer<vtkDiagonalMatrixSource>::New();
    source->SetExtents(3);
    source->SetArrayType(vtkDiagonalMatrixSource::SPARSE);
    source->SetDiagonal(1.0);
    source->SetSuperDiagonal(0.5);
    source->SetSubDiagonal(-0.5);

    std::cout << std::fixed << setprecision(1);
    std::cout << "sparse diagonal source:\n";
    source->Update();
    vtkPrintMatrixFormat(std::cout, vtkTypedArray<double>::SafeDownCast(
        source->GetOutput()->GetArray(static_cast<vtkIdType>(0))));

    vtkSmartPointer<vtkNormalizeMatrixVectors> normalize = vtkSmartPointer<vtkNormalizeMatrixVectors>::New();
    normalize->AddInputConnection(source->GetOutputPort());
    normalize->SetVectorDimension(1);

    normalize->Update();
    vtkTypedArray<double>* normalized = vtkTypedArray<double>::SafeDownCast(
      normalize->GetOutput()->GetArray(static_cast<vtkIdType>(0)));
    std::cout << std::fixed << setprecision(17);
    std::cout << "sparse normalized:\n";
    vtkPrintMatrixFormat(std::cout, normalized);

    test_expression(normalized);
    test_expression(close_enough(normalized->GetValue(vtkArrayCoordinates(0, 0)), 0.89442719099991586));
    test_expression(close_enough(normalized->GetValue(vtkArrayCoordinates(1, 0)), -0.44721359549995793));
    test_expression(close_enough(normalized->GetValue(vtkArrayCoordinates(2, 0)), 0.00000000000000000));

    test_expression(close_enough(normalized->GetValue(vtkArrayCoordinates(0, 1)), 0.40824829046386307));
    test_expression(close_enough(normalized->GetValue(vtkArrayCoordinates(1, 1)), 0.81649658092772615));
    test_expression(close_enough(normalized->GetValue(vtkArrayCoordinates(2, 1)), -0.40824829046386307));

    test_expression(close_enough(normalized->GetValue(vtkArrayCoordinates(0, 2)), 0.00000000000000000));
    test_expression(close_enough(normalized->GetValue(vtkArrayCoordinates(1, 2)), 0.44721359549995793));
    test_expression(close_enough(normalized->GetValue(vtkArrayCoordinates(2, 2)), 0.89442719099991586));

    source->SetArrayType(vtkDiagonalMatrixSource::DENSE);
    std::cout << std::fixed << setprecision(1);
    std::cout << "dense diagonal source:\n";
    source->Update();
    vtkPrintMatrixFormat(std::cout, vtkTypedArray<double>::SafeDownCast(
        source->GetOutput()->GetArray(static_cast<vtkIdType>(0))));

    normalize->Update();
    normalized = vtkTypedArray<double>::SafeDownCast(
      normalize->GetOutput()->GetArray(static_cast<vtkIdType>(0)));
    std::cout << std::fixed << setprecision(17);
    std::cout << "dense normalized:\n";
    vtkPrintMatrixFormat(std::cout, normalized);

    test_expression(normalized);
    test_expression(close_enough(normalized->GetValue(vtkArrayCoordinates(0, 0)), 0.89442719099991586));
    test_expression(close_enough(normalized->GetValue(vtkArrayCoordinates(1, 0)), -0.44721359549995793));
    test_expression(close_enough(normalized->GetValue(vtkArrayCoordinates(2, 0)), 0.00000000000000000));

    test_expression(close_enough(normalized->GetValue(vtkArrayCoordinates(0, 1)), 0.40824829046386307));
    test_expression(close_enough(normalized->GetValue(vtkArrayCoordinates(1, 1)), 0.81649658092772615));
    test_expression(close_enough(normalized->GetValue(vtkArrayCoordinates(2, 1)), -0.40824829046386307));

    test_expression(close_enough(normalized->GetValue(vtkArrayCoordinates(0, 2)), 0.00000000000000000));
    test_expression(close_enough(normalized->GetValue(vtkArrayCoordinates(1, 2)), 0.44721359549995793));
    test_expression(close_enough(normalized->GetValue(vtkArrayCoordinates(2, 2)), 0.89442719099991586));

    return EXIT_SUCCESS;
  }
  catch(std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}

