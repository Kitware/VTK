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
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleBasic.h>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/geometry_refinement/Triangulate.h>
#include <viskores/filter/multi_block/MergeDataSets.h>
#include <viskores/worklet/WorkletMapField.h>
namespace
{
struct SetPointValuesV4Worklet : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);
  template <typename CoordinatesType, typename V4Type>
  VISKORES_EXEC void operator()(const CoordinatesType& coordinates, V4Type& vec4) const
  {
    vec4 = {
      coordinates[0] * 0.1, coordinates[1] * 0.1, coordinates[2] * 0.1, coordinates[0] * 0.1
    };
    return;
  }
};
struct SetPointValuesV1Worklet : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);
  template <typename CoordinatesType, typename ScalarType>
  VISKORES_EXEC void operator()(const CoordinatesType& coordinates, ScalarType& value) const
  {
    value = (coordinates[0] + coordinates[1] + coordinates[2]) * 0.1;
    return;
  }
};
viskores::cont::DataSet CreateSingleCellSetData(viskores::Vec3f coordinates[4])
{
  const int connectivitySize = 6;
  viskores::Id pointId[connectivitySize] = { 0, 1, 2, 1, 2, 3 };
  viskores::cont::ArrayHandle<viskores::Id> connectivity;
  connectivity.Allocate(connectivitySize);
  for (viskores::Id i = 0; i < connectivitySize; ++i)
  {
    connectivity.WritePortal().Set(i, pointId[i]);
  }
  viskores::cont::CellSetSingleType<> cellSet;
  cellSet.Fill(4, viskores::CELL_SHAPE_TRIANGLE, 3, connectivity);
  viskores::cont::DataSet dataSet;
  dataSet.AddCoordinateSystem(
    viskores::cont::make_CoordinateSystem("coords", coordinates, 4, viskores::CopyFlag::On));
  dataSet.SetCellSet(cellSet);

  std::vector<viskores::Float32> pointvar(4);
  std::iota(pointvar.begin(), pointvar.end(), 15.f);
  std::vector<viskores::Float32> cellvar(connectivitySize / 3);
  std::iota(cellvar.begin(), cellvar.end(), 132.f);
  dataSet.AddPointField("pointVar", pointvar);
  dataSet.AddCellField("cellVar", cellvar);
  return dataSet;
}

viskores::cont::DataSet CreateUniformData(viskores::Vec2f origin)
{
  viskores::cont::DataSetBuilderUniform dsb;
  viskores::Id2 dimensions(3, 2);
  viskores::cont::DataSet dataSet = dsb.Create(dimensions, origin, viskores::Vec2f(1, 1));
  constexpr viskores::Id nVerts = 6;
  constexpr viskores::Float32 var[nVerts] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f };
  dataSet.AddPointField("pointVar", var, nVerts);
  constexpr viskores::Float32 cellvar[2] = { 100.1f, 200.1f };
  dataSet.AddCellField("cellVar", cellvar, 2);
  return dataSet;
}

