/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayAPISparse.cxx

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

#include <vtkSparseArray.h>
#include <vtkSmartPointer.h>

#include <vtksys/ios/iostream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    throw vtkstd::runtime_error("Expression failed: " #expression); \
}

int TestArrayAPISparse(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    // Create an array ...
    vtkSmartPointer<vtkSparseArray<double> > array = vtkSmartPointer<vtkSparseArray<double> >::New();
    array->Resize(vtkArrayExtents::Uniform(3, 0));
    test_expression(array);

    // Test to ensure that casting works correctly ...
    test_expression(vtkTypedArray<double>::SafeDownCast(array));
    test_expression(vtkArray::SafeDownCast(array));
    test_expression(vtkObject::SafeDownCast(array));

    test_expression(!vtkTypedArray<int>::SafeDownCast(array));

    // Confirm the initial state of the array ...
    test_expression(array->GetDimensions() == 3);
    test_expression(array->GetSize() == 0);
    test_expression(array->GetNonNullSize() == 0);
    test_expression(array->GetExtents() == vtkArrayExtents(0, 0, 0));

    // Resize the array and verify that everything adds-up ...
    array->Resize(vtkArrayExtents(1, 2, 3));
    test_expression(array->GetDimensions() == 3);
    test_expression(array->GetSize() == 6);
    test_expression(array->GetNonNullSize() == 0);
    test_expression(array->GetExtents() == vtkArrayExtents(1, 2, 3));

    // Verify that the array contains all zeros ...
    {
    const vtkArrayExtents extents = array->GetExtents();
    for(int i = extents[0].GetBegin(); i != extents[0].GetEnd(); ++i)
      {
      for(int j = extents[1].GetBegin(); j != extents[1].GetEnd(); ++j)
        {
        for(int k = extents[2].GetBegin(); k != extents[2].GetEnd(); ++k)
          {
          test_expression(array->GetValue(vtkArrayCoordinates(i, j, k)) == 0);
          }
        }
      }
    }

    // Verify that we can write data into the array with AddValue() and read it out again ...
    {
    double value = 0;
    const vtkArrayExtents extents = array->GetExtents();
    for(int i = extents[0].GetBegin(); i != extents[0].GetEnd(); ++i)
      {
      for(int j = extents[1].GetBegin(); j != extents[1].GetEnd(); ++j)
        {
        for(int k = extents[2].GetBegin(); k != extents[2].GetEnd(); ++k)
          {
          array->AddValue(vtkArrayCoordinates(i, j, k), value++);
          }
        }
      }
    }

    {
    double value = 0;
    vtkIdType index = 0;
    const vtkArrayExtents extents = array->GetExtents();
    for(int i = extents[0].GetBegin(); i != extents[0].GetEnd(); ++i)
      {
      for(int j = extents[1].GetBegin(); j != extents[1].GetEnd(); ++j)
        {
        for(int k = extents[2].GetBegin(); k != extents[2].GetEnd(); ++k)
          {
          test_expression(array->GetValue(vtkArrayCoordinates(i, j, k)) == value);
          test_expression(array->GetValueN(index) == value);

          vtkArrayCoordinates coordinates;
          array->GetCoordinatesN(index, coordinates);

          test_expression(coordinates[0] == i);
          test_expression(coordinates[1] == j);
          test_expression(coordinates[2] == k);

          ++index;
          ++value;
          }
        }
      }
    }

    // Verify the number of non-null values
    test_expression(array->GetNonNullSize() == 6);

    // Verify that deep-copy works correctly ...
    array->SetNullValue(1.0);

    vtkSmartPointer<vtkSparseArray<double> > deep_copy;
    deep_copy.TakeReference(vtkSparseArray<double>::SafeDownCast(array->DeepCopy()));
    test_expression(deep_copy->GetDimensions() == array->GetDimensions());
    test_expression(deep_copy->GetSize() == array->GetSize());
    test_expression(deep_copy->GetNonNullSize() == array->GetNonNullSize());
    test_expression(deep_copy->GetExtents() == array->GetExtents());
    test_expression(deep_copy->GetNullValue() == array->GetNullValue());
    for(vtkArray::SizeT n = 0; n != deep_copy->GetNonNullSize(); ++n)
      test_expression(deep_copy->GetValueN(n) == array->GetValueN(n));

    // Verify that the Clear() method removes all values from the array ...
    array->Clear();
    test_expression(array->GetDimensions() == 3);
    test_expression(array->GetSize() == 6);
    test_expression(array->GetNonNullSize() == 0);
    test_expression(array->GetExtents() == vtkArrayExtents(1, 2, 3));

    // Verify that we can write data into the array with SetValue() and read it out again ...
    array->Resize(vtkArrayExtents(2, 3, 4));
    array->SetValue(vtkArrayCoordinates(0, 1, 2), 1.1);
    array->SetValue(vtkArrayCoordinates(1, 2, 3), 2.2);
    array->SetValue(vtkArrayCoordinates(0, 1, 1), 3.3);

    test_expression(array->GetSize() == 24);
    test_expression(array->GetNonNullSize() == 3);
    test_expression(array->GetValue(vtkArrayCoordinates(0, 1, 2)) == 1.1);
    test_expression(array->GetValue(vtkArrayCoordinates(1, 2, 3)) == 2.2);
    test_expression(array->GetValue(vtkArrayCoordinates(0, 1, 1)) == 3.3);

    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}
