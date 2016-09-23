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

#include "vtkTestErrorObserver.h"

#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    throw std::runtime_error("Expression failed: " #expression); \
}

int TestArrayAPISparse(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
  {
    // Create an 3D array ...
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
    array->SetDimensionLabel(0, "I");
    test_expression(array->GetDimensions() == 3);
    test_expression(array->GetSize() == 6);
    test_expression(array->GetNonNullSize() == 0);
    test_expression(array->GetExtents() == vtkArrayExtents(1, 2, 3));
    test_expression(array->GetDimensionLabel(0) == "I");

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
          test_expression(array->GetValue(i, j, k) == value);
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
    {
      test_expression(deep_copy->GetValueN(n) == array->GetValueN(n));
    }
    // Verify that the Clear() method removes all values from the array ...
    array->Clear();
    test_expression(array->GetDimensions() == 3);
    test_expression(array->GetSize() == 6);
    test_expression(array->GetNonNullSize() == 0);
    test_expression(array->GetExtents() == vtkArrayExtents(1, 2, 3));

    // Verify that we can write data into the array with SetValue()
    // and read it out again ...
    array->Resize(vtkArrayExtents(2, 3, 4));
    array->SetValue(0, 1, 2, 1.1);
    array->SetValue(vtkArrayCoordinates(1, 2, 3), 2.2);
    array->SetValue(vtkArrayCoordinates(0, 1, 1), 3.3);

    test_expression(array->GetSize() == 24);
    test_expression(array->GetNonNullSize() == 3);
    test_expression(array->GetValue(vtkArrayCoordinates(0, 1, 2)) == 1.1);
    test_expression(array->GetValue(0, 1, 2) == 1.1);
    test_expression(array->GetValue(vtkArrayCoordinates(1, 2, 3)) == 2.2);
    test_expression(array->GetValue(vtkArrayCoordinates(0, 1, 1)) == 3.3);
    test_expression(!array->IsDense());
    test_expression(array->Validate());

    // Verify we can set a value by index
    array->SetValueN(array->GetNonNullSize() - 1, -1.0);
    test_expression(array->GetValueN(array->GetNonNullSize() - 1) == -1.0);

    // Verify the SetExtents works
    array->SetExtents(vtkArrayExtents(2, 3, 4));
    test_expression(array->GetExtents() == vtkArrayExtents(2, 3, 4));

    // Verify the sort
    array->Clear();
    array->Resize(vtkArrayExtents(5));
    array->AddValue(1, 4.0);
    array->AddValue(2, 3.0);
    array->AddValue(0, -5.0);
    array->AddValue(3, 2.0);
    array->AddValue(4, 1.0);

    vtkArraySort arraySort;
    arraySort.SetDimensions(1);
    arraySort[0] = 0;
    array->Sort(arraySort);
    const vtkSparseArray<double>::CoordinateT *coords =
      array->GetCoordinateStorage(0);

    for (unsigned int i = 0; i < array->GetNonNullSize() - 1; ++i)
    {
      test_expression(coords[i] < coords[i+1]);
    }

    // ------------------------------------------------------------
    // Verify Set/Add/GetValue and SetExtentsFromContents for 0, 1, 2
    // dimension API's
    vtkSmartPointer<vtkSparseArray<double> > array1D =
      vtkSmartPointer<vtkSparseArray<double> >::New();
    array1D->Resize(vtkArrayExtents::Uniform(1, 0));
    array1D->SetValue(0, 1.0);
    array1D->AddValue(0, 2.0);
    test_expression(!array1D->GetValue(1));
    test_expression(array1D->GetExtents() == vtkArrayExtents(0));

    std::vector<vtkSparseArray<double>::CoordinateT> coords1DU =
      array1D->GetUniqueCoordinates(0);
    test_expression(!(coords1DU.size() == array1D->GetNonNullSize()));

    vtkArrayCoordinates coord1D;
    coord1D.SetDimensions(1);
    coord1D[0] = 3;
    array1D->SetValue(coord1D, 1.0);
    test_expression(array1D->GetValue(3) == 1.0);
    array1D->SetValue(coord1D, 1.1);
    test_expression(array1D->GetValue(3) == 1.1);

    array1D->SetValue(2, 1.0);
    array1D->SetValue(2, 2.0);
    array1D->AddValue(9, 1.0);
    test_expression(array1D->GetValue(9) == 1.0);
    test_expression(!array1D->GetValue(5));

    double *values = array1D->GetValueStorage();
    test_expression(values[0] == 1.0);
    values[0] = -1.0;
    test_expression(array1D->GetValue(0) == -1.0);

    double const * valuesConst = array1D->GetValueStorage();
    test_expression(array1D->GetValue(0) == valuesConst[0]);

    array1D->SetExtentsFromContents();
    test_expression(array1D->GetExtents() == vtkArrayExtents(10));

    array1D->ReserveStorage(1000);
    test_expression(array1D->GetNonNullSize() == 1000);

    vtkSmartPointer<vtkSparseArray<double> > array2D =
      vtkSmartPointer<vtkSparseArray<double> >::New();
    array2D->Resize(vtkArrayExtents::Uniform(2, 0));
    array2D->SetValue(0, 0, 2.0);
    array2D->AddValue(0, 0, 2.0);
    test_expression(array2D->GetExtents() == vtkArrayExtents(0,0));

    array2D->SetValue(0, 1, 2.0);
    array2D->SetValue(1, 1, 2.0);
    array2D->SetValue(1, 1, 3.0);
    array2D->AddValue(9, 7, 2.0);
    array2D->AddValue(9, 8, 2.0);
    test_expression(array2D->GetValue(9,7) == 2.0);
    test_expression(array2D->GetValue(9,8) == 2.0);
    test_expression(!array2D->GetValue(5,8));
    array2D->SetExtentsFromContents();
    test_expression(array2D->GetExtents() == vtkArrayExtents(10,9));

    vtkSmartPointer<vtkSparseArray<double> > array3D =
      vtkSmartPointer<vtkSparseArray<double> >::New();
    array3D->Resize(vtkArrayExtents::Uniform(3, 0));
    array3D->SetValue(0, 0, 0, 0.0);
    array3D->AddValue(0, 0, 0, 3.0);
    test_expression(array3D->GetExtents() == vtkArrayExtents(0,0,0));

    array3D->SetValue(9, 7, 7, 3.0);
    array3D->SetValue(9, 7, 7, 4.0);
    array3D->SetValue(9, 8, 6, 3.0);
    array3D->SetValue(9, 8, 7, 3.0);
    test_expression(array3D->GetValue(9, 7, 7) == 4.0);
    test_expression(array3D->GetValue(9, 8, 6) == 3.0);
    test_expression(array3D->GetValue(9, 8, 7) == 3.0);
    test_expression(!array3D->GetValue(5,8,7));
    array3D->SetExtentsFromContents();
    test_expression(array3D->GetExtents() == vtkArrayExtents(10,9,8));

    // ------------------------------------------------------------
    // Generate some errors and verify error messages where appropriate
    vtkSmartPointer<vtkTest::ErrorObserver> errorObserver =
      vtkSmartPointer<vtkTest::ErrorObserver>::New();
    array->AddObserver(vtkCommand::ErrorEvent,errorObserver);
    array1D->AddObserver(vtkCommand::ErrorEvent,errorObserver);
    array2D->AddObserver(vtkCommand::ErrorEvent,errorObserver);
    array3D->AddObserver(vtkCommand::ErrorEvent,errorObserver);

    // Extent array dimension mismatch
    array1D->SetExtents(vtkArrayExtents(2, 3, 4));
    test_expression(
      errorObserver->GetErrorMessage().find(
        "Extent-array dimension mismatch.") != std::string::npos);
    errorObserver->Clear();

    // Index mismatch errors
    vtkArrayCoordinates coordND;
    coordND.SetDimensions(4);

    array1D->SetValue(coordND, 3.3);
    test_expression(
      errorObserver->GetErrorMessage().find(
        "Index-array dimension mismatch.") != std::string::npos);
    errorObserver->Clear();

    array1D->AddValue(coordND, 3.3);
    test_expression(
      errorObserver->GetErrorMessage().find(
        "Index-array dimension mismatch.") != std::string::npos);
    errorObserver->Clear();

    test_expression(!(array->GetValue(coordND) == 3.3));
    test_expression(
      errorObserver->GetErrorMessage().find(
        "Index-array dimension mismatch.") != std::string::npos);
    errorObserver->Clear();

    test_expression(!(array1D->GetCoordinateStorage(5)));
    test_expression(
      errorObserver->GetErrorMessage().find(
        "Dimension out-of-bounds.") != std::string::npos);
    errorObserver->Clear();

    std::vector<vtkSparseArray<double>::CoordinateT> badcoords =
      array1D->GetUniqueCoordinates(5);
    test_expression(badcoords.size() == 0);
    test_expression(
      errorObserver->GetErrorMessage().find(
        "Dimension out-of-bounds.") != std::string::npos);
    errorObserver->Clear();

    double value = 5.5;
    array->AddValue(0, value);
    test_expression(!(array->GetValue(0) == 1.1));

    test_expression(!(array1D->GetValue(0, 0) == 1.1));
    test_expression(
      errorObserver->GetErrorMessage().find(
        "Index-array dimension mismatch.") != std::string::npos);
    errorObserver->Clear();

    array1D->SetValue(0, 0, 1.0);
    test_expression(
      errorObserver->GetErrorMessage().find(
        "Index-array dimension mismatch.") != std::string::npos);
    errorObserver->Clear();

    test_expression(!(array1D->GetValue(0, 0) == 1.1));
    test_expression(
      errorObserver->GetErrorMessage().find(
        "Index-array dimension mismatch.") != std::string::npos);
    errorObserver->Clear();

    array2D->SetValue(0, 0, 0, 2.0);
    test_expression(
      errorObserver->GetErrorMessage().find(
        "Index-array dimension mismatch.") != std::string::npos);
    errorObserver->Clear();

    test_expression(!(array2D->GetValue(0, 0, 0) == 1.1));
    test_expression(
      errorObserver->GetErrorMessage().find(
        "Index-array dimension mismatch.") != std::string::npos);
    errorObserver->Clear();

    array3D->SetValue(0, 3.0);
    test_expression(
      errorObserver->GetErrorMessage().find(
        "Index-array dimension mismatch.") != std::string::npos);
    errorObserver->Clear();

    test_expression(!(array3D->GetValue(0) == 1.1));
    test_expression(
      errorObserver->GetErrorMessage().find(
        "Index-array dimension mismatch.") != std::string::npos);
    errorObserver->Clear();

    // Verify Sort errors
    arraySort.SetDimensions(0);
    array1D->Sort(arraySort);
    test_expression(
      errorObserver->GetErrorMessage().find(
        "Sort must order at least one dimension.") != std::string::npos);
    errorObserver->Clear();

    arraySort.SetDimensions(1);
    arraySort[0] = 5;
    array1D->Sort(arraySort);
    test_expression(
      errorObserver->GetErrorMessage().find(
        "Sort dimension out-of-bounds.") != std::string::npos);
    errorObserver->Clear();

    // Verify Validate error messages
    // First, duplicates present
    array->AddValue(0, 1.1);
    test_expression(!(array->Validate()));
    test_expression(
      errorObserver->GetErrorMessage().find(
        "duplicate coordinates.") != std::string::npos);
    errorObserver->Clear();

    // Second, out-of-bounds present
    array->AddValue(1000, value);
    test_expression(!(array->Validate()));
    test_expression(
      errorObserver->GetErrorMessage().find(
        "out-of-bound coordinates.") != std::string::npos);
    errorObserver->Clear();

    // Finally, a silent PrintSelf
    std::ostringstream os;
    array1D->Print(os);
    test_expression(
      os.str().find(
        "NonNullSize:") != std::string::npos);

    return 0;
  }
  catch(std::exception& e)
  {
    std::cerr << e.what() << endl;
    return 1;
  }
}
