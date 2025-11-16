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

#include <viskores/Bounds.h>
#include <viskores/CellShape.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/CellLocatorBoundingIntervalHierarchy.h>
#include <viskores/cont/CellLocatorTwoLevel.h>
#include <viskores/cont/CellLocatorUniformBins.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/exec/ParametricCoordinates.h>
#include <viskores/filter/geometry_refinement/worklet/Tetrahedralize.h>
#include <viskores/filter/geometry_refinement/worklet/Triangulate.h>
#include <viskores/worklet/ScatterPermutation.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <ctime>
#include <random>

namespace
{

using PointType = viskores::Vec3f;

std::default_random_engine RandomGenerator;

class ParametricToWorldCoordinates : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInPoint coords,
                                FieldInOutCell pcs,
                                FieldOutCell wcs);
  using ExecutionSignature = void(CellShape, _2, _3, _4);

  using ScatterType = viskores::worklet::ScatterPermutation<>;

  VISKORES_CONT
  static ScatterType MakeScatter(const viskores::cont::ArrayHandle<viskores::Id>& cellIds)
  {
    return ScatterType(cellIds);
  }

  template <typename CellShapeTagType, typename PointsVecType>
  VISKORES_EXEC void operator()(CellShapeTagType cellShape,
                                PointsVecType points,
                                const PointType& pc,
                                PointType& wc) const
  {
    auto status =
      viskores::exec::ParametricCoordinatesToWorldCoordinates(points, pc, cellShape, wc);
    if (status != viskores::ErrorCode::Success)
    {
      this->RaiseError(viskores::ErrorString(status));
    }
  }
};

template <viskores::IdComponent DIMENSIONS>
viskores::cont::DataSet MakeTestDataSet(const viskores::Vec<viskores::Id, DIMENSIONS>& dims)
{
  auto uniformDs = viskores::cont::DataSetBuilderUniform::Create(
    dims,
    viskores::Vec<viskores::FloatDefault, DIMENSIONS>(0.0f),
    viskores::Vec<viskores::FloatDefault, DIMENSIONS>(1.0f));

  auto uniformCs =
    uniformDs.GetCellSet().template AsCellSet<viskores::cont::CellSetStructured<DIMENSIONS>>();

  // triangulate the cellset
  viskores::cont::CellSetSingleType<> cellset;
  switch (DIMENSIONS)
  {
    case 2:
      cellset = viskores::worklet::Triangulate().Run(uniformCs);
      break;
    case 3:
      cellset = viskores::worklet::Tetrahedralize().Run(uniformCs);
      break;
    default:
      VISKORES_ASSERT(false);
  }

  // Warp the coordinates
  std::uniform_real_distribution<viskores::FloatDefault> warpFactor(-0.10f, 0.10f);
  auto inPointsPortal =
    uniformDs.GetCoordinateSystem()
      .GetData()
      .template AsArrayHandle<viskores::cont::ArrayHandleUniformPointCoordinates>()
      .ReadPortal();
  viskores::cont::ArrayHandle<PointType> points;
  points.Allocate(inPointsPortal.GetNumberOfValues());
  auto outPointsPortal = points.WritePortal();
  for (viskores::Id i = 0; i < outPointsPortal.GetNumberOfValues(); ++i)
  {
    PointType warpVec(0);
    for (viskores::IdComponent c = 0; c < DIMENSIONS; ++c)
    {
      warpVec[c] = warpFactor(RandomGenerator);
    }
    outPointsPortal.Set(i, inPointsPortal.Get(i) + warpVec);
  }

  // build dataset
  viskores::cont::DataSet out;
  out.AddCoordinateSystem(viskores::cont::CoordinateSystem("coords", points));
  out.SetCellSet(cellset);
  return out;
}