void TestUniformSameFieldsSameDataTypeSingleCellSet()
{
  std::cout << "TestUniformSameFieldsSameDataTypeSingleCellSet" << std::endl;
  const int nVerts = 4;
  viskores::Vec3f coordinates1[nVerts] = { viskores::Vec3f(0.0, 0.0, 0.0),
                                           viskores::Vec3f(1.0, 0.0, 0.0),
                                           viskores::Vec3f(0.0, 1.0, 0.0),
                                           viskores::Vec3f(1.0, 1.0, 0.0) };
  viskores::cont::DataSet dataSet1 = CreateSingleCellSetData(coordinates1);
  viskores::Vec3f coordinates2[nVerts] = { viskores::Vec3f(1.0, 0.0, 0.0),
                                           viskores::Vec3f(2.0, 0.0, 0.0),
                                           viskores::Vec3f(1.0, 1.0, 0.0),
                                           viskores::Vec3f(2.0, 1.0, 0.0) };
  viskores::cont::DataSet dataSet2 = CreateSingleCellSetData(coordinates2);
  viskores::cont::PartitionedDataSet inputDataSets;
  inputDataSets.AppendPartition(dataSet1);
  inputDataSets.AppendPartition(dataSet2);
  viskores::filter::multi_block::MergeDataSets mergeDataSets;
  auto result = mergeDataSets.Execute(inputDataSets);
  //Validating result cell sets
  auto cellSet = result.GetPartition(0).GetCellSet();
  viskores::cont::CellSetSingleType<> singleType =
    cellSet.AsCellSet<viskores::cont::CellSetSingleType<>>();
  VISKORES_TEST_ASSERT(singleType.GetCellShapeAsId() == 5, "Wrong cellShape Id");
  VISKORES_TEST_ASSERT(cellSet.GetNumberOfCells() == 4, "Wrong numberOfCells");
  VISKORES_TEST_ASSERT(cellSet.GetNumberOfPoints() == 8, "Wrong numberOfPoints");
  const viskores::cont::ArrayHandle<viskores::Id> connectivityArray =
    singleType.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                    viskores::TopologyElementTagPoint());
  viskores::cont::ArrayHandle<viskores::Id> validateConnArray =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 1, 2, 1, 2, 3, 4, 5, 6, 5, 6, 7 });
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(connectivityArray, validateConnArray));
  //Validating result fields
  viskores::cont::ArrayHandle<viskores::Float32> validatePointVar =
    viskores::cont::make_ArrayHandle<viskores::Float32>(
      { 15.0, 16.0, 17.0, 18.0, 15.0, 16.0, 17.0, 18.0 });
  viskores::cont::ArrayHandle<viskores::Float32> validateCellVar =
    viskores::cont::make_ArrayHandle<viskores::Float32>({ 132, 133, 132, 133 });
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(
                         result.GetPartition(0).GetField("pointVar").GetData(), validatePointVar),
                       "wrong pointVar values");
  VISKORES_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0).GetField("cellVar").GetData(), validateCellVar),
    "wrong cellVar values");
  //Validating result coordinates
  viskores::cont::CoordinateSystem coords = result.GetPartition(0).GetCoordinateSystem();
  viskores::cont::ArrayHandle<viskores::Vec3f> resultCoords =
    coords.GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec3f>>();
  viskores::cont::ArrayHandle<viskores::Vec3f> validateCoords =
    viskores::cont::make_ArrayHandle<viskores::Vec3f>({ { 0, 0, 0 },
                                                        { 1, 0, 0 },
                                                        { 0, 1, 0 },
                                                        { 1, 1, 0 },
                                                        { 1, 0, 0 },
                                                        { 2, 0, 0 },
                                                        { 1, 1, 0 },
                                                        { 2, 1, 0 } });
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(resultCoords, validateCoords),
                       "wrong validateCoords values");
}

