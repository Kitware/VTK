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

#include <viskores/exec/arg/FetchTagArrayDirectInOut.h>

#include <viskores/exec/testing/ThreadIndicesTesting.h>

#include <viskores/testing/Testing.h>

namespace
{

static constexpr viskores::Id ARRAY_SIZE = 10;

static viskores::Id g_NumSets;

template <typename T>
struct TestPortal
{
  using ValueType = T;

  viskores::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  ValueType Get(viskores::Id index) const
  {
    VISKORES_TEST_ASSERT(index >= 0, "Bad portal index.");
    VISKORES_TEST_ASSERT(index < this->GetNumberOfValues(), "Bad portal index.");
    return TestValue(index, ValueType());
  }

  void Set(viskores::Id index, const ValueType& value) const
  {
    VISKORES_TEST_ASSERT(index >= 0, "Bad portal index.");
    VISKORES_TEST_ASSERT(index < this->GetNumberOfValues(), "Bad portal index.");
    VISKORES_TEST_ASSERT(test_equal(value, ValueType(2) * TestValue(index, ValueType())),
                         "Tried to set invalid value.");
    g_NumSets++;
  }
};

template <typename T>
struct FetchArrayDirectInTests
{

  void operator()()
  {
    TestPortal<T> execObject;

    using FetchType = viskores::exec::arg::Fetch<viskores::exec::arg::FetchTagArrayDirectInOut,
                                                 viskores::exec::arg::AspectTagDefault,
                                                 TestPortal<T>>;

    FetchType fetch;

    g_NumSets = 0;

    for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
    {
      viskores::exec::arg::ThreadIndicesTesting indices(index);

      T value = fetch.Load(indices, execObject);
      VISKORES_TEST_ASSERT(test_equal(value, TestValue(index, T())),
                           "Got invalid value from Load.");

      value = T(T(2) * value);

      fetch.Store(indices, execObject, value);
    }

    VISKORES_TEST_ASSERT(g_NumSets == ARRAY_SIZE,
                         "Array portal's set not called correct number of times."
                         "Store method must be wrong.");
  }
};

struct TryType
{
  template <typename T>
  void operator()(T) const
  {
    FetchArrayDirectInTests<T>()();
  }
};

void TestExecObjectFetch()
{
  viskores::testing::Testing::TryTypes(TryType(), viskores::TypeListCommon());
}

} // anonymous namespace

int UnitTestFetchArrayDirectInOut(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestExecObjectFetch, argc, argv);
}