template <viskores::IdComponent DIMENSIONS>
void GenerateRandomInput(const viskores::cont::DataSet& ds,
                         viskores::Id count,
                         viskores::cont::ArrayHandle<viskores::Id>& cellIds,
                         viskores::cont::ArrayHandle<PointType>& pcoords,
                         viskores::cont::ArrayHandle<PointType>& wcoords)
{
  viskores::Id numberOfCells = ds.GetNumberOfCells();

  std::uniform_int_distribution<viskores::Id> cellIdGen(0, numberOfCells - 1);

  cellIds.Allocate(count);
  pcoords.Allocate(count);
  wcoords.Allocate(count);

  for (viskores::Id i = 0; i < count; ++i)
  {
    cellIds.WritePortal().Set(i, cellIdGen(RandomGenerator));

    PointType pc(0.0f);
    viskores::FloatDefault minPc = 1e-2f;
    viskores::FloatDefault sum = 0.0f;
    for (viskores::IdComponent c = 0; c < DIMENSIONS; ++c)
    {
      viskores::FloatDefault maxPc =
        1.0f - (static_cast<viskores::FloatDefault>(DIMENSIONS - c) * minPc) - sum;
      std::uniform_real_distribution<viskores::FloatDefault> pcoordGen(minPc, maxPc);
      pc[c] = pcoordGen(RandomGenerator);
      sum += pc[c];
    }
    pcoords.WritePortal().Set(i, pc);
  }

  viskores::cont::Invoker invoker;
  invoker(ParametricToWorldCoordinates{},
          ParametricToWorldCoordinates::MakeScatter(cellIds),
          ds.GetCellSet(),
          ds.GetCoordinateSystem().GetDataAsMultiplexer(),
          pcoords,
          wcoords);
}

class FindCellWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn points,
                                ExecObject locator,
                                FieldOut cellIds,
                                FieldOut pcoords);
  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename LocatorType>
  VISKORES_EXEC void operator()(const viskores::Vec3f& point,
                                const LocatorType& locator,
                                viskores::Id& cellId,
                                viskores::Vec3f& pcoords) const
  {
    viskores::ErrorCode status = locator.FindCell(point, cellId, pcoords);
    if (status != viskores::ErrorCode::Success)
    {
      this->RaiseError(viskores::ErrorString(status));
    }
  }
};

class FindCellWorkletWithLastCell : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn points,
                                ExecObject locator,
                                FieldOut cellIds,
                                FieldOut pcoords,
                                FieldInOut lastCell);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);

  template <typename LocatorType>
  VISKORES_EXEC void operator()(const viskores::Vec3f& point,
                                const LocatorType& locator,
                                viskores::Id& cellId,
                                viskores::Vec3f& pcoords,
                                typename LocatorType::LastCell& lastCell) const
  {
    viskores::ErrorCode status;
    status = locator.FindCell(point, cellId, pcoords, lastCell);

    if (status != viskores::ErrorCode::Success)
    {
      this->RaiseError(viskores::ErrorString(status));
    }
  }
};

class CountAllCellsWorklet : public viskores::worklet::WorkletMapField
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

class FindAllCellsWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn points,
                                ExecObject locator,
                                FieldOut cellIds,
                                FieldOut pCoords);
  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename LocatorType, typename CellIdVecType, typename ParametricCoordsVecType>
  VISKORES_EXEC void operator()(const viskores::Vec3f& point,
                                const LocatorType& locator,
                                CellIdVecType& cellIds,
                                ParametricCoordsVecType pCoords) const
  {
    locator.FindAllCells(point, cellIds, pCoords);
  }
};

template <typename LocatorType>
void TestLastCell(LocatorType& locator,
                  viskores::Id numPoints,
                  viskores::cont::ArrayHandle<typename LocatorType::LastCell>& lastCell,
                  const viskores::cont::ArrayHandle<PointType>& points,
                  const viskores::cont::ArrayHandle<viskores::Id>& expCellIds,
                  const viskores::cont::ArrayHandle<PointType>& expPCoords)

{
  viskores::cont::ArrayHandle<viskores::Id> cellIds;
  viskores::cont::ArrayHandle<PointType> pcoords;

  viskores::cont::Invoker invoker;
  invoker(FindCellWorkletWithLastCell{}, points, locator, cellIds, pcoords, lastCell);

  auto cellIdPortal = cellIds.ReadPortal();
  auto expCellIdsPortal = expCellIds.ReadPortal();
  auto pcoordsPortal = pcoords.ReadPortal();
  auto expPCoordsPortal = expPCoords.ReadPortal();

  for (viskores::Id i = 0; i < numPoints; ++i)
  {
    VISKORES_TEST_ASSERT(cellIdPortal.Get(i) == expCellIdsPortal.Get(i), "Incorrect cell ids");
    VISKORES_TEST_ASSERT(test_equal(pcoordsPortal.Get(i), expPCoordsPortal.Get(i), 1e-3),
                         "Incorrect parameteric coordinates");
  }
}

