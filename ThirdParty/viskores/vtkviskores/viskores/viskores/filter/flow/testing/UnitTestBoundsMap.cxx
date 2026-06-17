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

#include <viskores/CellShape.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/flow/internal/BoundsMap.h>
#include <viskores/worklet/WorkletMapField.h>

#include <algorithm>

namespace
{

struct LocatedCellIds
{
  viskores::IdComponent Count = 0;
  std::vector<viskores::Id> Ids;
};

viskores::cont::DataSet CreateBoundsBox(const viskores::Bounds& bounds)
{
  std::vector<viskores::Vec3f> points = { { static_cast<viskores::FloatDefault>(bounds.X.Min),
                                            static_cast<viskores::FloatDefault>(bounds.Y.Min),
                                            static_cast<viskores::FloatDefault>(bounds.Z.Min) },
                                          { static_cast<viskores::FloatDefault>(bounds.X.Max),
                                            static_cast<viskores::FloatDefault>(bounds.Y.Min),
                                            static_cast<viskores::FloatDefault>(bounds.Z.Min) },
                                          { static_cast<viskores::FloatDefault>(bounds.X.Max),
                                            static_cast<viskores::FloatDefault>(bounds.Y.Max),
                                            static_cast<viskores::FloatDefault>(bounds.Z.Min) },
                                          { static_cast<viskores::FloatDefault>(bounds.X.Min),
                                            static_cast<viskores::FloatDefault>(bounds.Y.Max),
                                            static_cast<viskores::FloatDefault>(bounds.Z.Min) },
                                          { static_cast<viskores::FloatDefault>(bounds.X.Min),
                                            static_cast<viskores::FloatDefault>(bounds.Y.Min),
                                            static_cast<viskores::FloatDefault>(bounds.Z.Max) },
                                          { static_cast<viskores::FloatDefault>(bounds.X.Max),
                                            static_cast<viskores::FloatDefault>(bounds.Y.Min),
                                            static_cast<viskores::FloatDefault>(bounds.Z.Max) },
                                          { static_cast<viskores::FloatDefault>(bounds.X.Max),
                                            static_cast<viskores::FloatDefault>(bounds.Y.Max),
                                            static_cast<viskores::FloatDefault>(bounds.Z.Max) },
                                          { static_cast<viskores::FloatDefault>(bounds.X.Min),
                                            static_cast<viskores::FloatDefault>(bounds.Y.Max),
                                            static_cast<viskores::FloatDefault>(bounds.Z.Max) } };
  std::vector<viskores::UInt8> shapes = { viskores::CELL_SHAPE_HEXAHEDRON };
  std::vector<viskores::IdComponent> numIndices = { 8 };
  std::vector<viskores::Id> connectivity = { 0, 1, 2, 3, 4, 5, 6, 7 };

  return viskores::cont::DataSetBuilderExplicit::Create(points, shapes, numIndices, connectivity);
}

viskores::cont::PartitionedDataSet CreatePartitionedDataSet(
  const std::vector<viskores::Bounds>& bounds)
{
  viskores::cont::PartitionedDataSet pds;
  for (const auto& blockBounds : bounds)
    pds.AppendPartition(CreateBoundsBox(blockBounds));

  return pds;
}

class CountBoundsMapCellsWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn points, ExecObject locator, FieldOut count);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename LocatorType>
  VISKORES_EXEC void operator()(const viskores::Vec3f& point,
                                const LocatorType& locator,
                                viskores::Id& count) const
  {
    count = locator.CountAllCells(point);
  }
};

class FindBoundsMapCellIdsWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn points, ExecObject locator, FieldOut cellIds);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename LocatorType, typename CellIdVecType>
  VISKORES_EXEC void operator()(const viskores::Vec3f& point,
                                const LocatorType& locator,
                                CellIdVecType& cellIds) const
  {
    locator.FindAllCellIds(point, cellIds);
  }
};

