/*==============================================================================

  Program:   Visualization Toolkit
  Module:    ExampleDataArrayRangeAPI.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

==============================================================================*/

// This file provides some examples of how to use the vtkDataArrayRange
// objects: TupleRanges and ValueRanges.
//
// This is not meant to be an exhaustive test of the API's correctness --
// see TestDataArray[Tuple|Value]Range.cxx for those. This is simply a quick
// reference for "what can be done with these range/iterator/reference objects?"

#include "vtkDataArrayRange.h"

#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkSOADataArrayTemplate.h"

#include <iterator> // for std::distance
#include <numeric>  // for std::iota
#include <vector>

namespace
{

template <typename ArrayT>
void TestTupleRangeAPI(ArrayT* someArray)
{
  // TupleRanges have a two-step hierarchy of iterators and references. The
  // first layer encapsulates the concept of tuples, and the second layer
  // provides access to the components in a tuple. The following code shows
  // how these objects (TupleRange, TupleIterator, TupleReference,
  // ComponentIterator, ComponentReference) can be used.

  // A TupleRange can be restricted to a subset of the array's data by passing
  // explicit start/end values to vtk::DataArrayTupleRange:
  {
    auto subRange = vtk::DataArrayTupleRange(someArray, 2, 8);
    // Iterates over tuples 2-7 (inclusive).
    for (auto tupleRef : subRange)
    {
      std::cout << "Tuple: ";
      for (auto compRef : tupleRef)
      {
        std::cout << compRef << " ";
      }
      std::cout << "\n";
    }
  }

  // If the exact number of components in a tuple is known at compile-time,
  // this can be passed as a template argument to vtk::DataArrayTupleRange.
  // This will enable additional compiler optimizations to improve performance.
  {
    auto optimizedRange = vtk::DataArrayTupleRange<4>(someArray);
    // Accessing data in optimizedRange will be more efficient as variables
    // like array strides are made available to the compiler.
    for (auto tupleRef : optimizedRange)
    {
      std::cout << "Tuple: ";
      for (auto compRef : tupleRef)
      {
        std::cout << compRef << " ";
      }
      std::cout << "\n";
    }
  }

  // Both tuple size and subrange information can be used simultaneously:
  {
    auto optimizedSubRange = vtk::DataArrayTupleRange<4>(someArray, 2, 8);
    for (auto tupleRef : optimizedSubRange)
    {
      std::cout << "Tuple: ";
      for (auto compRef : tupleRef)
      {
        std::cout << compRef << " ";
      }
      std::cout << "\n";
    }
  }

  // If tuple size is unknown and the range should encompass the full length of
  // the array, simply pass in the array with no template arguments:
  auto range = vtk::DataArrayTupleRange(someArray);
  for (auto tupleRef : range)
  {
    std::cout << "Tuple: ";
    for (auto compRef : tupleRef)
    {
      std::cout << compRef << " ";
    }
    std::cout << "\n";
  }

  // The GetSubRange method can be used to create a new TupleRange that
  // spans a portion of the original range:
  {
    auto fullRange = vtk::DataArrayTupleRange(someArray);
    // Arguments are (beginOffset, endOffset)
    auto range_2_thru_8 = fullRange.GetSubRange(2, 8);
    // Arguments are relative to the current range:
    auto range_3_thru_6 = range_2_thru_8.GetSubRange(1, 4);
    // If the second arg is omitted, the new range uses the parent's end point:
    auto range_4_thru_6 = range_3_thru_6.GetSubRange(1);

    // Compiler warnings:
    (void)range_4_thru_6;
  }

  // decltype can be used to deduce the exact type of the TupleRange. The
  // TupleRange classes provide additional type aliases that can be used to
  // refer to specific types in the iterator/reference hierarchy:
  using RangeType = decltype(range);

  // These specific type names are available, though just using `auto` is
  // sufficient in most cases. The usage of these types is detailed in the
  // sections that follow.

  // Tuple iterators:
  using TupleIteratorType = typename RangeType::TupleIteratorType;
  using ConstTupleIteratorType = typename RangeType::ConstTupleIteratorType;

  // Tuple references:
  using TupleReferenceType = typename RangeType::TupleReferenceType;
  using ConstTupleReferenceType = typename RangeType::ConstTupleReferenceType;

  // Component iterators:
  using ComponentIteratorType = typename RangeType::ComponentIteratorType;
  using ConstComponentIteratorType = typename RangeType::ConstComponentIteratorType;

  // Component references (ie. `T&` and `const T&`):
  using ComponentReferenceType = typename RangeType::ComponentReferenceType;
  using ConstComponentReferenceType = typename RangeType::ConstComponentReferenceType;

  // Component valuetype (ie. `T`):
  using ComponentType = typename RangeType::ComponentType;

  /////////////////////////
  // TupleRange methods: //
  /////////////////////////

  range.GetArray();        // Returns someArray
  range.GetTupleSize();    // Returns someArray->GetNumberOfComponents()
  range.GetBeginTupleId(); // Returns start of tuple range
  range.GetEndTupleId();   // Returns end of tuple range.
  range.size();            // Returns the number of tuples in the range.
  range[4];                // Returns a TupleReference of the range's 5th tuple.
  range[4][2];             // Returns the 3rd component of the 5th tuple

  // Returns an iterator pointing to the first tuple.
  TupleIteratorType iter = range.begin();
  // Returns a const iterator at the first tuple.
  ConstTupleIteratorType citer = range.cbegin();

  // Returns an iterator pointing to the end tuple.
  TupleIteratorType end = range.end();
  // Returns a const iterator at the end tuple.
  ConstTupleIteratorType cend = range.cend();

  // For-loop syntax:
  for (TupleReferenceType tupleReference : range)
  {
    (void)tupleReference;
  }
  for (ConstTupleReferenceType tupleReference : range)
  {
    (void)tupleReference;
  }

  // `auto` here will always be either TupleReferenceType or
  // ConstTupleReferenceType, depending on whether `range` is const-qualified.
  // For component references and value references, `auto`
  // should be used with care (see explanations in those sections).
  for (auto tupleReference : range)
  {
    (void)tupleReference;
  }

  /////////////////////////////////////
  // Tuple iterator supported usage: //
  /////////////////////////////////////

  // Dereference:
  // Dereference to obtain the current [Const]TupleReference
  TupleReferenceType tuple = *iter;
  // ...or a reference to an offset tuple.
  ConstTupleReferenceType ctuple = citer[3];

  // Traversal:
  ++iter;
  --iter; // prefix increment/decrement behavior
  iter++;
  iter--;          // postfix increment/decrement behavior
  iter += 3;       // increment
  iter = iter - 3; // addition, assignment

  // Assignment:
  iter = range.begin(); // Iterators can be reassigned.
  // can assign const iterators from non-const iterators from same range.
  citer = iter;

  // Comparison:
  if (iter == end)
  {
  } // All of these work as expected
  if (iter != end)
  {
  }
  if (iter < end)
  {
  }
  if (iter <= end)
  {
  }
  if (iter > end)
  {
  }
  if (iter >= end)
  {
  }

  // Math
  assert((end - iter) == std::distance(iter, end)); // distance

  /////////////////////////////////////
  // Tuple reference supported usage //
  /////////////////////////////////////

  // Obtaining:
  TupleReferenceType tuple1 = *iter;         // tuple iterator dereference
  ConstTupleReferenceType tuple2 = citer[1]; // tuple iterator indexed access
  TupleReferenceType tuple3 = range[3];      // range indexed access

  // For-loop syntax:
  for (ComponentReferenceType component : tuple1)
  {
    (void)component;
  }
  for (ConstComponentReferenceType component : tuple1)
  {
    (void)component;
  }
  for (ComponentType component : tuple1)
  {
    (void)component;
  }

  // Due to limitations of C++, auto should be used with care; depending on
  // the implementation of the array, `auto` may have either value or reference
  // semantics. The rule of thumb is: only read from auto variables in a
  // for-range loop. If writing to them, use RangeType::ComponentReferenceType
  // or RangeType::ComponentType explicitly.
  for (auto component : range)
  {
    (void)component;
  }

  // Assignment:
  tuple1 = tuple2; // Component-wise copy of values from tuple2 into tuple1

  // Comparison:
  assert(tuple1 == tuple2); // Component-wise comparisons of tuple values
  assert(tuple1 != tuple3);

  // Indexing
  tuple3[1] = tuple1[0]; // Access tuple components with []-brackets

  // Misc:
  tuple3.fill(0); // Sets all components to 0
  tuple1.size();  // Returns number of components in tuple.

  // Copy to/from pointer:
  std::vector<typename decltype(range)::ComponentType> vec(tuple1.size());
  tuple2.GetTuple(vec.data()); // Copy values from tuple1 into vec
  tuple1.SetTuple(vec.data()); // Copy values from vec into tuple2

  // Component iterators
  ComponentIteratorType compIter = tuple1.begin();
  ComponentIteratorType compEnd = tuple1.end();
  ConstComponentIteratorType constCompIter = tuple1.cbegin();
  ConstComponentIteratorType constCompEnd = tuple1.cend();

  ////////////////////////////////////////
  // Component iterator supported usage //
  ////////////////////////////////////////

  // Traversal:
  ++compIter;
  --compIter; // prefix increment/decrement behavior
  compIter++;
  compIter--;              // postfix increment/decrement behavior
  compIter += 3;           // increment
  compIter = compIter - 3; // addition, assignment

  // Dereference:
  // Dereference to get the current [Const]ComponentReference
  ComponentReferenceType comp = *compIter;
  // ...or a reference to an offset component.
  ConstComponentReferenceType constComp = constCompIter[3];
  // If a ValueType (instead of a reference) is desired, the ComponentType
  // can be used.
  ComponentType compVal = comp;

  // Assignment:
  compIter = tuple1.begin();
  // can assign const iterators from non-const iterators from same range.
  constCompIter = compIter;

  // Comparison:
  if (compIter == compEnd)
  {
  } // All of these work as expected
  if (compIter != compEnd)
  {
  }
  if (compIter < compEnd)
  {
  }
  if (compIter <= compEnd)
  {
  }
  if (compIter > compEnd)
  {
  }
  if (compIter >= compEnd)
  {
  }

  // Math
  assert((compEnd - compIter) == std::distance(compIter, compEnd)); // distance

  // Suppress unsed variable warnings:
  (void)cend;
  (void)constCompEnd;
  (void)tuple;
  (void)ctuple;
  (void)constComp;
  (void)compVal;
}

template <typename ArrayT>
void TestValueRangeAPI(ArrayT* someArray)
{
  // ValueRanges emulate walking a vtkDataArray object using
  // vtkAOSDataArrayTemplate<T>::GetPointer(). That is, ValueRange provides
  // a flat iterator that traverses the components of each tuple without any
  // explicit representation of the tuple abstraction; when one tuple is
  // exhausted, it simply moves to the first component of the next tuple.
  //
  // ValueRange uses the concept of value indices, named for the GetValue
  // method of the common AOS data arrays. A value index describes a location
  // as the offset into an AOS array's data buffer. For example, for an array
  // with 3-component tuples, a value index of 7 refers to the second component
  // of the third tuple:
  //
  // Array:    {X, X, X}, {X, X, X}, {X, X, X}, ...
  // TupleIdx:  0  0  0    1  1  1    2  2  2
  // CompIdx:   0  1  2    0  1  2    0  1  2
  // ValueIdx:  0  1  2    3  4  5    6  7  8
  //
  // As a result, ValueRange uses fewer objects than TupleRange and is more
  // familiar to experienced VTK developers. The ValueRange uses ValueIterators
  // and ValueReferences.

  // A ValueRange can be restricted to a subset of the array's data by passing
  // explicit start/end value indices to vtk::DataArrayValueRange:
  {
    auto subRange = vtk::DataArrayValueRange(someArray, 3, 19);
    // Iterates over values at value indices 3-18 (inclusive).
    std::cout << "Values: ";
    for (auto value : subRange)
    {
      std::cout << value << " ";
    }
    std::cout << "\n";
  }

  // If the exact number of components in a tuple is known at compile-time,
  // this can be passed as a template argument to vtk::DataArrayValueRange.
  // While the tuple abstraction is not directly used while working with
  // ValueRanges, this will enable additional compiler optimizations in the
  // implementation that can improve performance.
  {
    auto optimizedRange = vtk::DataArrayValueRange<4>(someArray);
    // Accessing data in optimizedRange will be more efficient as variables
    // like array strides are made available to the compiler.
    std::cout << "Values: ";
    for (auto value : optimizedRange)
    {
      std::cout << value << " ";
    }
    std::cout << "\n";
  }

  // Both tuple size and subrange information can be used simultaneously:
  {
    auto optimizedSubRange = vtk::DataArrayValueRange<4>(someArray, 3, 19);
    std::cout << "Values: ";
    for (auto value : optimizedSubRange)
    {
      std::cout << value << " ";
    }
    std::cout << "\n";
  }

  // If tuple size is unknown and the range should encompass the full length of
  // the array, simply pass in the array with no template arguments:
  auto range = vtk::DataArrayValueRange(someArray);
  std::cout << "Values: ";
  for (auto value : range)
  {
    std::cout << value << " ";
  }
  std::cout << "\n";

  // The GetSubRange method can be used to create a new ValueRange that
  // spans a portion of the original range:
  {
    auto fullRange = vtk::DataArrayValueRange(someArray);
    // Arguments are (beginOffset, endOffset)
    auto range_2_thru_8 = fullRange.GetSubRange(2, 8);
    // Arguments are relative to the current range:
    auto range_3_thru_6 = range_2_thru_8.GetSubRange(1, 4);
    // If the second arg is omitted, the new range uses the parent's end point:
    auto range_4_thru_6 = range_3_thru_6.GetSubRange(1);

    // Compiler warnings:
    (void)range_4_thru_6;
  }

  // decltype can be used to deduce the exact type of the ValueRange. The
  // ValueRange classes provide additional type aliases that can be used to
  // refer to specific types in the iterator/reference hierarchy:
  using RangeType = decltype(range);

  // These specific type names are available, though just using `auto` is
  // sufficient in most cases. The usage of these types is detailed in the
  // sections that follow.

  // Value iterators:
  using IteratorType = typename RangeType::IteratorType;
  using ConstIteratorType = typename RangeType::ConstIteratorType;

  // Value references (ie. `T&` and `const T&`):
  using ReferenceType = typename RangeType::ReferenceType;
  using ConstReferenceType = typename RangeType::ConstReferenceType;

  // Value type (ie. `T`)
  using ValueType = typename RangeType::ValueType;

  /////////////////////////
  // ValueRange methods: //
  /////////////////////////

  range.GetArray();        // Returns someArray
  range.GetTupleSize();    // Returns someArray->GetNumberOfComponents()
  range.GetBeginValueId(); // Returns start of value range
  range.GetEndValueId();   // Returns end of value range.
  range.size();            // Returns the number of values in the range.
  range[4];                // Returns a ValueReference of the range's 5th value.

  // Returns an iterator pointing to the first value.
  IteratorType iter = range.begin();
  // Returns a const iterator at the first value.
  ConstIteratorType citer = range.cbegin();

  // Returns an iterator pointing to the end value.
  IteratorType end = range.end();
  // Returns a const iterator at the end value.
  ConstIteratorType cend = range.cend();

  // For-loop syntax:
  for (ReferenceType value : range)
  {
    (void)value;
  }
  for (ConstReferenceType value : range)
  {
    (void)value;
  }
  for (ValueType value : range)
  {
    (void)value;
  }

  // Due to limitations of C++, auto should be used with care; depending on
  // the implementation of the array, `auto` may have either value or reference
  // semantics. The rule of thumb is: only read from auto variables in a
  // for-range loop. If writing to them, use RangeType::ReferenceType or
  // RangeType::ValueType explicitly.
  for (auto value : range)
  {
    (void)value;
  }

  ////////////////////////////////////
  // Value iterator supported usage //
  ////////////////////////////////////

  // Traversal:
  ++iter;
  --iter; // prefix increment/decrement behavior
  citer++;
  citer--;         // postfix increment/decrement behavior
  iter += 3;       // increment
  iter = iter - 3; // addition, assignment

  // Dereference:
  // Dereference to get the current [Const]ValueReference
  ReferenceType valueRef = *iter;
  // ...or a reference to an offset component.
  ConstReferenceType constValueRef = citer[3];
  // If a ValueType (instead of a reference) is desired, the ValueType alias
  // can be used.
  ValueType value = valueRef;

  // Assignment:
  iter = range.begin();
  // can assign const iterators from non-const iterators from same range.
  citer = iter;

  // Comparison:
  if (iter == end)
  {
  } // All of these work as expected
  if (iter != end)
  {
  }
  if (iter < end)
  {
  }
  if (iter <= end)
  {
  }
  if (iter > end)
  {
  }
  if (iter >= end)
  {
  }

  // Math
  assert((end - iter) == std::distance(iter, end)); // distance

  // Suppress unsed variable warnings:
  (void)cend;
  (void)constValueRef;
  (void)value;
}

} // end anon namespace

int ExampleDataArrayRangeAPI(int, char*[])
{
  vtkNew<vtkFloatArray> aosArray;
  aosArray->SetNumberOfComponents(4);
  aosArray->SetNumberOfTuples(10);

  { // Fill with data we don't care about:
    auto range = vtk::DataArrayValueRange<4>(aosArray);
    std::iota(range.begin(), range.end(), 0.f);
  }

  vtkNew<vtkSOADataArrayTemplate<float> > soaArray;
  soaArray->DeepCopy(aosArray);

  // Some vtkDataArray pointers to show that these ranges work with the generic
  // vtkDataArray API:
  vtkDataArray* daAos = aosArray;
  vtkDataArray* daSoa = soaArray;

  TestTupleRangeAPI(aosArray.Get());
  TestTupleRangeAPI(soaArray.Get());
  TestTupleRangeAPI(daAos);
  TestTupleRangeAPI(daSoa);

  TestValueRangeAPI(aosArray.Get());
  TestValueRangeAPI(soaArray.Get());
  TestValueRangeAPI(daAos);
  TestValueRangeAPI(daSoa);

  return EXIT_SUCCESS;
}
