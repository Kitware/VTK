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

#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/filter/field_conversion/worklet/CellAverage.h>
#include <viskores/filter/field_conversion/worklet/PointAverage.h>

#include <viskores/Math.h>

#include <viskores/cont/DataSet.h>
#include <viskores/cont/DeviceAdapterTag.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace test_uniform
{

class MaxPointOrCellValue : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(FieldInCell inCells,
                                FieldInPoint inPoints,
                                CellSetIn topology,
                                FieldOutCell outCells);
  using ExecutionSignature = void(_1, _4, _2, PointCount, CellShape, PointIndices);
  using InputDomain = _3;

  VISKORES_CONT
  MaxPointOrCellValue() {}

  template <typename InCellType,
            typename OutCellType,
            typename InPointVecType,
            typename CellShapeTag,
            typename PointIndexType>
  VISKORES_EXEC void operator()(const InCellType& cellValue,
                                OutCellType& maxValue,
                                const InPointVecType& pointValues,
                                const viskores::IdComponent& numPoints,
                                const CellShapeTag& viskoresNotUsed(type),
                                const PointIndexType& viskoresNotUsed(pointIDs)) const
  {
    //simple functor that returns the max of cellValue and pointValue
    maxValue = static_cast<OutCellType>(cellValue);
    for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; ++pointIndex)
    {
      maxValue = viskores::Max(maxValue, static_cast<OutCellType>(pointValues[pointIndex]));
    }
  }
};

struct CheckStructuredUniformPointCoords : public viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn topology, FieldInPoint pointCoords);
  using ExecutionSignature = void(_2);

  VISKORES_CONT
  CheckStructuredUniformPointCoords() {}

  template <viskores::IdComponent NumDimensions>
  VISKORES_EXEC void operator()(
    const viskores::VecAxisAlignedPointCoordinates<NumDimensions>& viskoresNotUsed(coords)) const
  {
    // Success if here.
  }

  template <typename PointCoordsVecType>
  VISKORES_EXEC void operator()(const PointCoordsVecType& viskoresNotUsed(coords)) const
  {
    this->RaiseError("Got wrong point coordinates type.");
  }
};
}