void TestUniformSameFieldsSameDataType()
{
  std::cout << "TestUniformSameFieldsSameDataType" << std::endl;
  viskores::cont::PartitionedDataSet inputDataSets;
  viskores::cont::DataSet dataSet0 = CreateUniformData(viskores::Vec2f(0.0, 0.0));
  viskores::cont::DataSet dataSet1 = CreateUniformData(viskores::Vec2f(3.0, 0.0));
  inputDataSets.AppendPartition(dataSet0);
  inputDataSets.AppendPartition(dataSet1);
  viskores::filter::multi_block::MergeDataSets mergeDataSets;
  auto result = mergeDataSets.Execute(inputDataSets);
  //validating cellsets
  auto cellSet = result.GetPartition(0).GetCellSet();
  viskores::cont::CellSetExplicit<> explicitType =
    cellSet.AsCellSet<viskores::cont::CellSetExplicit<>>();
  const viskores::cont::ArrayHandle<viskores::Id> connectivityArray =
    explicitType.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                      viskores::TopologyElementTagPoint());
  const viskores::cont::ArrayHandle<viskores::UInt8> shapesArray = explicitType.GetShapesArray(
    viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
  const viskores::cont::ArrayHandle<viskores::Id> offsetsArray = explicitType.GetOffsetsArray(
    viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
  viskores::cont::ArrayHandle<viskores::Id> validateConnectivity =
    viskores::cont::make_ArrayHandle<viskores::Id>(
      { 0, 1, 4, 3, 1, 2, 5, 4, 6, 7, 10, 9, 7, 8, 11, 10 });
  viskores::cont::ArrayHandle<viskores::UInt8> validateShapes =
    viskores::cont::make_ArrayHandle<viskores::UInt8>({ 9, 9, 9, 9 });
  viskores::cont::ArrayHandle<viskores::Id> validateOffsets =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 4, 8, 12, 16 });
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(connectivityArray, validateConnectivity),
                       "wrong connectivity array");
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(shapesArray, validateShapes),
                       "wrong connectivity array");
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(offsetsArray, validateOffsets),
                       "wrong connectivity array");
  // validating fields
  viskores::cont::ArrayHandle<viskores::Float32> validatePointVar =
    viskores::cont::make_ArrayHandle<viskores::Float32>(
      { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f, 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f });
  viskores::cont::ArrayHandle<viskores::Float32> validateCellVar =
    viskores::cont::make_ArrayHandle<viskores::Float32>({ 100.1f, 200.1f, 100.1f, 200.1f });
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(
                         result.GetPartition(0).GetField("pointVar").GetData(), validatePointVar),
                       "wrong pointVar values");
  VISKORES_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0).GetField("cellVar").GetData(), validateCellVar),
    "wrong cellVar values");
  //validating coordinates
  viskores::cont::CoordinateSystem coords = result.GetPartition(0).GetCoordinateSystem();
  viskores::cont::ArrayHandle<viskores::Vec3f> resultCoords =
    coords.GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec3f>>();
  viskores::cont::ArrayHandle<viskores::Vec3f> validateCoords =
    viskores::cont::make_ArrayHandle<viskores::Vec3f>({ { 0, 0, 0 },
                                                        { 1, 0, 0 },
                                                        { 2, 0, 0 },
                                                        { 0, 1, 0 },
                                                        { 1, 1, 0 },
                                                        { 2, 1, 0 },
                                                        { 3, 0, 0 },
                                                        { 4, 0, 0 },
                                                        { 5, 0, 0 },
                                                        { 3, 1, 0 },
                                                        { 4, 1, 0 },
                                                        { 5, 1, 0 } });
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(resultCoords, validateCoords),
                       "wrong validateCoords values");
}
void TestTriangleSameFieldsSameDataType()
{
  std::cout << "TestTriangleSameFieldsSameDataType" << std::endl;
  viskores::cont::PartitionedDataSet input;
  viskores::cont::DataSetBuilderUniform dsb;
  viskores::Id3 dimensions(3, 2, 1);
  viskores::cont::DataSet dataSet0 =
    dsb.Create(dimensions,
               viskores::make_Vec<viskores::FloatDefault>(0, 0, 0),
               viskores::make_Vec<viskores::FloatDefault>(1, 1, 0));
  constexpr viskores::Id nVerts = 6;
  constexpr viskores::Float32 var[nVerts] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f };
  dataSet0.AddPointField("pointVar", var, nVerts);
  constexpr viskores::Float32 cellvar[2] = { 100.1f, 200.1f };
  dataSet0.AddCellField("cellVar", cellvar, 2);
  viskores::filter::geometry_refinement::Triangulate triangulate;
  auto tranDataSet0 = triangulate.Execute(dataSet0);
  viskores::cont::DataSet dataSet1 =
    dsb.Create(dimensions,
               viskores::make_Vec<viskores::FloatDefault>(3, 0, 0),
               viskores::make_Vec<viskores::FloatDefault>(1, 1, 0));
  constexpr viskores::Float32 var1[nVerts] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f };
  dataSet1.AddPointField("pointVar", var1, nVerts);
  constexpr viskores::Float32 cellvar1[2] = { 100.1f, 200.1f };
  dataSet1.AddCellField("cellVar", cellvar1, 2);
  auto tranDataSet1 = triangulate.Execute(dataSet1);
  input.AppendPartition(tranDataSet0);
  input.AppendPartition(tranDataSet1);
  viskores::filter::multi_block::MergeDataSets mergeDataSets;
  auto result = mergeDataSets.Execute(input);
  //validating results
  auto cellSet = result.GetPartition(0).GetCellSet();
  viskores::cont::CellSetSingleType<> singleType =
    cellSet.AsCellSet<viskores::cont::CellSetSingleType<>>();
  VISKORES_TEST_ASSERT(singleType.GetCellShapeAsId() == 5, "Wrong cellShape Id");
  VISKORES_TEST_ASSERT(cellSet.GetNumberOfCells() == 8, "Wrong numberOfCells");
  VISKORES_TEST_ASSERT(cellSet.GetNumberOfPoints() == 12, "Wrong numberOfPoints");
  const viskores::cont::ArrayHandle<viskores::Id> connectivityArray =
    singleType.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                    viskores::TopologyElementTagPoint());
  viskores::cont::ArrayHandle<viskores::Id> validateConnArray =
    viskores::cont::make_ArrayHandle<viskores::Id>(
      { 0, 1, 4, 0, 4, 3, 1, 2, 5, 1, 5, 4, 6, 7, 10, 6, 10, 9, 7, 8, 11, 7, 11, 10 });
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(connectivityArray, validateConnArray));
  //Validating result fields
  viskores::cont::ArrayHandle<viskores::Float32> validatePointVar =
    viskores::cont::make_ArrayHandle<viskores::Float32>(
      { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f, 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f });
  viskores::cont::ArrayHandle<viskores::Float32> validateCellVar =
    viskores::cont::make_ArrayHandle<viskores::Float32>(
      { 100.1f, 100.1f, 200.1f, 200.1f, 100.1f, 100.1f, 200.1f, 200.1f });

  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(
                         result.GetPartition(0).GetField("pointVar").GetData(), validatePointVar),
                       "wrong pointVar values");
  VISKORES_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0).GetField("cellVar").GetData(), validateCellVar),
    "wrong cellVar values");
  //Validating result coordinates
  viskores::cont::CoordinateSystem coords = result.GetPartition(0).GetCoordinateSystem();
  viskores::cont::ArrayHandle<viskores::Vec3f> resultCoords =
    coords.GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec3f>>();
  viskores::cont::ArrayHandle<viskores::Vec3f> validateCoords =
    viskores::cont::make_ArrayHandle<viskores::Vec3f>({ { 0, 0, 0 },
                                                        { 1, 0, 0 },
                                                        { 2, 0, 0 },
                                                        { 0, 1, 0 },
                                                        { 1, 1, 0 },
                                                        { 2, 1, 0 },
                                                        { 3, 0, 0 },
                                                        { 4, 0, 0 },
                                                        { 5, 0, 0 },
                                                        { 3, 1, 0 },
                                                        { 4, 1, 0 },
                                                        { 5, 1, 0 } });
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(resultCoords, validateCoords),
                       "wrong validateCoords values");
}