template <typename LocatorType, viskores::IdComponent DIMENSIONS>
void TestCellLocator(LocatorType& locator,
                     const viskores::Vec<viskores::Id, DIMENSIONS>& dim,
                     viskores::Id numberOfPoints,
                     bool testFindAllCells = true)
{
  auto ds = MakeTestDataSet(dim);

  std::cout << "TestCellLocator: " << DIMENSIONS << "D dataset with " << ds.GetNumberOfCells()
            << " cells\n";

  locator.SetCellSet(ds.GetCellSet());
  locator.SetCoordinates(ds.GetCoordinateSystem());
  locator.Update();

  viskores::cont::ArrayHandle<viskores::Id> expCellIds;
  viskores::cont::ArrayHandle<PointType> expPCoords;
  viskores::cont::ArrayHandle<PointType> points;
  GenerateRandomInput<DIMENSIONS>(ds, numberOfPoints, expCellIds, expPCoords, points);

  std::cout << "Finding cells for " << numberOfPoints << " points\n";
  viskores::cont::ArrayHandle<viskores::Id> cellIds;
  viskores::cont::ArrayHandle<PointType> pcoords;

  viskores::cont::Invoker invoker;
  invoker(FindCellWorklet{}, points, locator, cellIds, pcoords);

  auto cellIdsPortal = cellIds.ReadPortal();
  auto expCellIdsPortal = expCellIds.ReadPortal();
  auto pcoordsPortal = pcoords.ReadPortal();
  auto expPCoordsPortal = expPCoords.ReadPortal();
  for (viskores::Id i = 0; i < numberOfPoints; ++i)
  {
    VISKORES_TEST_ASSERT(cellIdsPortal.Get(i) == expCellIdsPortal.Get(i), "Incorrect cell ids");
    VISKORES_TEST_ASSERT(test_equal(pcoordsPortal.Get(i), expPCoordsPortal.Get(i), 1e-3),
                         "Incorrect parameteric coordinates");
  }

  //Test locator using lastCell
  //Test it with initialized.
  viskores::cont::ArrayHandle<typename LocatorType::LastCell> lastCell;
  lastCell.AllocateAndFill(numberOfPoints, typename LocatorType::LastCell{});
  TestLastCell(locator, numberOfPoints, lastCell, points, expCellIds, pcoords);

  //Call it again using the lastCell just computed to validate.
  TestLastCell(locator, numberOfPoints, lastCell, points, expCellIds, pcoords);

  //Test it with uninitialized array.
  viskores::cont::ArrayHandle<typename LocatorType::LastCell> lastCell2;
  lastCell2.Allocate(numberOfPoints);

  TestLastCell(locator, numberOfPoints, lastCell2, points, expCellIds, pcoords);


  //Test CountAllCells and FindAllCells. Should be identical to the tests above.
  viskores::cont::ArrayHandle<viskores::Id> cellCounts;
  invoker(CountAllCellsWorklet{}, points, locator, cellCounts);

  //Expected to find 1 cell for each point.
  auto portal = cellCounts.ReadPortal();
  for (viskores::Id i = 0; i < numberOfPoints; ++i)
  {
    VISKORES_TEST_ASSERT(portal.Get(i) == 1, "Expected to find 1 cell for each point");
  }

  if (testFindAllCells)
  {
    auto numberOfFoundCells = viskores::cont::Algorithm::Reduce(cellCounts, viskores::Id(0));

    //create an array to hold all of the found cells.
    viskores::cont::ArrayHandle<viskores::Id> allCellIds;
    viskores::cont::ArrayHandle<viskores::Vec3f> pCoords;
    allCellIds.AllocateAndFill(numberOfFoundCells, viskores::Id(-1));
    pCoords.Allocate(numberOfFoundCells);

    viskores::cont::ArrayHandle<viskores::Id> cellOffsets =
      viskores::cont::ConvertNumComponentsToOffsets(cellCounts);
    auto cellIdsVec = viskores::cont::make_ArrayHandleGroupVecVariable(allCellIds, cellOffsets);
    auto pCoordsVec = viskores::cont::make_ArrayHandleGroupVecVariable(pCoords, cellOffsets);

    invoker(FindAllCellsWorklet{}, points, locator, cellIdsVec, pCoordsVec);
    portal = allCellIds.ReadPortal();
    for (viskores::Id i = 0; i < numberOfFoundCells; ++i)
    {
      VISKORES_TEST_ASSERT(portal.Get(i) == expCellIds.ReadPortal().Get(i),
                           "Found cell id out of range");
    }
  }
}

