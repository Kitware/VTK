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

#include <viskores/worklet/ScatterPermutation.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleXGCCoordinates.h>
#include <viskores/cont/CellSetExtrude.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/filter/field_conversion/PointAverage.h>

namespace
{
std::vector<float> points_rz = { 1.72485139f, 0.020562f,   1.73493571f,
                                 0.02052826f, 1.73478011f, 0.02299051f }; //really a vec<float,2>
std::vector<viskores::Int32> topology = { 0, 2, 1 };
std::vector<viskores::Int32> nextNode = { 0, 1, 2 };


struct CopyTopo : public viskores::worklet::WorkletVisitCellsWithPoints
{
  typedef void ControlSignature(CellSetIn, FieldOutCell);
  typedef _2 ExecutionSignature(CellShape, PointIndices);

  template <typename T>
  VISKORES_EXEC T&& operator()(viskores::CellShapeTagWedge, T&& t) const
  {
    return std::forward<T>(t);
  }
};

struct CopyTopoScatter : public viskores::worklet::WorkletVisitCellsWithPoints
{
  typedef void ControlSignature(CellSetIn, FieldOutCell);
  typedef _2 ExecutionSignature(CellShape, PointIndices);

  using ScatterType = viskores::worklet::ScatterPermutation<viskores::cont::StorageTagCounting>;

  template <typename T>
  VISKORES_EXEC T&& operator()(viskores::CellShapeTagWedge, T&& t) const
  {
    return std::forward<T>(t);
  }
};

struct CopyReverseCellCount : public viskores::worklet::WorkletVisitPointsWithCells
{
  typedef void ControlSignature(CellSetIn, FieldOutPoint, FieldOutPoint);
  typedef _2 ExecutionSignature(CellShape, CellCount, CellIndices, _3);

  template <typename CellIndicesType, typename OutVec>
  VISKORES_EXEC viskores::Int32 operator()(viskores::CellShapeTagVertex,
                                           viskores::IdComponent count,
                                           CellIndicesType&& cellIndices,
                                           OutVec& outIndices) const
  {
    cellIndices.CopyInto(outIndices);

    bool valid = true;
    for (viskores::IdComponent i = 0; i < count; ++i)
    {
      valid = valid && cellIndices[i] >= 0;
    }
    return (valid && count == cellIndices.GetNumberOfComponents()) ? count : -1;
  }
};

struct CopyReverseCellCountScatter : public viskores::worklet::WorkletVisitPointsWithCells
{
  typedef void ControlSignature(CellSetIn, FieldOutPoint, FieldOutPoint);
  typedef _2 ExecutionSignature(CellShape, CellCount, CellIndices, _3);

  using ScatterType = viskores::worklet::ScatterPermutation<viskores::cont::StorageTagCounting>;