void TestDiffCellsSameFieldsSameDataType()
{
  std::cout << "TestDiffCellsSameFieldsSameDataType" << std::endl;
  viskores::Vec3f coordinates1[4] = { viskores::Vec3f(0.0, 0.0, 0.0),
                                      viskores::Vec3f(1.0, 0.0, 0.0),
                                      viskores::Vec3f(0.0, 1.0, 0.0),
                                      viskores::Vec3f(1.0, 1.0, 0.0) };
  viskores::cont::DataSet dataSet0 = CreateSingleCellSetData(coordinates1);
  viskores::cont::DataSet dataSet1 = CreateUniformData(viskores::Vec2f(3.0, 0.0));
  viskores::cont::PartitionedDataSet input;
  input.AppendPartition(dataSet0);
  input.AppendPartition(dataSet1);
  viskores::filter::multi_block::MergeDataSets mergeDataSets;
  auto result = mergeDataSets.Execute(input);
  //validating cellsets
  auto cellSet = result.GetPartition(0).GetCellSet();
  viskores::cont::CellSetExplicit<> explicitType =
    cellSet.AsCellSet<viskores::cont::CellSetExplicit<>>();
  const viskores::cont::ArrayHandle<viskores::Id> connectivityArray =
    explicitType.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                      viskores::TopologyElementTagPoint());
  const viskores::cont::ArrayHandle<viskores::UInt8> shapesArray = explicitType.GetShapesArray(
    viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
  const viskores::cont::ArrayHandle<viskores::Id> offsetsArray = explicitType.GetOffsetsArray(
    viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
  viskores::cont::ArrayHandle<viskores::Id> validateConnectivity =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 1, 2, 1, 2, 3, 4, 5, 8, 7, 5, 6, 9, 8 });
  viskores::cont::ArrayHandle<viskores::UInt8> validateShapes =
    viskores::cont::make_ArrayHandle<viskores::UInt8>({ 5, 5, 9, 9 });
  viskores::cont::ArrayHandle<viskores::Id> validateOffsets =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 3, 6, 10, 14 });
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(connectivityArray, validateConnectivity),
                       "wrong connectivity array");
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(shapesArray, validateShapes),
                       "wrong connectivity array");
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(offsetsArray, validateOffsets),
                       "wrong connectivity array");
  // Validating fields
  viskores::cont::ArrayHandle<viskores::Float32> validatePointVar =
    viskores::cont::make_ArrayHandle<viskores::Float32>(
      { 15.f, 16.f, 17.f, 18.f, 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f });
  viskores::cont::ArrayHandle<viskores::Float32> validateCellVar =
    viskores::cont::make_ArrayHandle<viskores::Float32>({ 132.0f, 133.0f, 100.1f, 200.1f });

  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(
                         result.GetPartition(0).GetField("pointVar").GetData(), validatePointVar),
                       "wrong pointVar values");
  VISKORES_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0).GetField("cellVar").GetData(), validateCellVar),
    "wrong cellVar values");
  //Validating coordinates
  viskores::cont::CoordinateSystem coords = result.GetPartition(0).GetCoordinateSystem();
  viskores::cont::ArrayHandle<viskores::Vec3f> resultCoords =
    coords.GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec3f>>();
  viskores::cont::ArrayHandle<viskores::Vec3f> validateCoords =
    viskores::cont::make_ArrayHandle<viskores::Vec3f>({ { 0, 0, 0 },
                                                        { 1, 0, 0 },
                                                        { 0, 1, 0 },
                                                        { 1, 1, 0 },
                                                        { 3, 0, 0 },
                                                        { 4, 0, 0 },
                                                        { 5, 0, 0 },
                                                        { 3, 1, 0 },
                                                        { 4, 1, 0 },
                                                        { 5, 1, 0 } });
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(resultCoords, validateCoords),
                       "Wrong coords values");
}

