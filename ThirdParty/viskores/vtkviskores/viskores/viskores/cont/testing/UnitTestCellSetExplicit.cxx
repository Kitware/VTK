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
#include <viskores/cont/CellSetExplicit.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/RuntimeDeviceTracker.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace
{

using CellTag = viskores::TopologyElementTagCell;
using PointTag = viskores::TopologyElementTagPoint;

const viskores::Id numberOfPoints = 11;

const viskores::UInt8 g_shapes[] = { static_cast<viskores::UInt8>(viskores::CELL_SHAPE_HEXAHEDRON),
                                     static_cast<viskores::UInt8>(viskores::CELL_SHAPE_PYRAMID),
                                     static_cast<viskores::UInt8>(viskores::CELL_SHAPE_TETRA),
                                     static_cast<viskores::UInt8>(viskores::CELL_SHAPE_WEDGE) };
const viskores::UInt8 g_shapes2[] = { g_shapes[1], g_shapes[2] };

const viskores::Id g_offsets[] = { 0, 8, 13, 17, 23 };
const viskores::Id g_offsets2[] = { 0, 5, 9 };

const viskores::Id g_connectivity[] = { 0, 1, 5, 4,  3, 2, 6, 7, 1, 5, 6, 2,
                                        8, 5, 8, 10, 6, 4, 7, 9, 5, 6, 10 };
const viskores::Id g_connectivity2[] = { 1, 5, 6, 2, 8, 5, 8, 10, 6 };

template <typename T, std::size_t Length>
viskores::Id ArrayLength(const T (&)[Length])
{
  return static_cast<viskores::Id>(Length);
}

// all points are part of atleast 1 cell
viskores::cont::CellSetExplicit<> MakeTestCellSet1()
{
  viskores::cont::CellSetExplicit<> cs;
  cs.Fill(
    numberOfPoints,
    viskores::cont::make_ArrayHandle(g_shapes, ArrayLength(g_shapes), viskores::CopyFlag::Off),
    viskores::cont::make_ArrayHandle(
      g_connectivity, ArrayLength(g_connectivity), viskores::CopyFlag::Off),
    viskores::cont::make_ArrayHandle(g_offsets, ArrayLength(g_offsets), viskores::CopyFlag::Off));
  return cs;
}

// some points are not part of any cell
viskores::cont::CellSetExplicit<> MakeTestCellSet2()
{
  viskores::cont::CellSetExplicit<> cs;
  cs.Fill(
    numberOfPoints,
    viskores::cont::make_ArrayHandle(g_shapes2, ArrayLength(g_shapes2), viskores::CopyFlag::Off),
    viskores::cont::make_ArrayHandle(
      g_connectivity2, ArrayLength(g_connectivity2), viskores::CopyFlag::Off),
    viskores::cont::make_ArrayHandle(g_offsets2, ArrayLength(g_offsets2), viskores::CopyFlag::Off));
  return cs;
}

struct WorkletPointToCell : public viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellset, FieldOutCell numPoints);
  using ExecutionSignature = void(PointIndices, _2);
  using InputDomain = _1;

  template <typename PointIndicesType>
  VISKORES_EXEC void operator()(const PointIndicesType& pointIndices, viskores::Id& numPoints) const
  {
    numPoints = pointIndices.GetNumberOfComponents();
  }
};

struct WorkletCellToPoint : public viskores::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn cellset, FieldOutPoint numCells);
  using ExecutionSignature = void(CellIndices, _2);
  using InputDomain = _1;

  template <typename CellIndicesType>
  VISKORES_EXEC void operator()(const CellIndicesType& cellIndices, viskores::Id& numCells) const
  {
    numCells = cellIndices.GetNumberOfComponents();
  }
};

