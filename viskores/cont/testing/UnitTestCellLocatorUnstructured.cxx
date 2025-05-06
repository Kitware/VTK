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

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/CellLocatorTwoLevel.h>
#include <viskores/cont/CellLocatorUniformBins.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/exec/ParametricCoordinates.h>

#include <viskores/filter/geometry_refinement/worklet/Tetrahedralize.h>
#include <viskores/filter/geometry_refinement/worklet/Triangulate.h>
#include <viskores/worklet/ScatterPermutation.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/CellShape.h>

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
    viskores::ErrorCode status = locator.FindCell(point, cellId, pcoords, lastCell);
    if (status != viskores::ErrorCode::Success)
      this->RaiseError(viskores::ErrorString(status));
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
                     viskores::Id numberOfPoints)
{
  auto ds = MakeTestDataSet(dim);

  std::cout << "Testing " << DIMENSIONS << "D dataset with " << ds.GetNumberOfCells() << " cells\n";

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

  //Call it again using the lastCell2 just computed to validate.
  TestLastCell(locator, numberOfPoints, lastCell2, points, expCellIds, pcoords);
}

void TestingCellLocatorUnstructured()
{
  viskores::UInt32 seed = static_cast<viskores::UInt32>(std::time(nullptr));
  std::cout << "Seed: " << seed << std::endl;
  RandomGenerator.seed(seed);

  //Test viskores::cont::CellLocatorTwoLevel
  viskores::cont::CellLocatorTwoLevel locator2L;
  locator2L.SetDensityL1(64.0f);
  locator2L.SetDensityL2(1.0f);

  TestCellLocator(locator2L, viskores::Id3(8), 512);  // 3D dataset
  TestCellLocator(locator2L, viskores::Id2(18), 512); // 2D dataset

  //Test viskores::cont::CellLocatorUniformBins
  viskores::cont::CellLocatorUniformBins locatorUB;
  locatorUB.SetDims({ 32, 32, 32 });
  TestCellLocator(locatorUB, viskores::Id3(8), 512);  // 3D dataset
  TestCellLocator(locatorUB, viskores::Id2(18), 512); // 2D dataset

  //Test 2D dataset with 2D bins.
  locatorUB.SetDims({ 32, 32, 1 });
  TestCellLocator(locatorUB, viskores::Id2(18), 512); // 2D dataset
}


} // anonymous

int UnitTestCellLocatorUnstructured(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestingCellLocatorUnstructured, argc, argv);
}