void TestDifferentCoords()
{
  std::cout << "TestDifferentCoords" << std::endl;
  viskores::cont::PartitionedDataSet inputDataSets;
  viskores::cont::DataSet dataSet0 = CreateUniformData(viskores::Vec2f(0.0, 0.0));
  viskores::Vec3f coordinates[6];
  dataSet0.AddCoordinateSystem(
    viskores::cont::make_CoordinateSystem("coordsExtra", coordinates, 6, viskores::CopyFlag::On));
  viskores::cont::DataSet dataSet1 = CreateUniformData(viskores::Vec2f(3.0, 0.0));
  inputDataSets.AppendPartition(dataSet0);
  inputDataSets.AppendPartition(dataSet1);
  viskores::filter::multi_block::MergeDataSets mergeDataSets;
  try
  {
    mergeDataSets.Execute(inputDataSets);
  }
  catch (viskores::cont::ErrorExecution& e)
  {
    VISKORES_TEST_ASSERT(
      e.GetMessage().find("Data sets have different number of coordinate systems") !=
      std::string::npos);
  }
  viskores::cont::DataSetBuilderUniform dsb;
  viskores::Id2 dimensions(3, 2);
  viskores::cont::DataSet dataSet2 =
    dsb.Create(dimensions, viskores::Vec2f(0.0, 0.0), viskores::Vec2f(1, 1));
  constexpr viskores::Float32 var2[6] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f };
  dataSet2.AddPointField("pointVarExtra", var2, 6);
  constexpr viskores::Float32 cellvar2[2] = { 100.1f, 200.1f };
  dataSet2.AddCellField("cellVarExtra", cellvar2, 2);
  viskores::cont::PartitionedDataSet inputDataSets2;
  inputDataSets2.AppendPartition(dataSet1);
  inputDataSets2.AppendPartition(dataSet2);
  try
  {
    mergeDataSets.Execute(inputDataSets2);
  }
  catch (viskores::cont::ErrorExecution& e)
  {
    VISKORES_TEST_ASSERT(e.GetMessage().find("Coordinates system name:") != std::string::npos);
  }
}

