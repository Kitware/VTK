/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayAPIDenseCoordinates.cxx
  
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
#include <vtkDenseArray.h>
#include <vtkDiagonalMatrixSource.h>
#include <vtkSmartPointer.h>
#include <vtkSparseArray.h>

#include <vtksys/ios/iostream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    throw vtkstd::runtime_error("Expression failed: " #expression); \
}

int ArrayAPIDenseCoordinates(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    vtkSmartPointer<vtkDiagonalMatrixSource> source = vtkSmartPointer<vtkDiagonalMatrixSource>::New();
    source->SetExtents(3);
    source->SetArrayType(vtkDiagonalMatrixSource::DENSE);
    source->SetDiagonal(1.0);
    source->SetSuperDiagonal(0.5);
    source->SetSubDiagonal(-0.5);
    source->Update();

    vtkDenseArray<double>* const array = vtkDenseArray<double>::SafeDownCast(
      source->GetOutput()->GetArray(static_cast<vtkIdType>(0)));

    cout << "dense diagonal matrix:\n";
    vtkPrintMatrixFormat(cout, array);

    cout << "dense diagonal coordinates:\n";
    vtkPrintCoordinateFormat(cout, array);

    test_expression(array);
    test_expression(array->GetValue(vtkArrayCoordinates(0, 0)) == 1.0);
    test_expression(array->GetValue(vtkArrayCoordinates(1, 0)) == -0.5);
    test_expression(array->GetValue(vtkArrayCoordinates(2, 0)) == 0.0);
    test_expression(array->GetValue(vtkArrayCoordinates(0, 1)) == 0.5);
    test_expression(array->GetValue(vtkArrayCoordinates(1, 1)) == 1.0);
    test_expression(array->GetValue(vtkArrayCoordinates(2, 1)) == -0.5);
    test_expression(array->GetValue(vtkArrayCoordinates(0, 2)) == 0.0);
    test_expression(array->GetValue(vtkArrayCoordinates(1, 2)) == 0.5);
    test_expression(array->GetValue(vtkArrayCoordinates(2, 2)) == 1.0);

    for(vtkArray::SizeT n = 0; n != array->GetNonNullSize(); ++n)
      {
      vtkArrayCoordinates coordinates;
      array->GetCoordinatesN(n, coordinates);

      if(coordinates[0] == 0 && coordinates[1] == 0)
        {
        test_expression(array->GetValueN(n) == 1.0);
        }
      else if(coordinates[0] == 0 && coordinates[1] == 1)
        {
        test_expression(array->GetValueN(n) == 0.5);
        }
      else if(coordinates[0] == 1 && coordinates[1] == 0)
        {
        test_expression(array->GetValueN(n) == -0.5);
        }
      }

    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

