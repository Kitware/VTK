/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayCasting.cxx
  
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
#include <vtkSparseArray.h>
#include <vtkTryDowncast.h>

#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>

#include <boost/algorithm/string.hpp>

//
//
// Ignore this stuff, it's just for testing ...
//
//

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()


//
//
// Here are some sample functors that end-users might write.
//
//

// This functor increments array values in-place using a parameter passed via the algorithm (instead of a parameter
// stored in the functor).  It can work with any numeric array type.
struct IncrementValues
{
  template<typename T>
  void operator()(T* array, int amount) const
  {
    for(vtkIdType n = 0; n != array->GetNonNullSize(); ++n)
      array->SetValueN(n, array->GetValueN(n) + amount);
  }
};

// This functor converts strings in-place to a form suitable for case-insensitive comparison.  It's an example of
// how you can write generic code while still specializing functionality on a case-by-case basis, since
// in this situation we want to use some special functionality provided by vtkUnicodeString.
struct FoldCase
{
  template<typename ValueT>
  void operator()(vtkTypedArray<ValueT>* array) const
  {
    for(vtkIdType n = 0; n != array->GetNonNullSize(); ++n)
      {
      ValueT value = array->GetValueN(n);
      boost::algorithm::to_lower(value);
      array->SetValueN(n, value);
      }
  }

  void operator()(vtkTypedArray<vtkUnicodeString>* array) const
  {
    for(vtkIdType n = 0; n != array->GetNonNullSize(); ++n)
      array->SetValueN(n, array->GetValueN(n).fold_case());
  }
};

// This functor efficiently creates a transposed array.  It's one example of how you can create an output array
// with the same type as an input array.
struct Transpose
{
  Transpose(vtkSmartPointer<vtkArray>& result_matrix) : ResultMatrix(result_matrix) {}

  template<typename ValueT>
  void operator()(vtkDenseArray<ValueT>* input) const
  {
    if(input->GetDimensions() != 2 || input->GetExtents()[0] != input->GetExtents()[1])
      throw vtkstd::runtime_error("A square matrix is required.");

    vtkDenseArray<ValueT>* output = vtkDenseArray<ValueT>::SafeDownCast(input->DeepCopy());
    for(vtkIdType i = 0; i != input->GetExtents()[0]; ++i)
      {
      for(vtkIdType j = i + 1; j != input->GetExtents()[1]; ++j)
        {
        output->SetValue(i, j, input->GetValue(j, i));
        output->SetValue(j, i, input->GetValue(i, j));
        }
      }

    this->ResultMatrix = output;
  }

  vtkSmartPointer<vtkArray>& ResultMatrix;
};

//
//
// Here are some examples of how the algorithm might be called.
//
//

int main(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    VTK_CREATE(vtkDenseArray<int>, dense_int);
    VTK_CREATE(vtkDenseArray<double>, dense_double);
    VTK_CREATE(vtkDenseArray<vtkStdString>, dense_string);

    // Calling a functor with extra arguments passed via the algorithm ...
    vtkTryDowncast<vtkTypedArray, vtkNumericTypes>(dense_int, IncrementValues(), 1);
    vtkTryDowncast<vtkTypedArray, vtkNumericTypes>(dense_double, IncrementValues(), 2);
    vtkTryDowncast<vtkTypedArray, vtkNumericTypes>(dense_string, IncrementValues(), 3);

    // Alternative syntax: passing arguments via the functor ...
    // vtkTryDowncast<vtkTypedArray, vtkNumericTypes>(dense_int, IncrementValues(1));
    // vtkTryDowncast<vtkTypedArray, vtkNumericTypes>(dense_double, IncrementValues(2));
    // vtkTryDowncast<vtkTypedArray, vtkNumericTypes>(dense_string, IncrementValues(3));

    vtkTryDowncast<vtkTypedArray, vtkStringTypes>(dense_int, FoldCase());
    vtkTryDowncast<vtkTypedArray, vtkStringTypes>(dense_double, FoldCase());
    vtkTryDowncast<vtkTypedArray, vtkStringTypes>(dense_string, FoldCase());

    vtkSmartPointer<vtkArray> transposed_matrix;
    vtkTryDowncast<vtkDenseArray, vtkAllTypes>(dense_double, Transpose(transposed_matrix));

    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