void TestSameFieldsDifferentDataType()
{
  std::cout << "TestSameFieldsDifferentDataType" << std::endl;
  viskores::cont::DataSetBuilderUniform dsb;
  viskores::Id2 dimensions(3, 2);
  viskores::cont::DataSet dataSet1 =
    dsb.Create(dimensions, viskores::Vec2f(0.0, 0.0), viskores::Vec2f(1, 1));
  constexpr viskores::Float32 var[6] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f };
  dataSet1.AddPointField("pointVar", var, 6);
  constexpr viskores::Float32 cellvar[2] = { 100.1f, 200.1f };
  dataSet1.AddCellField("cellVar", cellvar, 2);
  viskores::cont::DataSet dataSet2 =
    dsb.Create(dimensions, viskores::Vec2f(0.0, 0.0), viskores::Vec2f(1, 1));
  constexpr viskores::Id var2[6] = { 10, 20, 30, 40, 50, 60 };
  dataSet2.AddPointField("pointVar", var2, 6);
  constexpr viskores::Id cellvar2[2] = { 100, 200 };
  dataSet2.AddCellField("cellVar", cellvar2, 2);
  viskores::cont::PartitionedDataSet inputDataSets;
  inputDataSets.AppendPartition(dataSet1);
  inputDataSets.AppendPartition(dataSet2);
  viskores::filter::multi_block::MergeDataSets mergeDataSets;
  auto result = mergeDataSets.Execute(inputDataSets);
  //Validating fields in results, they will use the first partition's field type
  viskores::cont::ArrayHandle<viskores::Float32> validatePointVar =
    viskores::cont::make_ArrayHandle<viskores::Float32>(
      { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f, 10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f });
  viskores::cont::ArrayHandle<viskores::Float32> validateCellVar =
    viskores::cont::make_ArrayHandle<viskores::Float32>({ 100.1f, 200.1f, 100.0f, 200.0f });
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(
                         result.GetPartition(0).GetField("pointVar").GetData(), validatePointVar),
                       "wrong pointVar values");
  VISKORES_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0).GetField("cellVar").GetData(), validateCellVar),
    "wrong cellVar values");
}
void TestMissingFieldsAndSameFieldName()
{
  std::cout << "TestMissingFieldsAndSameFieldName" << std::endl;
  viskores::cont::DataSetBuilderUniform dsb;
  viskores::Id2 dimensions(3, 2);
  viskores::cont::DataSet dataSet1 =
    dsb.Create(dimensions, viskores::Vec2f(0.0, 0.0), viskores::Vec2f(1, 1));
  constexpr viskores::Float32 pointVar[6] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f };
  viskores::cont::DataSet dataSet2 =
    dsb.Create(dimensions, viskores::Vec2f(0.0, 0.0), viskores::Vec2f(1, 1));
  constexpr viskores::Id cellvar[2] = { 100, 200 };
  viskores::cont::PartitionedDataSet inputDataSets;
  dataSet1.AddPointField("pointVar", pointVar, 6);
  dataSet2.AddCellField("cellVar", cellvar, 2);
  //For testing the case where one field is associated with point in one partition
  //and one field (with a same name) is associated with cell in another partition
  dataSet1.AddPointField("fieldSameName", pointVar, 6);
  dataSet2.AddCellField("fieldSameName", cellvar, 2);
  //For testing the case where one partition have point field and a cell field with the same name.
  dataSet1.AddPointField("fieldSameName2", pointVar, 6);
  dataSet2.AddPointField("fieldSameName2", pointVar, 6);
  dataSet2.AddCellField("fieldSameName2", cellvar, 2);
  inputDataSets.AppendPartition(dataSet1);
  inputDataSets.AppendPartition(dataSet2);
  viskores::filter::multi_block::MergeDataSets mergeDataSets;
  mergeDataSets.SetInvalidValue(viskores::Float64(0));
  auto result = mergeDataSets.Execute(inputDataSets);
  //Validating fields in results, they will use InvalidValues for missing fields
  viskores::cont::ArrayHandle<viskores::Float32> validatePointVar1 =
    viskores::cont::make_ArrayHandle<viskores::Float32>(
      { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f });
  viskores::cont::ArrayHandle<viskores::Float32> validatePointVar2 =
    viskores::cont::make_ArrayHandle<viskores::Float32>(
      { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f, 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f });
  viskores::cont::ArrayHandle<viskores::Id> validateCellVar =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 0, 100, 200 });
  VISKORES_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0)
                              .GetField("pointVar", viskores::cont::Field::Association::Points)
                              .GetData(),
                            validatePointVar1),
    "wrong pointVar values");
  VISKORES_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0)
                              .GetField("cellVar", viskores::cont::Field::Association::Cells)
                              .GetData(),
                            validateCellVar),
    "wrong cellVar values");
  VISKORES_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0)
                              .GetField("fieldSameName", viskores::cont::Field::Association::Points)
                              .GetData(),
                            validatePointVar1),
    "wrong fieldSameName values");
  VISKORES_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0)
                              .GetField("fieldSameName", viskores::cont::Field::Association::Cells)
                              .GetData(),
                            validateCellVar),
    "wrong fieldSameName values");
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(
                         result.GetPartition(0)
                           .GetField("fieldSameName2", viskores::cont::Field::Association::Points)
                           .GetData(),
                         validatePointVar2),
                       "wrong fieldSameName2 values");
  VISKORES_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0)
                              .GetField("fieldSameName2", viskores::cont::Field::Association::Cells)
                              .GetData(),
                            validateCellVar),
    "wrong fieldSameName2 values");
}

void TestCustomizedVecField()
{
  std::cout << "TestCustomizedVecField" << std::endl;
  viskores::cont::DataSetBuilderUniform dsb;
  viskores::Id2 dimensions(3, 2);
  viskores::cont::DataSet dataSet1 =
    dsb.Create(dimensions, viskores::Vec2f(0.0, 0.0), viskores::Vec2f(1, 1));
  viskores::cont::ArrayHandle<viskores::Vec<viskores::Float64, 4>> pointVar1Vec4;
  pointVar1Vec4.Allocate(6);
  viskores::cont::Invoker invoker;
  invoker(SetPointValuesV4Worklet{}, dataSet1.GetCoordinateSystem().GetData(), pointVar1Vec4);
  dataSet1.AddPointField("pointVarV4", pointVar1Vec4);
  viskores::cont::DataSet dataSet2 =
    dsb.Create(dimensions, viskores::Vec2f(3.0, 0.0), viskores::Vec2f(1, 1));
  viskores::cont::ArrayHandle<viskores::Vec<viskores::Float64, 4>> pointVar2Vec4;
  pointVar2Vec4.Allocate(6);
  invoker(SetPointValuesV4Worklet{}, dataSet2.GetCoordinateSystem().GetData(), pointVar2Vec4);
  dataSet2.AddPointField("pointVarV4", pointVar2Vec4);
  viskores::cont::PartitionedDataSet inputDataSets;
  inputDataSets.AppendPartition(dataSet1);
  inputDataSets.AppendPartition(dataSet2);
  viskores::filter::multi_block::MergeDataSets mergeDataSets;
  auto result = mergeDataSets.Execute(inputDataSets);
  viskores::cont::ArrayHandle<viskores::Vec<viskores::Float64, 4>> validatePointVar;
  //Set point validatePointVar array based on coordinates.
  invoker(SetPointValuesV4Worklet{},
          result.GetPartition(0).GetCoordinateSystem().GetData(),
          validatePointVar);
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(
                         result.GetPartition(0).GetField("pointVarV4").GetData(), validatePointVar),
                       "wrong pointVar values");
}