viskores::Vec3f ToVec3f(const viskores::Float64& x,
                        const viskores::Float64& y,
                        const viskores::Float64& z)
{
  return { static_cast<viskores::FloatDefault>(x),
           static_cast<viskores::FloatDefault>(y),
           static_cast<viskores::FloatDefault>(z) };
}

viskores::cont::DataSet CreateDataSetFromBounds(const std::vector<viskores::Bounds>& bounds,
                                                bool is2D = true)
{
  viskores::cont::DataSetBuilderExplicit dsBuilder;
  std::vector<viskores::Vec3f> points;
  std::vector<viskores::Id> connectivity;
  std::vector<viskores::IdComponent> numIndices;
  std::vector<viskores::UInt8> shapes;
  viskores::Id ptId = 0;
  for (const auto& bound : bounds)
  {
    if (is2D)
    {
      // Create a quad in 2D
      points.push_back(ToVec3f(bound.X.Min, bound.Y.Min, bound.Z.Min));
      points.push_back(ToVec3f(bound.X.Max, bound.Y.Min, bound.Z.Min));
      points.push_back(ToVec3f(bound.X.Max, bound.Y.Max, bound.Z.Min));
      points.push_back(ToVec3f(bound.X.Min, bound.Y.Max, bound.Z.Min));

      for (int i = 0; i < 4; i++)
        connectivity.push_back(ptId++);

      numIndices.push_back(4);
      shapes.push_back(viskores::CELL_SHAPE_QUAD);
    }
    else
    {
      // Create a hexahedron in 3D
      points.push_back(ToVec3f(bound.X.Min, bound.Y.Min, bound.Z.Min));
      points.push_back(ToVec3f(bound.X.Max, bound.Y.Min, bound.Z.Min));
      points.push_back(ToVec3f(bound.X.Max, bound.Y.Max, bound.Z.Min));
      points.push_back(ToVec3f(bound.X.Min, bound.Y.Max, bound.Z.Min));

      points.push_back(ToVec3f(bound.X.Min, bound.Y.Min, bound.Z.Max));
      points.push_back(ToVec3f(bound.X.Max, bound.Y.Min, bound.Z.Max));
      points.push_back(ToVec3f(bound.X.Max, bound.Y.Max, bound.Z.Max));
      points.push_back(ToVec3f(bound.X.Min, bound.Y.Max, bound.Z.Max));

      for (int i = 0; i < 8; i++)
        connectivity.push_back(ptId++);
      numIndices.push_back(8);
      shapes.push_back(viskores::CELL_SHAPE_HEXAHEDRON);
    }
  }
  return viskores::cont::DataSetBuilderExplicit::Create(points, shapes, numIndices, connectivity);
}

} // namespace

