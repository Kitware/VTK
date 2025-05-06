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

#include <viskores/exec/arg/ThreadIndicesBasic3D.h>

#include <viskores/testing/Testing.h>

namespace
{

static constexpr viskores::Id3 ARRAY_SIZE = { 10, 10, 3 };

template <typename T>
struct TestPortal
{
  using ValueType = T;

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return viskores::ReduceProduct(ARRAY_SIZE); }

  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id3 index) const
  {
    VISKORES_TEST_ASSERT(index[0] >= 0, "Bad portal index.");
    VISKORES_TEST_ASSERT(index[1] >= 0, "Bad portal index.");
    VISKORES_TEST_ASSERT(index[2] >= 0, "Bad portal index.");

    VISKORES_TEST_ASSERT(index[0] < ARRAY_SIZE[0], "Bad portal index.");
    VISKORES_TEST_ASSERT(index[1] < ARRAY_SIZE[1], "Bad portal index.");
    VISKORES_TEST_ASSERT(index[2] < ARRAY_SIZE[2], "Bad portal index.");

    auto flatIndex = index[0] + ARRAY_SIZE[0] * (index[1] + ARRAY_SIZE[1] * index[2]);
    return TestValue(flatIndex, ValueType());
  }
};
}

namespace viskores
{
namespace exec
{
namespace arg
{
// Fetch for ArrayPortalTex3D when being used for Loads
template <typename T>
struct Fetch<viskores::exec::arg::FetchTagArrayDirectIn,
             viskores::exec::arg::AspectTagDefault,
             TestPortal<T>>
{
  using ValueType = T;
  using PortalType = const TestPortal<T>&;

  template <typename ThreadIndicesType>
  VISKORES_EXEC ValueType Load(const ThreadIndicesType& indices, PortalType field) const
  {
    return field.Get(indices.GetInputIndex3D());
  }

  template <typename ThreadIndicesType>
  VISKORES_EXEC void Store(const ThreadIndicesType&, PortalType, ValueType) const
  {
  }
};
}
}
}

namespace
{

template <typename T>
struct FetchArrayDirectIn3DTests
{
  void operator()()
  {
    TestPortal<T> execObject;

    using FetchType = viskores::exec::arg::Fetch<viskores::exec::arg::FetchTagArrayDirectIn,
                                                 viskores::exec::arg::AspectTagDefault,
                                                 TestPortal<T>>;

    FetchType fetch;

    viskores::Id index1d = 0;
    viskores::Id3 index3d = { 0, 0, 0 };
    for (viskores::Id k = 0; k < ARRAY_SIZE[2]; ++k)
    {
      index3d[2] = k;
      for (viskores::Id j = 0; j < ARRAY_SIZE[1]; ++j)
      {
        index3d[1] = j;
        for (viskores::Id i = 0; i < ARRAY_SIZE[0]; i++, index1d++)
        {
          index3d[0] = i;
          viskores::exec::arg::ThreadIndicesBasic3D indices(index3d, index1d, index1d, 0, index1d);
          T value = fetch.Load(indices, execObject);
          VISKORES_TEST_ASSERT(test_equal(value, TestValue(index1d, T())),
                               "Got invalid value from Load.");

          value = T(T(2) * value);

          // This should be a no-op, but we should be able to call it.
          fetch.Store(indices, execObject, value);
        }
      }
    }
  }
};

struct TryType
{
  template <typename T>
  void operator()(T) const
  {
    FetchArrayDirectIn3DTests<T>()();
  }
};

void TestExecObjectFetch3D()
{
  viskores::testing::Testing::TryTypes(TryType());
}

} // anonymous namespace

int UnitTestFetchArrayDirectIn3d(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestExecObjectFetch3D, argc, argv);
}
