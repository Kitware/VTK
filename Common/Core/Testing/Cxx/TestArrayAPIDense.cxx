/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayAPIDense.cxx

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

#include <vtkDenseArray.h>
#include <vtkSmartPointer.h>

#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    vtksys_ios::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw std::runtime_error(buffer.str()); \
    } \
}

int TestArrayAPIDense(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    // Create an array ...
    vtkSmartPointer<vtkDenseArray<double> > array = vtkSmartPointer<vtkDenseArray<double> >::New();
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
    array->Print(std::cout);
    test_expression(array->GetDimensions() == 3);
    test_expression(array->GetSize() == 6);
    test_expression(array->GetNonNullSize() == 6);
    test_expression(array->GetExtents() == vtkArrayExtents(1, 2, 3));

    // Initialize the array to zero and verify that the array contains all zeros ...
    {
    array->Fill(0.0);
    const vtkArrayExtents extents = array->GetExtents();
    for(vtkIdType i = extents[0].GetBegin(); i != extents[0].GetEnd(); ++i)
      {
      for(vtkIdType j = extents[1].GetBegin(); j != extents[1].GetEnd(); ++j)
        {
        for(vtkIdType k = extents[2].GetBegin(); k != extents[2].GetEnd(); ++k)
          {
          test_expression(array->GetValue(vtkArrayCoordinates(i, j, k)) == 0.0);
          }
        }
      }
    }

    // Verify that we can write data into the array and read it out again ...
    {
    double value = 0;
    const vtkArrayExtents extents = array->GetExtents();
    for(vtkIdType i = extents[0].GetBegin(); i != extents[0].GetEnd(); ++i)
      {
      for(vtkIdType j = extents[1].GetBegin(); j != extents[1].GetEnd(); ++j)
        {
        for(vtkIdType k = extents[2].GetBegin(); k != extents[2].GetEnd(); ++k)
          {
          array->SetValue(vtkArrayCoordinates(i, j, k), value++);
          }
        }
      }
    }

    {
    double value = 0;
    vtkIdType index = 0;
    const vtkArrayExtents extents = array->GetExtents();
    for(vtkIdType i = extents[0].GetBegin(); i != extents[0].GetEnd(); ++i)
      {
      for(vtkIdType j = extents[1].GetBegin(); j != extents[1].GetEnd(); ++j)
        {
        for(vtkIdType k = extents[2].GetBegin(); k != extents[2].GetEnd(); ++k)
          {
          test_expression(array->GetValue(vtkArrayCoordinates(i, j, k)) == value);

          vtkArrayCoordinates coordinates;
          array->GetCoordinatesN(index, coordinates);

          ++index;
          ++value;
          }
        }
      }
    }

    // Verify that fill works correctly ...
    array->Fill(19700827);

    // Test unordered access ...
    for(vtkArray::SizeT n = 0; n != array->GetNonNullSize(); ++n)
      test_expression(array->GetValueN(n) == 19700827);

    // Verify that deep-copy works correctly ...
    vtkSmartPointer<vtkDenseArray<double> > deep_copy;
    deep_copy.TakeReference(vtkDenseArray<double>::SafeDownCast(array->DeepCopy()));
    test_expression(deep_copy->GetDimensions() == array->GetDimensions());
    test_expression(deep_copy->GetSize() == array->GetSize());
    test_expression(deep_copy->GetNonNullSize() == array->GetNonNullSize());
    test_expression(deep_copy->GetExtents() == array->GetExtents());
    for(vtkArray::SizeT n = 0; n != deep_copy->GetNonNullSize(); ++n)
      test_expression(deep_copy->GetValueN(n) == 19700827);

    // Verify that data is organized in fortran-order ...
    array->SetValue(vtkArrayCoordinates(0, 0, 0), 2);
    array->SetValue(vtkArrayCoordinates(1, 0, 0), 4);
    array->SetValue(vtkArrayCoordinates(2, 0, 0), 6);

    test_expression(array->GetStorage()[0] == 2);
    test_expression(array->GetStorage()[1] == 4);
    test_expression(array->GetStorage()[2] == 6);

    // Verify that external storage works correctly ...
    double a[] = { 7, 8, 9 };
    double b[] = { 5, 6, 7, 8 };
    array->ExternalStorage(vtkArrayExtents(3), new vtkDenseArray<double>::StaticMemoryBlock(a));
    test_expression(array->GetValue(0) == 7);
    test_expression(array->GetValue(2) == 9);

    array->ExternalStorage(vtkArrayExtents(2, 2), new vtkDenseArray<double>::StaticMemoryBlock(b));
    test_expression(array->GetValue(0, 0) == 5);
    test_expression(array->GetValue(1, 0) == 6);

    return 0;
    }
  catch(std::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}
