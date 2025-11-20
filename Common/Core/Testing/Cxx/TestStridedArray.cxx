// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataArray.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkStridedArray.h"
#include "vtkTestUtilities.h"

#ifdef VTK_DISPATCH_STRIDED_ARRAYS
#include "vtkArrayDispatch.h"
namespace
{
struct DispatcherCheckerWorker
{
  template <typename Array>
  void operator()(Array* vtkNotUsed(array))
  {
  }
};
}
#endif // VTK_DISPATCH_STRIDED_ARRAYS

#include <cstdlib>
#include <vector>

#include <iostream>

namespace
{
/**
 * Describes the memory buffer.
 * This is an AOS layout, with a structure of 3 components and 10 tuples.
 * The test will mainly manipulate the value stored at index checkedBufferIdx,
 * which can be seen as data[2][2] and is initialized with "3002"
 */
namespace buffer
{
constexpr int nbOfArrays = 3;
constexpr int nbOfTuples = 10;
constexpr int totalSize = nbOfArrays * nbOfTuples;

// the test overrides and checks the "3002" value.
constexpr int checkedBufferIdx = 8;
// tests should copy it to avoid side effects
std::vector<float> arrayBuffer()
{
  // clang-format off
    return {
      1000, 2000, 3000,
      1001, 2001, 3001,
      1002, 2002, 3002,
      1003, 2003, 3003,
      1004, 2004, 3004,
      1005, 2005, 3005,
      1006, 2006, 3006,
      1007, 2007, 3007,
      1008, 2008, 3008,
      1009, 2009, 3009,
    };
  // clang-format on
}

/**
 * debugging purpose, print the array content, one row per tuple.
 */
void printArray(const std::vector<float>& vec)
{
  for (int tupleIdx = 0; tupleIdx < buffer::nbOfTuples; tupleIdx++)
  {
    for (int arrayIdx = 0; arrayIdx < buffer::nbOfArrays; arrayIdx++)
    {
      std::cout << vec[tupleIdx * buffer::nbOfArrays + arrayIdx] << " ";
    }
    std::cout << "\n";
  }
}
}

/**
 * Describe the strided array.
 * It has same number of tuples than the buffer and stride accordingly.
 * It has 2 components with offset of 1, meaning that the first value of each tuple-3 of the buffer
 * is not used. (i.e. the "100X" values will not be seen by the strided array)
 * The test will mainly manipulate the value stored at component 1 of tuple 2
 * (initialized with "3002", see buffer)
 */
namespace strided
{
constexpr vtkIdType nbOfTuples = buffer::nbOfTuples;
constexpr int nbOfComponents = 2;
constexpr int stride = buffer::nbOfArrays;
constexpr int offset = 1;

// arbitrarily check data for component 1 of tuple 2.
constexpr vtkIdType checkedTupleIdx = 2;
constexpr int checkedCompIdx = 1;
// this should match the same bytes as pointed by checkedBufferIdx
static_assert(
  checkedTupleIdx * buffer::nbOfArrays + checkedCompIdx + offset == buffer::checkedBufferIdx);

constexpr vtkIdType checkedValueIdx = checkedTupleIdx * strided::nbOfComponents + checkedCompIdx;
}

/**
 * vtkGenericDataArray requires some methods to be defined in subclass.
 * Test their implementations in vtkStridedArray
 */
bool TestGenericDataArrayAPI()
{
  std::vector<float> localBuffer = buffer::arrayBuffer();
  const std::vector<float> initialValues = localBuffer;

  vtkNew<vtkStridedArray<float>> stridedArray;
  stridedArray->SetNumberOfComponents(strided::nbOfComponents);
  stridedArray->SetNumberOfTuples(strided::nbOfTuples);
  stridedArray->ConstructBackend(
    localBuffer.data(), strided::stride, strided::nbOfComponents, strided::offset);

  // Test SetValue
  // buffer should not be modified accordingly
  const float setVal = -0.1;
  stridedArray->SetValue(strided::checkedValueIdx, setVal);

  if (setVal == localBuffer[buffer::checkedBufferIdx])
  {
    std::cerr << "SetValue should not write in read-only array. Has " << setVal << " instead of "
              << initialValues[buffer::checkedBufferIdx] << "\n";
    buffer::printArray(localBuffer);
    return false;
  }

  // Test GetValue
  const float getVal = stridedArray->GetValue(strided::checkedValueIdx);
  if (getVal != initialValues[buffer::checkedBufferIdx])
  {
    std::cerr << "Wrong GetValue result " << getVal << " "
              << initialValues[buffer::checkedBufferIdx] << "\n";
    return false;
  }

  // Test GetTypedTuple
  float typedTuple[strided::nbOfComponents];
  stridedArray->GetTypedTuple(strided::checkedTupleIdx, typedTuple);
  if (typedTuple[strided::checkedCompIdx] != localBuffer[buffer::checkedBufferIdx])
  {
    std::cerr << "Wrong GetTypedTyple result\n";
    buffer::printArray(localBuffer);
    return false;
  }

  // Test SetTypedTuple
  typedTuple[strided::checkedCompIdx] = -0.2;
  stridedArray->SetTypedTuple(strided::checkedTupleIdx, typedTuple);
  if (typedTuple[strided::checkedCompIdx] == localBuffer[buffer::checkedBufferIdx])
  {
    std::cerr << "Wrong SetTypedTyple result\n";
    buffer::printArray(localBuffer);
    return false;
  }

  // GetTypedComponent
  const float getTypedComp =
    stridedArray->GetTypedComponent(strided::checkedTupleIdx, strided::checkedCompIdx);
  if (getTypedComp != localBuffer[buffer::checkedBufferIdx])
  {
    std::cerr << "Wrong GetTypedComponent result " << getTypedComp << " "
              << localBuffer[buffer::checkedBufferIdx] << "\n";
    buffer::printArray(localBuffer);
    return false;
  }

  // SetTypedComponent
  const float setTypedComp = -0.3;
  stridedArray->SetTypedComponent(strided::checkedTupleIdx, strided::checkedCompIdx, setTypedComp);
  if (setTypedComp == localBuffer[buffer::checkedBufferIdx])
  {
    std::cerr << "Wrong SetTypedComponent result\n";
    buffer::printArray(localBuffer);
    return false;
  }

  return true;
}

/**
 * vtkStridedArray does not own its memory.
 * Check that usual memory-related methods are no-op,
 * and (smoke) test that there are no runtime errors / exceptions.
 */
bool TestMemoryAllocations()
{
  {
    std::vector<float> localBuffer = buffer::arrayBuffer();
    vtkNew<vtkStridedArray<float>> stridedArray;
    stridedArray->SetNumberOfComponents(strided::nbOfComponents);
    stridedArray->SetNumberOfTuples(strided::nbOfTuples);
    stridedArray->ConstructBackend(
      localBuffer.data(), strided::stride, strided::nbOfComponents, strided::offset);

    // Check no-op methods
    // ---------
    // MaxId should be -1, GetNumberOfValues == 0
    stridedArray->Allocate(0);
    vtkIdType nextNbOfTuples = stridedArray->GetNumberOfTuples();
    if (nextNbOfTuples != 0) // is 0 after any allocate
    {
      std::cerr << "Allocate should reset number of tuples, but still has " << nextNbOfTuples
                << "\n";
      return false;
    }
  }

  {
    std::vector<float> localBuffer = buffer::arrayBuffer();
    vtkNew<vtkStridedArray<float>> stridedArray;
    stridedArray->SetNumberOfComponents(strided::nbOfComponents);
    stridedArray->SetNumberOfTuples(strided::nbOfTuples);
    stridedArray->ConstructBackend(
      localBuffer.data(), strided::stride, strided::nbOfComponents, strided::offset);

    // increasing size is no-op
    stridedArray->Resize(buffer::totalSize * 2);
    vtkIdType nextNbOfValues = stridedArray->GetNumberOfValues();
    if (nextNbOfValues != strided::nbOfComponents * strided::nbOfTuples)
    {
      std::cerr << "Resize should reset number of tuples, but still has " << nextNbOfValues << "\n";
      return false;
    }

    // shrinking array. Memory is untouched but MaxId / Size are updated.
    stridedArray->Resize(2);
    nextNbOfValues = stridedArray->GetNumberOfValues();
    if (nextNbOfValues != 4)
    {
      std::cerr << "Resize should reset number of tuples, but still has " << nextNbOfValues << "\n";
      return false;
    }
  }
  return true;
}

/**
 * Test Copy methods
 */
bool TestCopies()
{
  std::vector<float> localBuffer = buffer::arrayBuffer();
  vtkNew<vtkStridedArray<float>> stridedArray;
  stridedArray->SetNumberOfComponents(strided::nbOfComponents);
  stridedArray->SetNumberOfTuples(strided::nbOfTuples);
  stridedArray->ConstructBackend(
    localBuffer.data(), strided::stride, strided::nbOfComponents, strided::offset);

  vtkNew<vtkStridedArray<float>> copy;
  copy->ImplicitDeepCopy(stridedArray.Get());

  if (!vtkTestUtilities::CompareAbstractArray(copy, stridedArray))
  {
    std::cerr << "Copied array differs from source\n";
    return false;
  }

  vtkSmartPointer<vtkDataArray> shallowCopy = vtk::TakeSmartPointer(stridedArray->NewInstance());
  shallowCopy->ShallowCopy(stridedArray);
  if (!vtkTestUtilities::CompareAbstractArray(shallowCopy, stridedArray))
  {
    std::cerr << "Error in Shallow copy.\n";
    return false;
  }

  vtkSmartPointer<vtkDataArray> deepCopy = vtk::TakeSmartPointer(stridedArray->NewInstance());
  deepCopy->DeepCopy(stridedArray);
  if (!vtkTestUtilities::CompareAbstractArray(deepCopy, stridedArray))
  {
    std::cerr << "Error in Deep copy.\n";
    return false;
  }

  return true;
}

}