template <typename LocatorType>
void ValidateFindAllCells(LocatorType& locator,
                          const std::vector<viskores::Vec3f>& testPts,
                          const std::vector<std::vector<viskores::Id>>& expCellIds)
{
  viskores::cont::ArrayHandle<viskores::Id> cellCounts;
  cellCounts.Allocate(testPts.size());
  auto pointsAH =
    viskores::cont::make_ArrayHandle<viskores::Vec3f>(testPts, viskores::CopyFlag::On);

  viskores::cont::Invoker invoker;
  invoker(CountAllCellsWorklet{}, pointsAH, locator, cellCounts);

  auto cellCountsPortal = cellCounts.ReadPortal();
  for (viskores::Id i = 0; i < pointsAH.GetNumberOfValues(); i++)
  {
    VISKORES_TEST_ASSERT(cellCountsPortal.Get(i) == static_cast<viskores::Id>(expCellIds[i].size()),
                         "Incorrect number of cells found for point " + std::to_string(i) + " " +
                           std::to_string(cellCountsPortal.Get(i)));
  }

  viskores::Id numberOfFoundCells = viskores::cont::Algorithm::Reduce(cellCounts, viskores::Id(0));
  viskores::cont::ArrayHandle<viskores::Id> cellIds;
  viskores::cont::ArrayHandle<viskores::Vec3f> pCoords;
  cellIds.AllocateAndFill(numberOfFoundCells, viskores::Id(-1));
  pCoords.Allocate(numberOfFoundCells);

  auto cellIdsVec = viskores::cont::make_ArrayHandleGroupVecVariable(
    cellIds, viskores::cont::ConvertNumComponentsToOffsets(cellCounts));
  auto pCoordsVec = viskores::cont::make_ArrayHandleGroupVecVariable(
    pCoords, viskores::cont::ConvertNumComponentsToOffsets(cellCounts));

  invoker(FindAllCellsWorklet{}, pointsAH, locator, cellIdsVec, pCoordsVec);

  auto portal = cellIdsVec.ReadPortal();
  for (viskores::Id i = 0; i < cellIdsVec.GetNumberOfValues(); i++)
  {
    viskores::IdComponent numComp = portal.Get(i).GetNumberOfComponents();
    VISKORES_TEST_ASSERT(numComp == static_cast<viskores::IdComponent>(expCellIds[i].size()),
                         "Wrong number of components for point " + std::to_string(i));
    //Create a std::vector and sort it to compare values.
    std::vector<viskores::Id> cells0, cells1;
    for (viskores::IdComponent j = 0; j < numComp; j++)
    {
      cells0.push_back(portal.Get(i)[j]);
      cells1.push_back(expCellIds[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)]);
    }

    //Sort them to compare.
    std::sort(cells0.begin(), cells0.end());
    std::sort(cells1.begin(), cells1.end());
    for (std::size_t j = 0; j < cells0.size(); j++)
    {
      VISKORES_TEST_ASSERT(cells0[j] == cells1[j],
                           "Cell Ids do not match at index " + std::to_string(i));
    }
  }
}

template <typename LocatorType>
void TestFindAllCells(LocatorType& locator)
{
  std::cout << "TestFindAllCells" << std::endl;
  std::vector<viskores::Bounds> bounds;

  //2D dataset with overlapping cells.
  bounds.push_back({ { 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f } });
  bounds.push_back({ { 0.9f, 2.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f } });
  bounds.push_back({ { 0.0f, 1.0f }, { 0.9f, 2.0f }, { 0.0f, 0.0f } });
  bounds.push_back({ { 0.9f, 2.0f }, { 0.9f, 2.0f }, { 0.0f, 0.0f } });

  auto ds = CreateDataSetFromBounds(bounds, true);

  locator.SetCellSet(ds.GetCellSet());
  locator.SetCoordinates(ds.GetCoordinateSystem());
  locator.Update();

  //set points.
  std::vector<viskores::Vec3f> testPts;
  std::vector<std::vector<viskores::Id>> expCellIds;

  //pt 1 is only in cell 0.
  testPts.push_back({ 0.25, 0.25f, 0.0f });
  expCellIds.push_back({ 0 });

  //pt 2 is in cell 0 and cell 1.
  testPts.push_back({ 0.95f, 0.25f, 0.0f });
  expCellIds.push_back({ 0, 1 });

  //pt 3 is in cell 1.
  testPts.push_back({ 1.95f, 0.25f, 0.0f });
  expCellIds.push_back({ 1 });

  //pt 4 is in cells 0, 2
  testPts.push_back({ 0.25, .95f, 0.0f });
  expCellIds.push_back({ 0, 2 });

  testPts.push_back({ .25, 1.95f, 0.0f });
  expCellIds.push_back({ 2 });

  testPts.push_back({ .95f, 1.25f, 0.0f });
  expCellIds.push_back({ 2, 3 });

  testPts.push_back({ 1.95f, 1.25f, 0.0f });
  expCellIds.push_back({ 3 });

  //pt in ALL cells.
  testPts.push_back({ .95f, .95f, 0.0f });
  expCellIds.push_back({ 0, 1, 2, 3 });

  ValidateFindAllCells(locator, testPts, expCellIds);

  //3D dataset with overlapping cells.
  bounds.clear();
  bounds.push_back({ { 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f } });
  bounds.push_back({ { 0.9f, 2.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f } });
  bounds.push_back({ { 0.0f, 1.0f }, { 0.9f, 2.0f }, { 0.0f, 1.0f } });
  bounds.push_back({ { 0.9f, 2.0f }, { 0.9f, 2.0f }, { 0.0f, 1.0f } });

  bounds.push_back({ { 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.9f, 2.0f } });
  bounds.push_back({ { 0.9f, 2.0f }, { 0.0f, 1.0f }, { 0.9f, 2.0f } });
  bounds.push_back({ { 0.0f, 1.0f }, { 0.9f, 2.0f }, { 0.9f, 2.0f } });
  bounds.push_back({ { 0.9f, 2.0f }, { 0.9f, 2.0f }, { 0.9f, 2.0f } });
  ds = CreateDataSetFromBounds(bounds, false);

  locator.SetCellSet(ds.GetCellSet());
  locator.SetCoordinates(ds.GetCoordinateSystem());
  locator.Update();

  testPts.clear();
  expCellIds.clear();

  testPts.push_back({ 0.25f, 0.25f, 0.25f });
  expCellIds.push_back({ 0 });

  testPts.push_back({ 0.25f, 0.25f, 0.95f });
  expCellIds.push_back({ 0, 4 });

  testPts.push_back({ 0.25f, 0.25f, 1.25f });
  expCellIds.push_back({ 4 });

  testPts.push_back({ 0.95f, 0.25f, 0.25f });
  expCellIds.push_back({ 0, 1 });

  testPts.push_back({ 1.25f, 0.25f, 0.25f });
  expCellIds.push_back({ 1 });

  testPts.push_back({ 1.25f, 0.25f, 0.95f });
  expCellIds.push_back({ 1, 5 });

  testPts.push_back({ 1.25f, 0.25f, 0.95f });
  expCellIds.push_back({ 1, 5 });

  testPts.push_back({ 1.25f, 0.25f, 1.25f });
  expCellIds.push_back({ 5 });

  testPts.push_back({ 1.25f, 1.25f, 1.25f });
  expCellIds.push_back({ 7 });

  testPts.push_back({ 0.95f, 0.95f, 0.95f });
  expCellIds.push_back({ 0, 1, 2, 3, 4, 5, 6, 7 });

  testPts.push_back({ 0.95f, 1.25f, 0.95f });
  expCellIds.push_back({ 2, 3, 6, 7 });

  testPts.push_back({ 1.25f, 0.95f, 0.95f });
  expCellIds.push_back({ 1, 3, 5, 7 });

  ValidateFindAllCells(locator, testPts, expCellIds);
}

