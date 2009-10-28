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

#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/vector.hpp>
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
// End-users can ignore this, it's the guts of the beast ...
//
//

template<template <typename> class TargetT, typename FunctorT>
struct vtkTryDowncastHelper
{
  vtkTryDowncastHelper(vtkObject* source, FunctorT functor, bool& succeeded) :
    Source(source),
    Functor(functor),
    Succeeded(succeeded)
  {
  }

  template<typename ValueT>
  void operator()(ValueT) const
  {
    if(Succeeded)
      return;

    if(TargetT<ValueT>* const target = TargetT<ValueT>::SafeDownCast(Source))
      {
      Succeeded = true;
      this->Functor(target);
      }
  }

  vtkObject* Source;
  FunctorT Functor;
  bool& Succeeded;
};

template<template <typename> class TargetT, typename FunctorT, typename Arg1T>
struct vtkTryDowncastHelper1
{
  vtkTryDowncastHelper1(vtkObject* source, FunctorT functor, Arg1T arg1, bool& succeeded) :
    Source(source),
    Functor(functor),
    Arg1(arg1),
    Succeeded(succeeded)
  {
  }

  template<typename ValueT>
  void operator()(ValueT) const
  {
    if(Succeeded)
      return;

    if(TargetT<ValueT>* const target = TargetT<ValueT>::SafeDownCast(Source))
      {
      Succeeded = true;
      this->Functor(target, this->Arg1);
      }
  }

  vtkObject* Source;
  FunctorT Functor;
  Arg1T Arg1;
  bool& Succeeded;
};

template<template <typename> class TargetT, typename TypesT, typename FunctorT>
FunctorT vtkTryDowncast(vtkObject* source, FunctorT functor)
{
  bool succeeded = false;
  vtkTryDowncastHelper<TargetT, FunctorT> helper(source, functor, succeeded);
  boost::mpl::for_each<TypesT>(helper);
  return functor;
}

template<template <typename> class TargetT, typename TypesT, typename FunctorT, typename Arg1T>
FunctorT vtkTryDowncast(vtkObject* source, FunctorT functor, Arg1T arg1)
{
  bool succeeded = false;
  vtkTryDowncastHelper1<TargetT, FunctorT, Arg1T> helper(source, functor, arg1, succeeded);
  boost::mpl::for_each<TypesT>(helper);
  return functor;
}

//
//
// These are lists of standard VTK types.  End-users will have to choose these when they implement
// their algorithms.
//
//

// Lists all integer VTK types
typedef boost::mpl::vector<vtkTypeUInt8, vtkTypeInt8, vtkTypeUInt16, vtkTypeInt16, vtkTypeUInt32, vtkTypeInt32, vtkTypeUInt64, vtkTypeInt64, vtkIdType> vtkIntegerTypes;
// Lists all integer VTK types
typedef boost::mpl::vector<vtkTypeFloat32, vtkTypeFloat64> vtkFloatingPointTypes;
// Lists all string VTK types
typedef boost::mpl::vector<vtkStdString, vtkUnicodeString> vtkStringTypes;
// Lists all numeric VTK types
typedef boost::mpl::joint_view<vtkIntegerTypes, vtkFloatingPointTypes> vtkNumericTypes;
// Lists all VTK types
typedef boost::mpl::joint_view<vtkNumericTypes, vtkStringTypes> vtkAllTypes;

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

  mutable vtkSmartPointer<vtkArray> ResultMatrix;
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

    vtkSmartPointer<vtkArray> transposed_matrix = vtkTryDowncast<vtkDenseArray, vtkAllTypes>(dense_double, Transpose()).ResultMatrix;

    // Alternative syntax: passing results via functor arguments ...
    // vtkSmartPointer<vtkArray> transposed_matrix;
    // vtkTryDowncast<vtkDenseArray, vtkAllTypes>(dense_double, Transpose(transposed_matrix));

    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

