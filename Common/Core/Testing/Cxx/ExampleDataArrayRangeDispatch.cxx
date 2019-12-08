/*==============================================================================

  Program:   Visualization Toolkit
  Module:    ExampleDataArrayRangeDispatch.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

==============================================================================*/

// This file provides some examples of how to use the ArrayDispatch system
// with the DataArrayRange utilities to create high performance algorithms that
// will work with all vtkDataArrays.
//
// ArrayDispatch provides a mechanism to automatically downcast a vtkDataArray
// object to a subclass (such as vtkFloatArray, vtkSOADataArrayTemplate<int>,
// etc). Using a subclass instead of a vtkDataArray allows the array data
// to be accessed directly in memory, while using the vtkDataArray API requires
// virtual methods and type conversions.
//
// The vtk::DataArrayRange utilities provide STL-style iterators that abstract
// the details of reading/writing values in a vtkDataArray. When used with a
// specific subclass of vtkDataArray, optimized memory access are used. When
// used with a vtkDataArray, the virtual interface is used. This allows a
// single algorithm to be written using these Range objects that will serve
// as both a fast-path for common array types, while also functioning as a
// slower fallback path for uncommon array types.
//
// This example fills an array with values increasing from [0, size), sums the
// values in the array, and then copies data into another array.
//
// The fill is performed using the STL function `std::iota` to show how the
// Ranges interact with the standard library.
//
// The sum is performed using `std::accumulate`.
//
// The copy is performed using a C++11-style for-range loop to retrieve the
// source values, and a raw iterator is used to write the output to the
// destination array. This demonstrates the similarity between using the Range
// iterators and the more familiar pointer-style algorithms.

#include "vtkDataArrayRange.h"

#include "vtkArrayDispatch.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"

#include <algorithm>
#include <numeric>

namespace
{

// Fills the supplied array with increasing values, starting from 0.
struct FillImpl
{
  template <typename ArrayType>
  void operator()(ArrayType* array) const
  {
    // We know this is a single component array, so providing a template
    // parameter of `1` enables additional optimizations. The template
    // parameter may be omitted if the tuple size is unknown:
    auto range = vtk::DataArrayValueRange<1>(array);

    // The begin/end methods return STL-style iterators, similar to std::vector.
    std::iota(range.begin(), range.end(), 0);
  }
};

// Adds all values in Range into Sum.
struct SumImpl
{
  double Sum{ 0 };

  template <typename ArrayType>
  void operator()(ArrayType* array)
  {
    const auto range = vtk::DataArrayValueRange<1>(array);

    // The cbegin/cend methods return constant iterators that prevent the data
    // they point at from being modified.
    this->Sum = std::accumulate(range.cbegin(), range.cend(), 0);
  }
};

// Copies from the src range to the dst range.
struct CopyImpl
{
  template <typename SrcArray, typename DstArray>
  void operator()(SrcArray* src, DstArray* dst) const
  {
    // The vtk::GetAPIType<ArrayType> helper provides the API type of the
    // data array (e.g. `float` for vtkFloatArray, `double` for the virtual
    // vtkDataArray API).
    using SrcValueType = vtk::GetAPIType<SrcArray>;
    using DstValueType = vtk::GetAPIType<DstArray>;

    // Create range object for the arrays. These work whether the arrays are
    // downcasted AOS arrays, SOA array, etc or plain vtkDataArrays.
    const auto srcRange = vtk::DataArrayValueRange<1>(src);
    auto dstRange = vtk::DataArrayValueRange<1>(dst);

    // Ensure that the ranges are the same size:
    assert(srcRange.size() == dstRange.size());

    // We use a C++11-style ranged-for loop to retrieve the values from src,
    // and write to an iterator obtained from dst. These iterators behave
    // like pointers.
    auto dstIter = dstRange.begin();
    for (SrcValueType srcVal : srcRange)
    {
      *dstIter++ = static_cast<DstValueType>(srcVal);
    }
  }
};

// Create an AOS<float> array and return a plain vtkDataArray pointer.
vtkDataArray* CreateArray()
{
  vtkNew<vtkFloatArray> aosArray;
  aosArray->SetNumberOfComponents(1);
  aosArray->SetNumberOfTuples(1024);
  aosArray->Register(nullptr); // Add a reference before we return.
  return aosArray;
}

// std::equal predicate to do "close enough" equality comparisons to work
// around floating point issues.
struct CloseEnough
{
  bool operator()(double a, double b) { return std::fabs(a - b) < 1e-5; }
};

} // end anon namespace

int ExampleDataArrayRangeDispatch(int, char*[])
{
  // Create the array. The auto type here is vtkSmartPointer<vtkDataArray>.
  auto array = vtk::TakeSmartPointer(CreateArray());

  // Attempt to downcast the array to a known type and call the FillImpl
  // functor with the downcasted array:
  FillImpl fillFunctor;
  if (!vtkArrayDispatch::Dispatch::Execute(array, fillFunctor))
  {
    // If Execute returns false, the input array type was not determined because
    // the array was not in the set of supported array types. No worries, we
    // can reuse the functor with the vtkDataArray pointer as a fallback:
    fillFunctor(array.Get());
  }

  // SumImpl:
  SumImpl sumFunctor;
  if (!vtkArrayDispatch::Dispatch::Execute(array, sumFunctor))
  {
    sumFunctor(array.Get());
  }

  // Verify the sum:
  double expected = (1024. * 1023.) / 2.;
  if (std::fabs(sumFunctor.Sum - expected) > 1e-5)
  {
    std::cerr << "Sum was not as expected: " << sumFunctor.Sum << " (expected: " << expected
              << ")\n";
    return EXIT_FAILURE;
  }

  // CopyImpl:
  {
    // Create an array of ints, and copy the other array's data into it.
    // The auto type here is vtkSmartPointer<vtkDataArray>.
    auto intArray = vtk::TakeSmartPointer(vtkDataArray::CreateDataArray(VTK_INT));
    intArray->SetNumberOfComponents(array->GetNumberOfComponents());
    intArray->SetNumberOfTuples(array->GetNumberOfTuples());

    // Dispatch2 dispatches two arrays at once. Various other dispatchers are
    // documented in the vtkArrayDispatch header.
    CopyImpl copyFunctor;
    if (!vtkArrayDispatch::Dispatch2::Execute(array, intArray, copyFunctor))
    {
      copyFunctor(array.Get(), intArray.Get());
    }

    // Check the arrays are equal using the vtkDataArray double interface via
    // range iterators and std::equal:
    const auto srcRange = vtk::DataArrayValueRange<1>(array);
    const auto dstRange = vtk::DataArrayValueRange<1>(intArray);
    if (!std::equal(srcRange.cbegin(), srcRange.cend(), dstRange.cbegin(), CloseEnough{}))
    {
      std::cerr << "Copied values do not match!\n";
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