void TestMoreThanTwoPartitions()
{
  std::cout << "TestMoreThanTwoPartitions" << std::endl;
  viskores::cont::DataSetBuilderUniform dsb;
  viskores::Id2 dimensions(3, 2);
  viskores::cont::Invoker invoker;
  viskores::cont::PartitionedDataSet inputDataSets;
  for (viskores::Id i = 0; i < 5; i++)
  {
    for (viskores::Id j = 0; j < 5; j++)
    {
      viskores::cont::DataSet dataSet =
        dsb.Create(dimensions,
                   viskores::Vec2f(static_cast<viskores::FloatDefault>(i),
                                   static_cast<viskores::FloatDefault>(j)),
                   viskores::Vec2f(1, 1));
      viskores::cont::ArrayHandle<viskores::Float64> pointVarArray;
      invoker(SetPointValuesV1Worklet{}, dataSet.GetCoordinateSystem().GetData(), pointVarArray);
      dataSet.AddPointField("pointVar", pointVarArray);
      inputDataSets.AppendPartition(dataSet);
    }
  }
  viskores::filter::multi_block::MergeDataSets mergeDataSets;
  auto result = mergeDataSets.Execute(inputDataSets);
  viskores::cont::ArrayHandle<viskores::Float64> validatePointVar;
  invoker(SetPointValuesV1Worklet{},
          result.GetPartition(0).GetCoordinateSystem().GetData(),
          validatePointVar);
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(
                         result.GetPartition(0).GetField("pointVar").GetData(), validatePointVar),
                       "wrong pointVar values");
}

void TestEmptyPartitions()
{
  std::cout << "TestEmptyPartitions" << std::endl;
  viskores::cont::PartitionedDataSet inputDataSets;
  viskores::cont::DataSet dataSet1 = CreateUniformData(viskores::Vec2f(0.0, 0.0));
  viskores::cont::DataSet dataSet2;
  inputDataSets.AppendPartition(dataSet1);
  inputDataSets.AppendPartition(dataSet2);
  viskores::filter::multi_block::MergeDataSets mergeDataSets;
  auto result = mergeDataSets.Execute(inputDataSets);
  //Validating data sets
  VISKORES_TEST_ASSERT(result.GetNumberOfPartitions() == 1, "Wrong number of partitions");
  auto cellSet = result.GetPartition(0).GetCellSet();
  VISKORES_TEST_ASSERT(cellSet.GetNumberOfCells() == 2, "Wrong numberOfCells");
  VISKORES_TEST_ASSERT(cellSet.GetNumberOfPoints() == 6, "Wrong numberOfPoints");
  viskores::cont::ArrayHandle<viskores::Float32> validatePointVar =
    viskores::cont::make_ArrayHandle<viskores::Float32>(
      { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f });
  viskores::cont::ArrayHandle<viskores::Float32> validateCellVar =
    viskores::cont::make_ArrayHandle<viskores::Float32>({ 100.1f, 200.1f });
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(
                         result.GetPartition(0).GetField("pointVar").GetData(), validatePointVar),
                       "wrong pointVar values");
  VISKORES_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0).GetField("cellVar").GetData(), validateCellVar),
    "wrong cellVar values");
  viskores::cont::PartitionedDataSet inputDataSets2;
  inputDataSets2.AppendPartition(dataSet2);
  inputDataSets2.AppendPartition(dataSet1);
  auto result2 = mergeDataSets.Execute(inputDataSets2);
  VISKORES_TEST_ASSERT(result2.GetNumberOfPartitions() == 1, "Wrong number of partitions");
  cellSet = result2.GetPartition(0).GetCellSet();
  VISKORES_TEST_ASSERT(cellSet.GetNumberOfCells() == 2, "Wrong numberOfCells");
  VISKORES_TEST_ASSERT(cellSet.GetNumberOfPoints() == 6, "Wrong numberOfPoints");
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(
                         result2.GetPartition(0).GetField("pointVar").GetData(), validatePointVar),
                       "wrong pointVar values");
  VISKORES_TEST_ASSERT(
    test_equal_ArrayHandles(result2.GetPartition(0).GetField("cellVar").GetData(), validateCellVar),
    "wrong cellVar values");
}

