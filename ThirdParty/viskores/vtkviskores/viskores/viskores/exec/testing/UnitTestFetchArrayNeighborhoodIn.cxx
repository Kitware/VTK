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

#include <viskores/exec/arg/FetchTagArrayNeighborhoodIn.h>
#include <viskores/exec/arg/ThreadIndicesPointNeighborhood.h>

#include <viskores/testing/Testing.h>

namespace
{

static const viskores::Id3 POINT_DIMS = { 10, 4, 16 };

template <typename T>
struct TestPortal
{
  using ValueType = T;

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return POINT_DIMS[0] * POINT_DIMS[1] * POINT_DIMS[2]; }

  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const
  {
    VISKORES_TEST_ASSERT(index >= 0, "Bad portal index.");
    VISKORES_TEST_ASSERT(index < this->GetNumberOfValues(), "Bad portal index.");
    return TestValue(index, ValueType());
  }
};

template <typename NeighborhoodType, typename T>
void verify_neighbors(NeighborhoodType neighbors, viskores::Id index, viskores::Id3 index3d, T)
{

  T expected;
  auto* boundary = neighbors.Boundary;

  //Verify the boundary flags first
  VISKORES_TEST_ASSERT(((index3d[0] != 0) && (index3d[0] != (POINT_DIMS[0] - 1))) ==
                         boundary->IsRadiusInXBoundary(1),
                       "Got invalid X radius boundary");
  VISKORES_TEST_ASSERT(((index3d[1] != 0) && (index3d[1] != (POINT_DIMS[1] - 1))) ==
                         boundary->IsRadiusInYBoundary(1),
                       "Got invalid Y radius boundary");
  VISKORES_TEST_ASSERT(((index3d[2] != 0) && (index3d[2] != (POINT_DIMS[2] - 1))) ==
                         boundary->IsRadiusInZBoundary(1),
                       "Got invalid Z radius boundary");

  VISKORES_TEST_ASSERT((index3d[0] != 0) == boundary->IsNeighborInXBoundary(-1),
                       "Got invalid X negative neighbor boundary");
  VISKORES_TEST_ASSERT((index3d[1] != 0) == boundary->IsNeighborInYBoundary(-1),
                       "Got invalid Y negative neighbor boundary");
  VISKORES_TEST_ASSERT((index3d[2] != 0) == boundary->IsNeighborInZBoundary(-1),
                       "Got invalid Z negative neighbor boundary");

  VISKORES_TEST_ASSERT((index3d[0] != (POINT_DIMS[0] - 1)) == boundary->IsNeighborInXBoundary(1),
                       "Got invalid X positive neighbor boundary");
  VISKORES_TEST_ASSERT((index3d[1] != (POINT_DIMS[1] - 1)) == boundary->IsNeighborInYBoundary(1),
                       "Got invalid Y positive neighbor boundary");
  VISKORES_TEST_ASSERT((index3d[2] != (POINT_DIMS[2] - 1)) == boundary->IsNeighborInZBoundary(1),
                       "Got invalid Z positive neighbor boundary");

  VISKORES_TEST_ASSERT(
    ((boundary->MinNeighborIndices(1)[0] == -1) && (boundary->MaxNeighborIndices(1)[0] == 1)) ==
      boundary->IsRadiusInXBoundary(1),
    "Got invalid min/max X indices");
  VISKORES_TEST_ASSERT(
    ((boundary->MinNeighborIndices(1)[1] == -1) && (boundary->MaxNeighborIndices(1)[1] == 1)) ==
      boundary->IsRadiusInYBoundary(1),
    "Got invalid min/max Y indices");
  VISKORES_TEST_ASSERT(
    ((boundary->MinNeighborIndices(1)[2] == -1) && (boundary->MaxNeighborIndices(1)[2] == 1)) ==
      boundary->IsRadiusInZBoundary(1),
    "Got invalid min/max Z indices");

  T forwardX = neighbors.Get(1, 0, 0);
  expected = (index3d[0] == POINT_DIMS[0] - 1) ? TestValue(index, T()) : TestValue(index + 1, T());
  VISKORES_TEST_ASSERT(test_equal(forwardX, expected), "Got invalid value from Load.");

  T backwardsX = neighbors.Get(-1, 0, 0);
  expected = (index3d[0] == 0) ? TestValue(index, T()) : TestValue(index - 1, T());
  VISKORES_TEST_ASSERT(test_equal(backwardsX, expected), "Got invalid value from Load.");
}


template <typename T>
struct FetchArrayNeighborhoodInTests
{
  void operator()()
  {
    TestPortal<T> execObject;

    using FetchType = viskores::exec::arg::Fetch<viskores::exec::arg::FetchTagArrayNeighborhoodIn,
                                                 viskores::exec::arg::AspectTagDefault,
                                                 TestPortal<T>>;

    FetchType fetch;



    viskores::internal::ConnectivityStructuredInternals<3> connectivityInternals;
    connectivityInternals.SetPointDimensions(POINT_DIMS);
    viskores::exec::
      ConnectivityStructured<viskores::TopologyElementTagPoint, viskores::TopologyElementTagCell, 3>
        connectivity(connectivityInternals);

    // Verify that 3D scheduling works with neighborhoods
    {
      viskores::Id3 index3d;
      viskores::Id index = 0;
      for (viskores::Id k = 0; k < POINT_DIMS[2]; k++)
      {
        index3d[2] = k;
        for (viskores::Id j = 0; j < POINT_DIMS[1]; j++)
        {
          index3d[1] = j;
          for (viskores::Id i = 0; i < POINT_DIMS[0]; i++, index++)
          {
            index3d[0] = i;
            viskores::exec::arg::ThreadIndicesPointNeighborhood indices(
              index3d, index, connectivity);

            auto neighbors = fetch.Load(indices, execObject);

            T value = neighbors.Get(0, 0, 0);
            VISKORES_TEST_ASSERT(test_equal(value, TestValue(index, T())),
                                 "Got invalid value from Load.");

            //We now need to check the neighbors.
            verify_neighbors(neighbors, index, index3d, value);

            // This should be a no-op, but we should be able to call it.
            fetch.Store(indices, execObject, neighbors);
          }
        }
      }
    }

    //Verify that 1D scheduling works with neighborhoods
    for (viskores::Id index = 0; index < (POINT_DIMS[0] * POINT_DIMS[1] * POINT_DIMS[2]); index++)
    {
      viskores::exec::arg::ThreadIndicesPointNeighborhood indices(
        index, index, 0, index, connectivity);

      auto neighbors = fetch.Load(indices, execObject);

      T value = neighbors.Get(0, 0, 0); //center value
      VISKORES_TEST_ASSERT(test_equal(value, TestValue(index, T())),
                           "Got invalid value from Load.");


      const viskores::Id indexij = index % (POINT_DIMS[0] * POINT_DIMS[1]);
      viskores::Id3 index3d(
        indexij % POINT_DIMS[0], indexij / POINT_DIMS[0], index / (POINT_DIMS[0] * POINT_DIMS[1]));

      //We now need to check the neighbors.
      verify_neighbors(neighbors, index, index3d, value);

      // This should be a no-op, but we should be able to call it.
      fetch.Store(indices, execObject, neighbors);
    }
  }
};

struct TryType
{
  template <typename T>
  void operator()(T) const
  {
    FetchArrayNeighborhoodInTests<T>()();
  }
};

void TestExecNeighborhoodFetch()
{
  viskores::testing::Testing::TryTypes(TryType());
}

} // anonymous namespace

int UnitTestFetchArrayNeighborhoodIn(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestExecNeighborhoodFetch, argc, argv);
}