/**
 * Test vtkStridedArray using 2-components array at offset 1
 * base on an externally allocated buffer.
 */
int TestStridedArray(int, char*[])
{
  if (!::TestGenericDataArrayAPI())
  {
    std::cerr << "Error with vtkGenericDataArray API test\n";
    return EXIT_FAILURE;
  }

  if (!::TestMemoryAllocations())
  {
    std::cerr << "Error with memory allocations checks.\n";
    return EXIT_FAILURE;
  }

  if (!::TestCopies())
  {
    std::cerr << "Error with copy\n";
    return EXIT_FAILURE;
  }
#ifdef VTK_DISPATCH_STRIDED_ARRAYS
  using Dispatcher = vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::AllArrays>;
  DispatcherCheckerWorker worker;
  vtkNew<vtkStridedArray<float>> stridedArrayFloat;
  if (!Dispatcher::Execute(stridedArrayFloat, worker))
  {
    return EXIT_FAILURE;
  }
  vtkNew<vtkStridedArray<double>> stridedArrayDouble;
  if (!Dispatcher::Execute(stridedArrayDouble, worker))
  {
    return EXIT_FAILURE;
  }
#endif // VTK_DISPATCH_IMPLICIT_POINT_ARRAYS

  return EXIT_SUCCESS;
}