  template <typename CellIndicesType, typename OutVec>
  VISKORES_EXEC viskores::Int32 operator()(viskores::CellShapeTagVertex,
                                           viskores::IdComponent count,
                                           CellIndicesType&& cellIndices,
                                           OutVec& outIndices) const
  {
    cellIndices.CopyInto(outIndices);

    bool valid = true;
    for (viskores::IdComponent i = 0; i < count; ++i)
    {
      valid = valid && cellIndices[i] >= 0;
    }
    return (valid && count == cellIndices.GetNumberOfComponents()) ? count : -1;
  }
};

template <typename T, typename S>
void verify_topo(viskores::cont::ArrayHandle<viskores::Vec<T, 6>, S> const& handle,
                 viskores::Id expectedLen,
                 viskores::Id skip)
{
  auto portal = handle.ReadPortal();
  VISKORES_TEST_ASSERT((portal.GetNumberOfValues() * skip) == expectedLen,
                       "topology portal size is incorrect");

  for (viskores::Id i = 0; i < expectedLen; i += skip)
  {
    auto v = portal.Get(i / skip);
    viskores::Vec<viskores::Id, 6> e;
    viskores::Id offset1 = i * static_cast<viskores::Id>(topology.size());
    viskores::Id offset2 =
      (i < expectedLen - 1) ? (offset1 + static_cast<viskores::Id>(topology.size())) : 0;
    e[0] = (static_cast<viskores::Id>(topology[0]) + offset1);
    e[1] = (static_cast<viskores::Id>(topology[1]) + offset1);
    e[2] = (static_cast<viskores::Id>(topology[2]) + offset1);
    e[3] = (static_cast<viskores::Id>(topology[0]) + offset2);
    e[4] = (static_cast<viskores::Id>(topology[1]) + offset2);
    e[5] = (static_cast<viskores::Id>(topology[2]) + offset2);
    std::cout << "v, e: " << v << ", " << e << "\n";
    VISKORES_TEST_ASSERT(test_equal(v, e), "incorrect conversion of topology to Cartesian space");
  }
}

void verify_reverse_topo(const viskores::cont::ArrayHandle<viskores::Int32>& counts,
                         const viskores::cont::ArrayHandle<viskores::Id2>& indices,
                         viskores::Id expectedLen,
                         viskores::Id skip)
{
  auto countsPortal = counts.ReadPortal();
  VISKORES_TEST_ASSERT((countsPortal.GetNumberOfValues() * skip) == expectedLen,
                       "topology portal size is incorrect");
  auto indicesPortal = indices.ReadPortal();
  VISKORES_TEST_ASSERT((indicesPortal.GetNumberOfValues() * skip) == expectedLen);
  for (viskores::Id i = 0; i < expectedLen - 1; i += skip)
  {
    auto vCount = countsPortal.Get(i / skip);
    auto vIndices = indicesPortal.Get(i / skip);
    std::cout << vCount << ":" << vIndices << " ";
    viskores::Int32 eCount = 2;
    viskores::Id2 eIndices((i / 3) - 1, i / 3);
    if (eIndices[0] < 0)
    {
      eIndices[0] = (expectedLen / 3) - 1;
    }
    VISKORES_TEST_ASSERT(vCount == eCount);
    VISKORES_TEST_ASSERT(vIndices == eIndices);
  }
  std::cout << "\n";
}
int TestCellSetExtrude()
{
  const std::size_t numPlanes = 8;

  auto coords = viskores::cont::make_ArrayHandleXGCCoordinates(points_rz, numPlanes, false);
  auto cells = viskores::cont::make_CellSetExtrude(topology, coords, nextNode);
  VISKORES_TEST_ASSERT(cells.GetNumberOfPoints() == coords.GetNumberOfValues(),
                       "number of points don't match between cells and coordinates");

  viskores::cont::Invoker invoke;

  std::cout << "Verify the topology by copying it into another array\n";
  {
    viskores::cont::ArrayHandle<viskores::Vec<int, 6>> output;
    invoke(CopyTopo{}, cells, output);
    verify_topo(output, numPlanes, 1);
  }

  std::cout << "Verify the topology works with a scatter\n";
  {
    constexpr viskores::Id skip = 2;
    viskores::cont::ArrayHandle<viskores::Vec<int, 6>> output;
    invoke(CopyTopoScatter{},
           CopyTopoScatter::ScatterType(
             viskores::cont::make_ArrayHandleCounting<viskores::Id>(0, skip, numPlanes / skip)),
           cells,
           output);
    verify_topo(output, numPlanes, skip);
  }

  std::cout << "Verify the reverse topology by copying the number of cells each point is "
            << "used by it into another array.\n";
  {
    viskores::cont::ArrayHandle<viskores::Int32> incidentCount;
    viskores::cont::ArrayHandle<viskores::Id2> incidentIndices;
    invoke(CopyReverseCellCount{}, cells, incidentCount, incidentIndices);
    verify_reverse_topo(incidentCount, incidentIndices, 3 * numPlanes, 1);
  }

  std::cout << "Verify reverse topology map with scatter\n";
  {
    constexpr viskores::Id skip = 2;
    viskores::cont::ArrayHandle<viskores::Int32> incidentCount;
    viskores::cont::ArrayHandle<viskores::Id2> incidentIndices;
    invoke(CopyReverseCellCountScatter{},
           CopyTopoScatter::ScatterType(viskores::cont::make_ArrayHandleCounting<viskores::Id>(
             0, skip, (3 * numPlanes) / skip)),
           cells,
           incidentCount,
           incidentIndices);
    verify_reverse_topo(incidentCount, incidentIndices, 3 * numPlanes, skip);
  }

  return 0;
}
}

int UnitTestCellSetExtrude(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestCellSetExtrude, argc, argv);
}
