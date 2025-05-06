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
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/filter/field_conversion/worklet/CellAverage.h>
#include <viskores/worklet/DispatcherMapTopology.h>

namespace
{

//simple functor that returns the average point value as a cell field
class CellAverage : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset, FieldInPoint inPoints, FieldOutCell outCells);
  using ExecutionSignature = void(PointCount, _2, _3);
  using InputDomain = _1;

  template <typename PointValueVecType, typename OutType>
  VISKORES_EXEC void operator()(const viskores::IdComponent& numPoints,
                                const PointValueVecType& pointValues,
                                OutType& average) const
  {
    average = OutType(pointValues[0]);
    for (viskores::IdComponent pointIndex = 1; pointIndex < numPoints; ++pointIndex)
    {
      average = average + pointValues[pointIndex];
    }

    average = static_cast<OutType>(average * (1.0 / numPoints));
  }
};

template <typename T, typename Storage>
bool TestArrayHandle(const viskores::cont::ArrayHandle<T, Storage>& ah,
                     const T* expected,
                     viskores::Id size)
{
  if (size != ah.GetNumberOfValues())
  {
    return false;
  }

  auto portal = ah.ReadPortal();
  for (viskores::Id i = 0; i < size; ++i)
  {
    if (portal.Get(i) != expected[i])
    {
      return false;
    }
  }

  return true;
}

inline viskores::cont::DataSet make_SingleTypeDataSet()
{
  using CoordType = viskores::Vec3f_32;
  std::vector<CoordType> coordinates;
  coordinates.push_back(CoordType(0, 0, 0));
  coordinates.push_back(CoordType(1, 0, 0));
  coordinates.push_back(CoordType(1, 1, 0));
  coordinates.push_back(CoordType(2, 1, 0));
  coordinates.push_back(CoordType(2, 2, 0));

  std::vector<viskores::Id> conn;
  // First Cell
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(2);
  // Second Cell
  conn.push_back(1);
  conn.push_back(2);
  conn.push_back(3);
  // Third Cell
  conn.push_back(2);
  conn.push_back(3);
  conn.push_back(4);

  viskores::cont::DataSet ds;
  viskores::cont::DataSetBuilderExplicit builder;
  ds = builder.Create(coordinates, viskores::CellShapeTagTriangle(), 3, conn);

  //Set point scalar
  const int nVerts = 5;
  viskores::Float32 vars[nVerts] = { 10.1f, 20.1f, 30.2f, 40.2f, 50.3f };

  ds.AddPointField("pointvar", vars, nVerts);

  return ds;
}

void TestDataSet_SingleType()
{
  viskores::cont::Invoker invoke;

  viskores::cont::DataSet dataSet = make_SingleTypeDataSet();

  //verify that we can get a CellSetSingleType from a dataset
  viskores::cont::CellSetSingleType<> cellset;
  dataSet.GetCellSet().AsCellSet(cellset);

  //verify that the point to cell connectivity types are correct
  viskores::cont::ArrayHandleConstant<viskores::UInt8> shapesPointToCell =
    cellset.GetShapesArray(viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
  viskores::cont::ArrayHandle<viskores::Id> connPointToCell = cellset.GetConnectivityArray(
    viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());

  VISKORES_TEST_ASSERT(shapesPointToCell.GetNumberOfValues() == 3, "Wrong number of shapes");
  VISKORES_TEST_ASSERT(connPointToCell.GetNumberOfValues() == 9, "Wrong connectivity length");

  //verify that the cell to point connectivity types are correct
  //note the handle storage types differ compared to point to cell
  viskores::cont::ArrayHandleConstant<viskores::UInt8> shapesCellToPoint =
    cellset.GetShapesArray(viskores::TopologyElementTagPoint(), viskores::TopologyElementTagCell());
  viskores::cont::ArrayHandle<viskores::Id> connCellToPoint = cellset.GetConnectivityArray(
    viskores::TopologyElementTagPoint(), viskores::TopologyElementTagCell());

  VISKORES_TEST_ASSERT(shapesCellToPoint.GetNumberOfValues() == 5, "Wrong number of shapes");
  VISKORES_TEST_ASSERT(connCellToPoint.GetNumberOfValues() == 9, "Wrong connectivity length");

  //run a basic for-each topology algorithm on this
  viskores::cont::ArrayHandle<viskores::Float32> input;
  viskores::cont::ArrayCopyShallowIfPossible(dataSet.GetField("pointvar").GetData(), input);
  viskores::cont::ArrayHandle<viskores::Float32> result;
  invoke(CellAverage{}, cellset, input, result);

  viskores::Float32 expected[3] = { 20.1333f, 30.1667f, 40.2333f };
  auto portal = result.ReadPortal();
  for (int i = 0; i < 3; ++i)
  {
    VISKORES_TEST_ASSERT(
      test_equal(portal.Get(i), expected[i]),
      "Wrong result for CellAverage worklet on explicit single type cellset data");
  }
}

VISKORES_CONT void Run()
{
  TestDataSet_SingleType();
}

} // anonymous namespace

int UnitTestDataSetSingleType(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