std::vector<LocatedCellIds> FindBoundsMapCellIds(
  const viskores::filter::flow::internal::BoundsMap& boundsMap,
  const std::vector<viskores::Vec3f>& points)
{
  auto pointsAH = viskores::cont::make_ArrayHandle(points, viskores::CopyFlag::On);

  viskores::cont::Invoker invoker;
  viskores::cont::ArrayHandle<viskores::Id> cellCounts;
  invoker(CountBoundsMapCellsWorklet{}, pointsAH, boundsMap.GetLocator(), cellCounts);

  viskores::Id totalCells = 0;
  auto cellCountsPortal = cellCounts.ReadPortal();
  for (viskores::Id i = 0; i < cellCounts.GetNumberOfValues(); ++i)
    totalCells += cellCountsPortal.Get(i);

  viskores::cont::ArrayHandle<viskores::Id> allCellIds;
  allCellIds.AllocateAndFill(totalCells, viskores::Id{ -1 });
  auto offsets = viskores::cont::ConvertNumComponentsToOffsets(cellCounts);
  auto allCellIdsVec = viskores::cont::make_ArrayHandleGroupVecVariable(allCellIds, offsets);
  invoker(FindBoundsMapCellIdsWorklet{}, pointsAH, boundsMap.GetLocator(), allCellIdsVec);

  std::vector<LocatedCellIds> cellIds;
  cellIds.reserve(points.size());
  auto cellIdsPortal = allCellIdsVec.ReadPortal();
  for (viskores::Id i = 0; i < static_cast<viskores::Id>(points.size()); ++i)
  {
    LocatedCellIds result;
    result.Count = static_cast<viskores::IdComponent>(cellCountsPortal.Get(i));

    auto idsForPoint = cellIdsPortal.Get(i);
    for (viskores::IdComponent j = 0; j < idsForPoint.GetNumberOfComponents(); ++j)
    {
      const viskores::Id id = idsForPoint[j];
      if (id >= 0)
        result.Ids.emplace_back(id);
    }
    std::sort(result.Ids.begin(), result.Ids.end());
    cellIds.emplace_back(std::move(result));
  }

  return cellIds;
}

void ValidateCellIds(const LocatedCellIds& actual,
                     std::initializer_list<viskores::Id> expected,
                     const std::string& message)
{
  std::vector<viskores::Id> expectedVec(expected);
  std::sort(expectedVec.begin(), expectedVec.end());

  VISKORES_TEST_ASSERT(actual.Count == static_cast<viskores::IdComponent>(expectedVec.size()),
                       message + ": CountAllCells returned wrong number of ids");
  VISKORES_TEST_ASSERT(actual.Ids.size() == expectedVec.size(),
                       message + ": FindAllCellIds returned wrong number of ids");
  for (std::size_t i = 0; i < expectedVec.size(); ++i)
    VISKORES_TEST_ASSERT(actual.Ids[i] == expectedVec[i], message + ": wrong id");
}

void TestBoundsMapLocatorFindsBlockIds()
{
  // BoundsMap should return every block containing a point, including
  // overlapping blocks and blocks that share a boundary.
  const std::vector<viskores::Bounds> bounds = { viskores::Bounds(0, 4, 0, 4, 0, 4),
                                                 viskores::Bounds(4, 8, 0, 4, 0, 4),
                                                 viskores::Bounds(2, 6, 0, 4, 0, 4) };

  viskores::filter::flow::internal::BoundsMap boundsMap(CreatePartitionedDataSet(bounds));
  auto cellIds = FindBoundsMapCellIds(boundsMap,
                                      { viskores::Vec3f(1.0f, 2.0f, 2.0f),
                                        viskores::Vec3f(3.0f, 2.0f, 2.0f),
                                        viskores::Vec3f(4.0f, 2.0f, 2.0f),
                                        viskores::Vec3f(7.0f, 2.0f, 2.0f),
                                        viskores::Vec3f(9.0f, 2.0f, 2.0f) });

  ValidateCellIds(cellIds[0], { 0 }, "BoundsMap locator failed for first block");
  ValidateCellIds(cellIds[1], { 0, 2 }, "BoundsMap locator failed for overlapping blocks");
  ValidateCellIds(cellIds[2], { 0, 1, 2 }, "BoundsMap locator failed on shared boundary");
  ValidateCellIds(cellIds[3], { 1 }, "BoundsMap locator failed for second block");
  ValidateCellIds(cellIds[4], {}, "BoundsMap locator failed outside all blocks");
}

