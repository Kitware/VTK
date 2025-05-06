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

#include <random>
#include <string>

#include <viskores/cont/CellLocatorUniformGrid.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Invoker.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/exec/CellLocatorUniformGrid.h>

#include <viskores/worklet/WorkletMapField.h>

namespace
{

class LocatorWorklet : public viskores::worklet::WorkletMapField
{
public:
  LocatorWorklet(viskores::Bounds& bounds, viskores::Id3& cellDims)
    : Bounds(bounds)
    , CellDims(cellDims)
  {
  }

  using ControlSignature =
    void(FieldIn pointIn, ExecObject locator, FieldOut cellId, FieldOut parametric, FieldOut match);

  using ExecutionSignature = void(_1, _2, _3, _4, _5);

  template <typename PointType>
  VISKORES_EXEC viskores::Id CalculateCellId(const PointType& point) const
  {
    if (!Bounds.Contains(point))
      return -1;

    viskores::Id3 logical;
    logical[0] = (point[0] == Bounds.X.Max)
      ? CellDims[0] - 1
      : static_cast<viskores::Id>(viskores::Floor(
          (point[0] / Bounds.X.Length()) * static_cast<viskores::FloatDefault>(CellDims[0])));
    logical[1] = (point[1] == Bounds.Y.Max)
      ? CellDims[1] - 1
      : static_cast<viskores::Id>(viskores::Floor(
          (point[1] / Bounds.Y.Length()) * static_cast<viskores::FloatDefault>(CellDims[1])));
    logical[2] = (point[2] == Bounds.Z.Max)
      ? CellDims[2] - 1
      : static_cast<viskores::Id>(viskores::Floor(
          (point[2] / Bounds.Z.Length()) * static_cast<viskores::FloatDefault>(CellDims[2])));

    return logical[2] * CellDims[0] * CellDims[1] + logical[1] * CellDims[0] + logical[0];
  }

  template <typename PointType, typename LocatorType>
  VISKORES_EXEC void operator()(const PointType& pointIn,
                                const LocatorType& locator,
                                viskores::Id& cellId,
                                PointType& parametric,
                                bool& match) const
  {
    viskores::Id calculated = CalculateCellId(pointIn);
    viskores::ErrorCode status = locator.FindCell(pointIn, cellId, parametric);
    if ((status != viskores::ErrorCode::Success) && (status != viskores::ErrorCode::CellNotFound))
    {
      this->RaiseError(viskores::ErrorString(status));
      match = false;
      return;
    }
    match = (calculated == cellId);
  }

private:
  viskores::Bounds Bounds;
  viskores::Id3 CellDims;
};

void TestTest()
{
  viskores::cont::Invoker invoke;

  viskores::cont::DataSet dataset =
    viskores::cont::testing::MakeTestDataSet().Make3DUniformDataSet1();
  viskores::cont::CoordinateSystem coords = dataset.GetCoordinateSystem();
  viskores::cont::UnknownCellSet cellSet = dataset.GetCellSet();

  viskores::Bounds bounds = coords.GetBounds();
  std::cout << "X bounds : " << bounds.X.Min << " to " << bounds.X.Max << std::endl;
  std::cout << "Y bounds : " << bounds.Y.Min << " to " << bounds.Y.Max << std::endl;
  std::cout << "Z bounds : " << bounds.Z.Min << " to " << bounds.Z.Max << std::endl;

  using StructuredType = viskores::cont::CellSetStructured<3>;
  viskores::Id3 cellDims =
    cellSet.AsCellSet<StructuredType>().GetSchedulingRange(viskores::TopologyElementTagCell());
  std::cout << "Dimensions of dataset : " << cellDims << std::endl;

  viskores::cont::CellLocatorUniformGrid locator;
  locator.SetCoordinates(coords);
  locator.SetCellSet(cellSet);

  locator.Update();

  // Generate some sample points.
  using PointType = viskores::Vec3f;
  std::vector<PointType> pointsVec;
  std::default_random_engine dre;
  std::uniform_real_distribution<viskores::Float32> inBounds(0.0f, 4.0f);
  for (size_t i = 0; i < 10; i++)
  {
    PointType point = viskores::make_Vec(inBounds(dre), inBounds(dre), inBounds(dre));
    pointsVec.push_back(point);
  }
  std::uniform_real_distribution<viskores::Float32> outBounds(4.0f, 5.0f);
  for (size_t i = 0; i < 5; i++)
  {
    PointType point = viskores::make_Vec(outBounds(dre), outBounds(dre), outBounds(dre));
    pointsVec.push_back(point);
  }
  std::uniform_real_distribution<viskores::Float32> outBounds2(-1.0f, 0.0f);
  for (size_t i = 0; i < 5; i++)
  {
    PointType point = viskores::make_Vec(outBounds2(dre), outBounds2(dre), outBounds2(dre));
    pointsVec.push_back(point);
  }

  // Add points right on the boundary.
  pointsVec.push_back(viskores::make_Vec(0, 0, 0));
  pointsVec.push_back(viskores::make_Vec(4, 4, 4));
  pointsVec.push_back(viskores::make_Vec(4, 0, 0));
  pointsVec.push_back(viskores::make_Vec(0, 4, 0));
  pointsVec.push_back(viskores::make_Vec(0, 0, 4));
  pointsVec.push_back(viskores::make_Vec(4, 4, 0));
  pointsVec.push_back(viskores::make_Vec(0, 4, 4));
  pointsVec.push_back(viskores::make_Vec(4, 0, 4));

  viskores::cont::ArrayHandle<PointType> points =
    viskores::cont::make_ArrayHandle(pointsVec, viskores::CopyFlag::Off);
  // Query the points using the locators.
  viskores::cont::ArrayHandle<viskores::Id> cellIds;
  viskores::cont::ArrayHandle<PointType> parametric;
  viskores::cont::ArrayHandle<bool> match;
  LocatorWorklet worklet(bounds, cellDims);
  invoke(worklet, points, locator, cellIds, parametric, match);

  auto matchPortal = match.ReadPortal();
  for (viskores::Id index = 0; index < match.GetNumberOfValues(); index++)
  {
    VISKORES_TEST_ASSERT(matchPortal.Get(index), "Points do not match");
  }
  std::cout << "Test finished successfully." << std::endl;
}

} // anonymous namespace

int UnitTestCellLocatorUniformGrid(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestTest, argc, argv);
}
