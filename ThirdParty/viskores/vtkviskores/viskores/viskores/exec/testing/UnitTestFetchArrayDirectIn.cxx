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

#include <viskores/exec/arg/FetchTagArrayDirectIn.h>

#include <viskores/exec/testing/ThreadIndicesTesting.h>

#include <viskores/testing/Testing.h>

namespace
{

static constexpr viskores::Id ARRAY_SIZE = 10;

template <typename T>
struct TestPortal
{
  using ValueType = T;

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const
  {
    VISKORES_TEST_ASSERT(index >= 0, "Bad portal index.");
    VISKORES_TEST_ASSERT(index < this->GetNumberOfValues(), "Bad portal index.");
    return TestValue(index, ValueType());
  }
};

template <typename T>
struct FetchArrayDirectInTests
{
  void operator()()
  {
    TestPortal<T> execObject;

    using FetchType = viskores::exec::arg::Fetch<viskores::exec::arg::FetchTagArrayDirectIn,
                                                 viskores::exec::arg::AspectTagDefault,
                                                 TestPortal<T>>;

    FetchType fetch;

    for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
    {
      viskores::exec::arg::ThreadIndicesTesting indices(index);

      T value = fetch.Load(indices, execObject);
      VISKORES_TEST_ASSERT(test_equal(value, TestValue(index, T())),
                           "Got invalid value from Load.");

      value = T(T(2) * value);

      // This should be a no-op, but we should be able to call it.
      fetch.Store(indices, execObject, value);
    }
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
  viskores::testing::Testing::TryTypes(TryType());
}

} // anonymous namespace

int UnitTestFetchArrayDirectIn(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestExecObjectFetch, argc, argv);
}
