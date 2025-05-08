//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

// This test does not really need a device compiler
#define VISKORES_NO_ERROR_ON_MIXED_CUDA_CXX_TAG

#include <viskores/cont/arg/TypeCheckTagArrayIn.h>
#include <viskores/cont/arg/TypeCheckTagArrayInOut.h>
#include <viskores/cont/arg/TypeCheckTagArrayOut.h>
#include <viskores/cont/arg/TypeCheckTagAtomicArray.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCompositeVector.h>
#include <viskores/cont/ArrayHandleCounting.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

struct TryArraysOfType
{
  template <typename T>
  void operator()(T) const
  {
    using viskores::cont::arg::TypeCheck;
    using viskores::cont::arg::TypeCheckTagArrayIn;
    using viskores::cont::arg::TypeCheckTagArrayInOut;
    using viskores::cont::arg::TypeCheckTagArrayOut;

    using StandardArray = viskores::cont::ArrayHandle<T>;
    VISKORES_TEST_ASSERT((TypeCheck<TypeCheckTagArrayIn, StandardArray>::value),
                         "Standard array type check failed.");
    VISKORES_TEST_ASSERT((TypeCheck<TypeCheckTagArrayInOut, StandardArray>::value),
                         "Standard array type check failed.");
    VISKORES_TEST_ASSERT((TypeCheck<TypeCheckTagArrayOut, StandardArray>::value),
                         "Standard array type check failed.");

    using CountingArray = viskores::cont::ArrayHandleCounting<T>;
    VISKORES_TEST_ASSERT((TypeCheck<TypeCheckTagArrayIn, CountingArray>::value),
                         "Counting array type check failed.");
    VISKORES_TEST_ASSERT((!TypeCheck<TypeCheckTagArrayInOut, CountingArray>::value),
                         "Counting array type check failed.");
    VISKORES_TEST_ASSERT((!TypeCheck<TypeCheckTagArrayOut, CountingArray>::value),
                         "Counting array type check failed.");

    using CompositeArray = viskores::cont::ArrayHandleCompositeVector<StandardArray, StandardArray>;
    VISKORES_TEST_ASSERT((TypeCheck<TypeCheckTagArrayIn, CompositeArray>::value),
                         "Composite array type check failed.");
    VISKORES_TEST_ASSERT((TypeCheck<TypeCheckTagArrayInOut, CompositeArray>::value),
                         "Counting array type check failed.");
    VISKORES_TEST_ASSERT((TypeCheck<TypeCheckTagArrayOut, CompositeArray>::value),
                         "Counting array type check failed.");

    // Just some type that is not a valid array.
    using NotAnArray = typename StandardArray::WritePortalType;
    VISKORES_TEST_ASSERT(!(TypeCheck<TypeCheckTagArrayIn, NotAnArray>::value),
                         "Not an array type check failed.");
    VISKORES_TEST_ASSERT(!(TypeCheck<TypeCheckTagArrayInOut, NotAnArray>::value),
                         "Not an array type check failed.");
    VISKORES_TEST_ASSERT(!(TypeCheck<TypeCheckTagArrayOut, NotAnArray>::value),
                         "Not an array type check failed.");

    // Another type that is not a valid array.
    VISKORES_TEST_ASSERT(!(TypeCheck<TypeCheckTagArrayIn, T>::value),
                         "Not an array type check failed.");
    VISKORES_TEST_ASSERT(!(TypeCheck<TypeCheckTagArrayInOut, T>::value),
                         "Not an array type check failed.");
    VISKORES_TEST_ASSERT(!(TypeCheck<TypeCheckTagArrayOut, T>::value),
                         "Not an array type check failed.");
  }
};

void TestCheckAtomicArray()
{
  std::cout << "Trying some arrays with atomic arrays." << std::endl;
  using viskores::cont::arg::TypeCheck;
  using viskores::cont::arg::TypeCheckTagAtomicArray;

  using Int32Array = viskores::cont::ArrayHandle<viskores::Int32>;
  using Int64Array = viskores::cont::ArrayHandle<viskores::Int64>;
  using FloatArray = viskores::cont::ArrayHandle<viskores::Float32>;

  VISKORES_TEST_ASSERT((TypeCheck<TypeCheckTagAtomicArray, Int32Array>::value),
                       "Check for 32-bit int failed.");
  VISKORES_TEST_ASSERT((TypeCheck<TypeCheckTagAtomicArray, Int64Array>::value),
                       "Check for 64-bit int failed.");
  VISKORES_TEST_ASSERT((TypeCheck<TypeCheckTagAtomicArray, FloatArray>::value),
                       "Check for float failed.");
}

void TestCheckArray()
{
  viskores::testing::Testing::TryTypes(TryArraysOfType());

  TestCheckAtomicArray();
}

} // anonymous namespace

int UnitTestTypeCheckArray(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestCheckArray, argc, argv);
}
