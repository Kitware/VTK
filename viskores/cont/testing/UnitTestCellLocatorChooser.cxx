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
#include <viskores/cont/CellLocatorChooser.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DataSetBuilderRectilinear.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/exec/CellInterpolate.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/ScatterPermutation.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <ctime>
#include <random>

namespace
{

std::default_random_engine RandomGenerator;

using PointType = viskores::Vec3f;

//-----------------------------------------------------------------------------
viskores::cont::DataSet MakeTestDataSetUniform()
{
  return viskores::cont::DataSetBuilderUniform::Create(
    viskores::Id3{ 32 }, PointType{ -32.0f }, PointType{ 1.0f / 64.0f });
}

viskores::cont::DataSet MakeTestDataSetRectilinear()
{
  std::uniform_real_distribution<viskores::FloatDefault> coordGen(1.0f / 128.0f, 1.0f / 32.0f);

  viskores::cont::ArrayHandle<viskores::FloatDefault> coords[3];
  for (int i = 0; i < 3; ++i)
  {
    coords[i].Allocate(16);
    auto portal = coords[i].WritePortal();

    viskores::FloatDefault cur = 0.0f;
    for (viskores::Id j = 0; j < portal.GetNumberOfValues(); ++j)
    {
      cur += coordGen(RandomGenerator);
      portal.Set(j, cur);
    }
  }

  return viskores::cont::DataSetBuilderRectilinear::Create(coords[0], coords[1], coords[2]);
}

viskores::cont::DataSet MakeTestDataSetCurvilinear()
{
  auto recti = MakeTestDataSetRectilinear();
  auto coords = recti.GetCoordinateSystem().GetDataAsMultiplexer();

  viskores::cont::ArrayHandle<PointType> sheared;
  sheared.Allocate(coords.GetNumberOfValues());

  auto inPortal = coords.ReadPortal();
  auto outPortal = sheared.WritePortal();
  for (viskores::Id i = 0; i < inPortal.GetNumberOfValues(); ++i)
  {
    auto val = inPortal.Get(i);
    outPortal.Set(i, val + viskores::make_Vec(val[1], val[2], val[0]));
  }

  viskores::cont::DataSet curvi;
  curvi.SetCellSet(recti.GetCellSet());
  curvi.AddCoordinateSystem(viskores::cont::CoordinateSystem("coords", sheared));

  return curvi;
}

//-----------------------------------------------------------------------------
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
    auto status = viskores::exec::CellInterpolate(points, pc, cellShape, wc);
    if (status != viskores::ErrorCode::Success)
    {
      this->RaiseError(viskores::ErrorString(status));
    }
  }
};

void GenerateRandomInput(const viskores::cont::DataSet& ds,
                         viskores::Id count,
                         viskores::cont::ArrayHandle<viskores::Id>& cellIds,
                         viskores::cont::ArrayHandle<PointType>& pcoords,
                         viskores::cont::ArrayHandle<PointType>& wcoords)
{
  viskores::Id numberOfCells = ds.GetNumberOfCells();

  std::uniform_int_distribution<viskores::Id> cellIdGen(0, numberOfCells - 1);
  std::uniform_real_distribution<viskores::FloatDefault> pcoordGen(0.0f, 1.0f);

  cellIds.Allocate(count);
  pcoords.Allocate(count);
  wcoords.Allocate(count);

  auto cwp = cellIds.WritePortal();
  auto pwp = pcoords.WritePortal();
  for (viskores::Id i = 0; i < count; ++i)
  {
    cwp.Set(i, cellIdGen(RandomGenerator));

    PointType pc{ pcoordGen(RandomGenerator),
                  pcoordGen(RandomGenerator),
                  pcoordGen(RandomGenerator) };
    pwp.Set(i, pc);
  }

  viskores::worklet::DispatcherMapTopology<ParametricToWorldCoordinates> dispatcher(
    ParametricToWorldCoordinates::MakeScatter(cellIds));
  dispatcher.Invoke(
    ds.GetCellSet(), ds.GetCoordinateSystem().GetDataAsMultiplexer(), pcoords, wcoords);
}

//-----------------------------------------------------------------------------
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

template <typename CellSetType, typename CoordinateSystemArrayType>
void TestWithDataSet(const viskores::cont::DataSet& dataset)
{
  VISKORES_TEST_ASSERT(dataset.GetCellSet().IsType<CellSetType>());
  VISKORES_TEST_ASSERT(dataset.GetCoordinateSystem().GetData().IsType<CoordinateSystemArrayType>());

  viskores::cont::CellLocatorChooser<CellSetType, CoordinateSystemArrayType> locator;
  locator.SetCellSet(dataset.GetCellSet());
  locator.SetCoordinates(dataset.GetCoordinateSystem());
  locator.Update();

  viskores::cont::ArrayHandle<viskores::Id> expCellIds;
  viskores::cont::ArrayHandle<PointType> expPCoords;
  viskores::cont::ArrayHandle<PointType> points;
  GenerateRandomInput(dataset, 32, expCellIds, expPCoords, points);

  viskores::cont::ArrayHandle<viskores::Id> cellIds;
  viskores::cont::ArrayHandle<PointType> pcoords;

  viskores::worklet::DispatcherMapField<FindCellWorklet> dispatcher;
  dispatcher.Invoke(points, locator, cellIds, pcoords);

  auto cellIdPortal = cellIds.ReadPortal();
  auto expCellIdsPortal = expCellIds.ReadPortal();
  auto pcoordsPortal = pcoords.ReadPortal();
  auto expPCoordsPortal = expPCoords.ReadPortal();
  for (viskores::Id i = 0; i < 32; ++i)
  {
    VISKORES_TEST_ASSERT(cellIdPortal.Get(i) == expCellIdsPortal.Get(i), "Incorrect cell ids");
    VISKORES_TEST_ASSERT(test_equal(pcoordsPortal.Get(i), expPCoordsPortal.Get(i), 1e-3),
                         "Incorrect parameteric coordinates");
  }
}

void TestCellLocatorChooser()
{
  TestWithDataSet<viskores::cont::CellSetStructured<3>,
                  viskores::cont::ArrayHandleUniformPointCoordinates>(MakeTestDataSetUniform());

  TestWithDataSet<viskores::cont::CellSetStructured<3>,
                  viskores::cont::ArrayHandleCartesianProduct<
                    viskores::cont::ArrayHandle<viskores::FloatDefault>,
                    viskores::cont::ArrayHandle<viskores::FloatDefault>,
                    viskores::cont::ArrayHandle<viskores::FloatDefault>>>(
    MakeTestDataSetRectilinear());

  TestWithDataSet<viskores::cont::CellSetStructured<3>, viskores::cont::ArrayHandle<PointType>>(
    MakeTestDataSetCurvilinear());
}

} // anonymous namespace

int UnitTestCellLocatorChooser(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestCellLocatorChooser, argc, argv);
}