void TestMissingVectorFields()
{
  std::cout << "TestMissingVectorFields" << std::endl;
  viskores::cont::DataSetBuilderUniform dsb;
  viskores::Id2 dimensions(3, 2);
  viskores::cont::DataSet dataSet1 =
    dsb.Create(dimensions, viskores::Vec2f(0.0, 0.0), viskores::Vec2f(1, 1));
  viskores::cont::ArrayHandle<viskores::Vec<viskores::Float64, 4>> pointVarVec4;
  pointVarVec4.Allocate(6);
  viskores::cont::Invoker invoker;
  invoker(SetPointValuesV4Worklet{}, dataSet1.GetCoordinateSystem().GetData(), pointVarVec4);
  dataSet1.AddPointField("pointVarV4", pointVarVec4);
  viskores::cont::DataSet dataSet2 =
    dsb.Create(dimensions, viskores::Vec2f(0.0, 0.0), viskores::Vec2f(1, 1));
  viskores::cont::ArrayHandle<viskores::Vec3f_64> cellVarVec3 =
    viskores::cont::make_ArrayHandle<viskores::Vec3f_64>({ { 1.0, 2.0, 3.0 }, { 4.0, 5.0, 6.0 } });
  dataSet2.AddCellField("cellVarV3", cellVarVec3);
  viskores::cont::PartitionedDataSet inputDataSets;
  inputDataSets.AppendPartition(dataSet1);
  inputDataSets.AppendPartition(dataSet2);
  viskores::filter::multi_block::MergeDataSets mergeDataSets;
  auto result = mergeDataSets.Execute(inputDataSets);

  //checking results
  viskores::cont::ArrayHandle<viskores::Vec<viskores::Float64, 4>> validatePointVar =
    viskores::cont::make_ArrayHandle<viskores::Vec<viskores::Float64, 4>>(
      { { 0, 0, 0, 0 },
        { 0.1, 0, 0, 0.1 },
        { 0.2, 0, 0, 0.2 },
        { 0, 0.1, 0, 0 },
        { 0.1, 0.1, 0, 0.1 },
        { 0.2, 0.1, 0, 0.2 },
        { viskores::Nan64(), viskores::Nan64(), viskores::Nan64(), viskores::Nan64() },
        { viskores::Nan64(), viskores::Nan64(), viskores::Nan64(), viskores::Nan64() },
        { viskores::Nan64(), viskores::Nan64(), viskores::Nan64(), viskores::Nan64() },
        { viskores::Nan64(), viskores::Nan64(), viskores::Nan64(), viskores::Nan64() },
        { viskores::Nan64(), viskores::Nan64(), viskores::Nan64(), viskores::Nan64() },
        { viskores::Nan64(), viskores::Nan64(), viskores::Nan64(), viskores::Nan64() } });
  viskores::cont::ArrayHandle<viskores::Vec3f_64> validateCellVar =
    viskores::cont::make_ArrayHandle<viskores::Vec3f_64>(
      { { viskores::Nan64(), viskores::Nan64(), viskores::Nan64() },
        { viskores::Nan64(), viskores::Nan64(), viskores::Nan64() },
        { 1.0, 2.0, 3.0 },
        { 4.0, 5.0, 6.0 } });
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(
                         result.GetPartition(0).GetField("pointVarV4").GetData(), validatePointVar),
                       "wrong point values for TestMissingVectorFields");
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(
                         result.GetPartition(0).GetField("cellVarV3").GetData(), validateCellVar),
                       "wrong cell values for TestMissingVectorFields");
}

void TestMergeDataSetsFilter()
{
  //same cell type (triangle), same field name, same data type, cellset is single type
  TestUniformSameFieldsSameDataTypeSingleCellSet();
  //same cell type (square), same field name, same data type
  TestUniformSameFieldsSameDataType();
  //same cell type (triangle), same field name, same data type
  TestTriangleSameFieldsSameDataType();
  //same cell type (square), same field name, different data type
  TestSameFieldsDifferentDataType();
  //different coordinates name
  TestDifferentCoords();
  //different cell types, same field name, same type
  TestDiffCellsSameFieldsSameDataType();
  //test multiple partitions
  TestMoreThanTwoPartitions();
  //some partitions have missing scalar fields
  TestMissingFieldsAndSameFieldName();
  //test empty partitions
  TestEmptyPartitions();
  //test customized types
  TestCustomizedVecField();
  //some partitions have missing vector fields
  TestMissingVectorFields();
}
} // anonymous namespace
int UnitTestMergeDataSetsFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestMergeDataSetsFilter, argc, argv);
}