void TestingCellLocatorUnstructured()
{
  viskores::UInt32 seed = static_cast<viskores::UInt32>(std::time(nullptr));
  //seed = 0;
  RandomGenerator.seed(seed);

  //Test viskores::cont::CellLocatorTwoLevel
  viskores::cont::CellLocatorTwoLevel locator2L;
  locator2L.SetDensityL1(64.0f);
  locator2L.SetDensityL2(1.0f);

  std::cout << "Testing CellLocatorTwoLevel" << std::endl;
  TestCellLocator(locator2L, viskores::Id3(8), 512);  // 3D dataset
  TestCellLocator(locator2L, viskores::Id2(18), 512); // 2D dataset
  TestFindAllCells(locator2L);

  viskores::cont::CellLocatorBoundingIntervalHierarchy locatorBIH;
  std::cout << "Testing CellLocatorBoundingIntervalHierarchy" << std::endl;
  TestCellLocator(locatorBIH, viskores::Id3(8), 512, false);  // 3D dataset
  TestCellLocator(locatorBIH, viskores::Id2(18), 512, false); // 2D dataset

  //Test viskores::cont::CellLocatorUniformBins
  viskores::cont::CellLocatorUniformBins locatorUB;
  locatorUB.SetDims({ 32, 32, 32 });
  std::cout << "Testing CellLocatorUniformBins" << std::endl;

  TestCellLocator(locatorUB, viskores::Id3(8), 512);  // 3D dataset
  TestCellLocator(locatorUB, viskores::Id2(18), 512); // 2D dataset

  //Test 2D dataset with 2D bins.
  locatorUB.SetDims({ 32, 32, 1 });
  std::cout << "Testing CellLocatorUniformBins" << std::endl;
  TestCellLocator(locatorUB, viskores::Id2(18), 512); // 2D dataset

  //Test finding all cells.
  locatorUB = viskores::cont::CellLocatorUniformBins();
  locatorUB.SetDims({ 32, 32, 32 });
  TestFindAllCells(locatorUB);
}

int UnitTestCellLocatorUnstructured(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestingCellLocatorUnstructured, argc, argv);
}