namespace
{

static void TestMaxPointOrCell();
static void TestAvgPointToCell();
static void TestAvgCellToPoint();
static void TestStructuredUniformPointCoords();

void TestWorkletMapTopologyUniform(viskores::cont::DeviceAdapterId id)
{
  std::cout << "Testing Topology Worklet ( Uniform ) on device adapter: " << id.GetName()
            << std::endl;

  TestMaxPointOrCell();
  TestAvgPointToCell();
  TestAvgCellToPoint();
  TestStructuredUniformPointCoords();
}

static void TestMaxPointOrCell()
{
  std::cout << "Testing MaxPointOfCell worklet" << std::endl;
  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  viskores::cont::ArrayHandle<viskores::Float32> result;

  viskores::worklet::DispatcherMapTopology<::test_uniform::MaxPointOrCellValue> dispatcher;
  dispatcher.Invoke(
    dataSet.GetField("cellvar")
      .GetData()
      .ResetTypes<viskores::TypeListFieldScalar, VISKORES_DEFAULT_STORAGE_LIST>(),
    dataSet.GetField("pointvar")
      .GetData()
      .ResetTypes<viskores::TypeListFieldScalar, VISKORES_DEFAULT_STORAGE_LIST>(),
    // We know that the cell set is a structured 2D grid and
    // The worklet does not work with general types because
    // of the way we get cell indices. We need to make that
    // part more flexible.
    dataSet.GetCellSet().ResetCellSetList(viskores::cont::CellSetListStructured2D()),
    result);

  std::cout << "Make sure we got the right answer." << std::endl;
  VISKORES_TEST_ASSERT(test_equal(result.ReadPortal().Get(0), 100.1f),
                       "Wrong result for MaxPointOrCell worklet");
  VISKORES_TEST_ASSERT(test_equal(result.ReadPortal().Get(1), 200.1f),
                       "Wrong result for MaxPointOrCell worklet");
}

static void TestAvgPointToCell()
{
  std::cout << "Testing AvgPointToCell worklet" << std::endl;
  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  viskores::cont::ArrayHandle<viskores::Float32> result;

  auto cellset = dataSet.GetCellSet().ResetCellSetList(viskores::cont::CellSetListStructured2D());

  viskores::worklet::DispatcherMapTopology<viskores::worklet::CellAverage> dispatcher;
  dispatcher.Invoke(
    // We know that the cell set is a structured 2D grid and
    // The worklet does not work with general types because
    // of the way we get cell indices. We need to make that
    // part more flexible.
    &cellset,
    dataSet.GetField("pointvar")
      .GetData()
      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>(),
    result);

  std::cout << "Make sure we got the right answer." << std::endl;
  VISKORES_TEST_ASSERT(test_equal(result.ReadPortal().Get(0), 30.1f),
                       "Wrong result for PointToCellAverage worklet");
  VISKORES_TEST_ASSERT(test_equal(result.ReadPortal().Get(1), 40.1f),
                       "Wrong result for PointToCellAverage worklet");

  std::cout << "Try to invoke with an input array of the wrong size." << std::endl;
  bool exceptionThrown = false;
  try
  {
    dispatcher.Invoke( // We know that the cell set is a structured 2D grid and
      // The worklet does not work with general types because
      // of the way we get cell indices. We need to make that
      // part more flexible.
      dataSet.GetCellSet().ResetCellSetList(viskores::cont::CellSetListStructured2D()),
      dataSet.GetField("cellvar")
        .GetData()
        .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>(), // should be pointvar
      result);
  }
  catch (viskores::cont::ErrorBadValue& error)
  {
    std::cout << "  Caught expected error: " << error.GetMessage() << std::endl;
    exceptionThrown = true;
  }
  VISKORES_TEST_ASSERT(exceptionThrown, "Dispatcher did not throw expected exception.");
}

static void TestAvgCellToPoint()
{
  std::cout << "Testing AvgCellToPoint worklet" << std::endl;

  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  viskores::cont::ArrayHandle<viskores::Float32> result;

  viskores::worklet::DispatcherMapTopology<viskores::worklet::PointAverage> dispatcher;
  dispatcher.Invoke(
    // We know that the cell set is a structured 2D grid and
    // The worklet does not work with general types because
    // of the way we get cell indices. We need to make that
    // part more flexible.
    dataSet.GetCellSet().ResetCellSetList(viskores::cont::CellSetListStructured2D()),
    dataSet.GetField("cellvar")
      .GetData()
      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>(),
    result);

  std::cout << "Make sure we got the right answer." << std::endl;
  VISKORES_TEST_ASSERT(test_equal(result.ReadPortal().Get(0), 100.1f),
                       "Wrong result for CellToPointAverage worklet");
  VISKORES_TEST_ASSERT(test_equal(result.ReadPortal().Get(1), 150.1f),
                       "Wrong result for CellToPointAverage worklet");

  std::cout << "Try to invoke with an input array of the wrong size." << std::endl;
  bool exceptionThrown = false;
  try
  {
    dispatcher.Invoke( // We know that the cell set is a structured 2D grid and
      // The worklet does not work with general types because
      // of the way we get cell indices. We need to make that
      // part more flexible.
      dataSet.GetCellSet().ResetCellSetList(viskores::cont::CellSetListStructured2D()),
      dataSet.GetField("pointvar")
        .GetData()
        .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>(), // should be cellvar
      result);
  }
  catch (viskores::cont::ErrorBadValue& error)
  {
    std::cout << "  Caught expected error: " << error.GetMessage() << std::endl;
    exceptionThrown = true;
  }
  VISKORES_TEST_ASSERT(exceptionThrown, "Dispatcher did not throw expected exception.");
}

static void TestStructuredUniformPointCoords()
{
  std::cout << "Testing uniform point coordinates in structured grids" << std::endl;

  viskores::cont::testing::MakeTestDataSet testDataSet;

  viskores::worklet::DispatcherMapTopology<::test_uniform::CheckStructuredUniformPointCoords>
    dispatcher;

  viskores::cont::DataSet dataSet3D = testDataSet.Make3DUniformDataSet0();
  dispatcher.Invoke(dataSet3D.GetCellSet(), dataSet3D.GetCoordinateSystem());

  viskores::cont::DataSet dataSet2D = testDataSet.Make2DUniformDataSet0();
  dispatcher.Invoke(dataSet2D.GetCellSet(), dataSet2D.GetCoordinateSystem());
}

} // anonymous namespace

int UnitTestWorkletMapTopologyUniform(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::RunOnDevice(TestWorkletMapTopologyUniform, argc, argv);
}