void TestCellSetExplicit()
{
  viskores::cont::CellSetExplicit<> cellset;
  viskores::cont::ArrayHandle<viskores::Id> result;

  std::cout << "----------------------------------------------------\n";
  std::cout << "Testing Case 1 (all points are part of atleast 1 cell): \n";
  cellset = MakeTestCellSet1();

  std::cout << "\tTesting PointToCell\n";
  viskores::worklet::DispatcherMapTopology<WorkletPointToCell>().Invoke(cellset, result);

  VISKORES_TEST_ASSERT(result.GetNumberOfValues() == cellset.GetNumberOfCells(),
                       "result length not equal to number of cells");
  auto portal = result.ReadPortal();
  for (viskores::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    VISKORES_TEST_ASSERT(portal.Get(i) == cellset.GetNumberOfPointsInCell(i), "incorrect result");
  }

  std::cout << "\tTesting CellToPoint\n";
  viskores::worklet::DispatcherMapTopology<WorkletCellToPoint>().Invoke(cellset, result);

  VISKORES_TEST_ASSERT(result.GetNumberOfValues() == cellset.GetNumberOfPoints(),
                       "result length not equal to number of points");

  viskores::Id expected1[] = { 1, 2, 2, 1, 2, 4, 4, 2, 2, 1, 2 };
  portal = result.ReadPortal();
  for (viskores::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    VISKORES_TEST_ASSERT(portal.Get(i) == expected1[i], "incorrect result");
  }

  std::cout << "----------------------------------------------------\n";
  std::cout << "Testing Case 2 (some points are not part of any cell): \n";
  cellset = MakeTestCellSet2();

  std::cout << "\tTesting PointToCell\n";
  viskores::worklet::DispatcherMapTopology<WorkletPointToCell>().Invoke(cellset, result);

  VISKORES_TEST_ASSERT(result.GetNumberOfValues() == cellset.GetNumberOfCells(),
                       "result length not equal to number of cells");
  portal = result.ReadPortal();
  for (viskores::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    VISKORES_TEST_ASSERT(portal.Get(i) == cellset.GetNumberOfPointsInCell(i), "incorrect result");
  }

  std::cout << "\tTesting CellToPoint\n";
  viskores::worklet::DispatcherMapTopology<WorkletCellToPoint>().Invoke(cellset, result);

  VISKORES_TEST_ASSERT(result.GetNumberOfValues() == cellset.GetNumberOfPoints(),
                       "result length not equal to number of points");

  viskores::Id expected2[] = { 0, 1, 1, 0, 0, 2, 2, 0, 2, 0, 1 };
  portal = result.ReadPortal();
  for (viskores::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    VISKORES_TEST_ASSERT(portal.Get(i) == expected2[i], "incorrect result at ", i);
  }

  std::cout << "----------------------------------------------------\n";
  std::cout << "General Testing: \n";

  std::cout << "\tTesting resource releasing in CellSetExplicit\n";
  cellset.ReleaseResourcesExecution();
  VISKORES_TEST_ASSERT(cellset.GetNumberOfCells() == ArrayLength(g_shapes) / 2,
                       "release execution resources should not change the number of cells");
  VISKORES_TEST_ASSERT(cellset.GetNumberOfPoints() == ArrayLength(expected2),
                       "release execution resources should not change the number of points");

  std::cout << "\tTesting CellToPoint table caching\n";
  cellset = MakeTestCellSet2();
  VISKORES_TEST_ASSERT(VISKORES_PASS_COMMAS(cellset.HasConnectivity(CellTag{}, PointTag{})),
                       "PointToCell table missing.");
  VISKORES_TEST_ASSERT(VISKORES_PASS_COMMAS(!cellset.HasConnectivity(PointTag{}, CellTag{})),
                       "CellToPoint table exists before PrepareForInput.");

  // Test a raw PrepareForInput call:
  viskores::cont::Token token;
  {
    viskores::cont::ScopedRuntimeDeviceTracker deviceScope(
      viskores::cont::DeviceAdapterTagSerial{});
    (void)deviceScope;
    cellset.PrepareForInput(viskores::cont::DeviceAdapterTagSerial{}, PointTag{}, CellTag{}, token);
  }

  VISKORES_TEST_ASSERT(VISKORES_PASS_COMMAS(cellset.HasConnectivity(PointTag{}, CellTag{})),
                       "CellToPoint table missing after PrepareForInput.");

  cellset.ResetConnectivity(PointTag{}, CellTag{});
  VISKORES_TEST_ASSERT(VISKORES_PASS_COMMAS(!cellset.HasConnectivity(PointTag{}, CellTag{})),
                       "CellToPoint table exists after resetting.");

  // Test a PrepareForInput wrapped inside a dispatch (See #268)
  viskores::worklet::DispatcherMapTopology<WorkletCellToPoint>().Invoke(cellset, result);
  VISKORES_TEST_ASSERT(VISKORES_PASS_COMMAS(cellset.HasConnectivity(PointTag{}, CellTag{})),
                       "CellToPoint table missing after CellToPoint worklet exec.");
}

} // anonymous namespace

int UnitTestCellSetExplicit(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestCellSetExplicit, argc, argv);
}