void TestBoundsMapLocatorHandlesDegenerateBounds()
{
  // Lower-dimensional block bounds should still produce valid locator cells so
  // 2D/1D datasets can participate in id-only block lookup. This covers each
  // degenerate shape branch in BoundsMap::BuildLocator.
  const std::vector<viskores::Bounds> bounds = {
    viskores::Bounds(0, 2, 0, 2, 0, 0),   // XY quad
    viskores::Bounds(4, 6, 0, 0, 0, 2),   // XZ quad
    viskores::Bounds(8, 8, 0, 2, 0, 2),   // YZ quad
    viskores::Bounds(12, 14, 0, 0, 0, 0), // X line
    viskores::Bounds(16, 16, 0, 2, 0, 0), // Y line
    viskores::Bounds(20, 20, 0, 0, 0, 2), // Z line
    viskores::Bounds(24, 24, 0, 0, 0, 0)  // vertex
  };

  viskores::filter::flow::internal::BoundsMap boundsMap(CreatePartitionedDataSet(bounds));
  VISKORES_TEST_ASSERT(boundsMap.GetLocator().GetCellSet().GetNumberOfCells() ==
                         static_cast<viskores::Id>(bounds.size()),
                       "BoundsMap locator has the wrong number of cells.");

  auto cellIds = FindBoundsMapCellIds(boundsMap,
                                      { viskores::Vec3f(1.0f, 1.0f, 0.0f),
                                        viskores::Vec3f(5.0f, 0.0f, 1.0f),
                                        viskores::Vec3f(8.0f, 1.0f, 1.0f),
                                        viskores::Vec3f(13.0f, 0.0f, 0.0f),
                                        viskores::Vec3f(16.0f, 1.0f, 0.0f),
                                        viskores::Vec3f(20.0f, 0.0f, 1.0f) });

  ValidateCellIds(cellIds[0], { 0 }, "BoundsMap locator failed for XY bounds");
  ValidateCellIds(cellIds[1], { 1 }, "BoundsMap locator failed for XZ bounds");
  ValidateCellIds(cellIds[2], { 2 }, "BoundsMap locator failed for YZ bounds");
  ValidateCellIds(cellIds[3], { 3 }, "BoundsMap locator failed for X line bounds");
  ValidateCellIds(cellIds[4], { 4 }, "BoundsMap locator failed for Y line bounds");
  ValidateCellIds(cellIds[5], { 5 }, "BoundsMap locator failed for Z line bounds");
}

void TestBoundsMapBlockIdValidation()
{
  // Locator cell ids are used directly as block ids, so sparse block ids would
  // require an explicit remapping layer and are rejected for this foundation MR.
  const std::vector<viskores::Bounds> bounds = { viskores::Bounds(0, 4, 0, 4, 0, 4),
                                                 viskores::Bounds(4, 8, 0, 4, 0, 4) };
  auto pds = CreatePartitionedDataSet(bounds);

  viskores::filter::flow::internal::BoundsMap denseBoundsMap(pds, { 0, 1 });
  VISKORES_TEST_ASSERT(denseBoundsMap.GetTotalNumBlocks() == 2, "Dense block ids failed.");
  const auto& denseRank0 = denseBoundsMap.FindRank(0);
  VISKORES_TEST_ASSERT(denseRank0.size() == std::size_t{ 1 } && denseRank0[0] == 0,
                       "Dense block id 0 should be owned by rank 0.");
  const auto& unknownRank = denseBoundsMap.FindRank(2);
  VISKORES_TEST_ASSERT(unknownRank.empty(), "Unknown block id should have no rank.");

  // Reordered dense ids should preserve the global block id returned by
  // FindAllCellIds rather than exposing local partition order.
  viskores::filter::flow::internal::BoundsMap reorderedBoundsMap(pds, { 1, 0 });
  auto reorderedCellIds = FindBoundsMapCellIds(
    reorderedBoundsMap, { viskores::Vec3f(2.0f, 2.0f, 2.0f), viskores::Vec3f(6.0f, 2.0f, 2.0f) });
  ValidateCellIds(reorderedCellIds[0], { 1 }, "Reordered block ids failed for first partition");
  ValidateCellIds(reorderedCellIds[1], { 0 }, "Reordered block ids failed for second partition");
  const auto& reorderedRank1 = reorderedBoundsMap.FindRank(1);
  VISKORES_TEST_ASSERT(reorderedRank1.size() == std::size_t{ 1 } && reorderedRank1[0] == 0,
                       "Reordered block id should map to rank 0.");

  bool threw = false;
  try
  {
    viskores::filter::flow::internal::BoundsMap sparseBoundsMap(pds, { 0, 2 });
  }
  catch (const viskores::cont::ErrorFilterExecution&)
  {
    threw = true;
  }
  VISKORES_TEST_ASSERT(threw, "Sparse block ids should throw.");
}

void TestBoundsMap()
{
  TestBoundsMapLocatorFindsBlockIds();
  TestBoundsMapLocatorHandlesDegenerateBounds();
  TestBoundsMapBlockIdValidation();
}

} // namespace

int UnitTestBoundsMap(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestBoundsMap, argc, argv);
}
