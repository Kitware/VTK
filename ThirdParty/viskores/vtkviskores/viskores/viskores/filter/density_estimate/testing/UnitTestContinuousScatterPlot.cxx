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
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/density_estimate/ContinuousScatterPlot.h>

namespace
{

std::vector<viskores::Vec3f> tetraCoords{ viskores::Vec3f(0.0f, 0.0f, 0.0f),
                                          viskores::Vec3f(2.0f, 0.0f, 0.0f),
                                          viskores::Vec3f(2.0f, 2.0f, 0.0f),
                                          viskores::Vec3f(1.0f, 0.0f, 2.0f) };

std::vector<viskores::UInt8> tetraShape{ viskores::CELL_SHAPE_TETRA };
std::vector<viskores::IdComponent> tetraIndex{ 4 };
std::vector<viskores::Id> tetraConnectivity{ 0, 1, 2, 3 };

std::vector<viskores::Vec3f> multiCoords{
  viskores::Vec3f(0.0f, 0.0f, 0.0f), viskores::Vec3f(2.0f, 0.0f, 0.0f),
  viskores::Vec3f(2.0f, 2.0f, 0.0f), viskores::Vec3f(1.0f, 0.0f, 2.0f),
  viskores::Vec3f(0.0f, 0.0f, 0.0f), viskores::Vec3f(2.0f, 0.0f, 0.0f),
  viskores::Vec3f(2.0f, 2.0f, 0.0f), viskores::Vec3f(1.0f, 0.0f, 2.0f),
  viskores::Vec3f(0.0f, 0.0f, 0.0f), viskores::Vec3f(2.0f, 0.0f, 0.0f),
  viskores::Vec3f(2.0f, 2.0f, 0.0f), viskores::Vec3f(1.0f, 0.0f, 2.0f)
};
std::vector<viskores::UInt8> multiShapes{ viskores::CELL_SHAPE_TETRA,
                                          viskores::CELL_SHAPE_TETRA,
                                          viskores::CELL_SHAPE_TETRA };
std::vector<viskores::IdComponent> multiIndices{ 4, 4, 4 };
std::vector<viskores::Id> multiConnectivity{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

template <typename FieldType1, typename FieldType2>
viskores::cont::DataSet makeDataSet(const std::vector<viskores::Vec3f>& ds_coords,
                                    const std::vector<viskores::UInt8>& ds_shapes,
                                    const std::vector<viskores::IdComponent>& ds_indices,
                                    const std::vector<viskores::Id>& ds_connectivity,
                                    const FieldType1& scalar1,
                                    const FieldType2& scalar2)
{
  viskores::cont::DataSetBuilderExplicit dsb;
  viskores::cont::DataSet ds = dsb.Create(ds_coords, ds_shapes, ds_indices, ds_connectivity);

  ds.AddPointField("scalar1", scalar1, 4);
  ds.AddPointField("scalar2", scalar2, 4);

  return ds;
}

viskores::cont::DataSet executeFilter(const viskores::cont::DataSet ds)
{
  auto continuousSCP = viskores::filter::density_estimate::ContinuousScatterPlot();
  continuousSCP.SetActiveFieldsPair("scalar1", "scalar2");
  return continuousSCP.Execute(ds);
}

template <typename PositionsPortalType, typename FieldType1, typename FieldType2>
void testCoordinates(const PositionsPortalType& positionsP,
                     const FieldType1& scalar1,
                     const FieldType2& scalar2,
                     const viskores::IdComponent numberOfPoints)
{
  for (viskores::IdComponent i = 0; i < numberOfPoints; i++)
  {
    VISKORES_TEST_ASSERT(test_equal(positionsP.Get(i)[0], scalar1[i]), "Wrong point coordinates");
    VISKORES_TEST_ASSERT(test_equal(positionsP.Get(i)[1], scalar2[i]), "Wrong point coordinates");
    VISKORES_TEST_ASSERT(test_equal(positionsP.Get(i)[2], 0.0f),
                         "Z coordinate value in the scatterplot should always be null");
  }
}

template <typename DensityArrayType, typename FieldType>
void testDensity(const DensityArrayType& density,
                 const viskores::IdComponent& centerId,
                 const FieldType& centerDensity)
{
  for (viskores::IdComponent i = 0; i < density.GetNumberOfValues(); i++)
  {
    if (i == centerId)
    {
      VISKORES_TEST_ASSERT(test_equal(density.Get(i), centerDensity),
                           "Wrong density in the middle point of the cell");
    }
    else
    {
      VISKORES_TEST_ASSERT(test_equal(density.Get(i), 0.0f),
                           "Density on the edge of the tetrahedron should be null");
    }
  }
}

template <typename CellSetType>
void testShapes(const CellSetType& cellSet)
{
  for (viskores::IdComponent i = 0; i < cellSet.GetNumberOfCells(); i++)
  {
    VISKORES_TEST_ASSERT(cellSet.GetCellShape(i) == viskores::CELL_SHAPE_TRIANGLE);
  }
}

template <typename CellSetType>
void testConnectivity(
  const CellSetType& cellSet,
  const viskores::cont::ArrayHandle<viskores::IdComponent>& expectedConnectivityArray)
{
  VISKORES_TEST_ASSERT(
    test_equal_ArrayHandles(cellSet.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                                         viskores::TopologyElementTagPoint()),
                            expectedConnectivityArray),
    "Wrong connectivity");
}

void TestSingleTetraProjectionQuadConvex()
{
  constexpr viskores::FloatDefault scalar1[4] = {
    0.0f,
    1.0f,
    0.0f,
    -2.0f,

  };

  constexpr viskores::FloatDefault scalar2[4] = {
    -1.0f,
    0.0f,
    2.0f,
    0.0f,
  };

  viskores::cont::DataSet ds =
    makeDataSet(tetraCoords, tetraShape, tetraIndex, tetraConnectivity, scalar1, scalar2);
  auto scatterPlot = executeFilter(ds);

  VISKORES_TEST_ASSERT(test_equal(scatterPlot.GetNumberOfCells(), 4),
                       "Wrong number of projected triangles in the continuous scatter plot");
  VISKORES_TEST_ASSERT(test_equal(scatterPlot.GetNumberOfPoints(), 5),
                       "Wrong number of projected points in the continuous scatter plot");

  // Test point positions
  auto positions = scatterPlot.GetCoordinateSystem().GetDataAsMultiplexer();
  auto positionsP = positions.ReadPortal();
  testCoordinates(positionsP, scalar1, scalar2, 4);

  // Test intersection point coordinates
  VISKORES_TEST_ASSERT(test_equal(positionsP.Get(4), viskores::Vec3f{ 0.0f, 0.0f, 0.0f }),
                       "Wrong intersection point coordinates");


  // Test for triangle shapes
  viskores::cont::CellSetSingleType<> cellSet;
  scatterPlot.GetCellSet().AsCellSet(cellSet);
  testShapes(cellSet);

  // Test connectivity
  auto expectedConnectivityArray =
    viskores::cont::make_ArrayHandle<viskores::IdComponent>({ 0, 1, 4, 1, 2, 4, 2, 3, 4, 3, 0, 4 });
  testConnectivity(cellSet, expectedConnectivityArray);

  // Test density values
  auto density = scatterPlot.GetField("density")
                   .GetData()
                   .AsArrayHandle<viskores::cont::ArrayHandle<viskores::FloatDefault>>()
                   .ReadPortal();
  testDensity(density, 4, 0.888889f);
}

void TestSingleTetraProjectionQuadSelfIntersect()
{
  constexpr viskores::FloatDefault scalar1[4] = {
    0.0f,
    0.0f,
    1.0f,
    -2.0f,
  };

  constexpr viskores::FloatDefault scalar2[4] = {
    -1.0f,
    2.0f,
    0.0f,
    0.0f,
  };

  viskores::cont::DataSet ds =
    makeDataSet(tetraCoords, tetraShape, tetraIndex, tetraConnectivity, scalar1, scalar2);
  auto scatterPlot = executeFilter(ds);

  VISKORES_TEST_ASSERT(test_equal(scatterPlot.GetNumberOfCells(), 4),
                       "Wrong number of projected triangles in the continuous scatter plot");

  // Test point positions
  auto positions = scatterPlot.GetCoordinateSystem().GetDataAsMultiplexer();
  auto positionsP = positions.ReadPortal();
  testCoordinates(positionsP, scalar1, scalar2, 4);


  // Test connectivity
  viskores::cont::CellSetSingleType<> cellSet;
  scatterPlot.GetCellSet().AsCellSet(cellSet);
  auto expectedConnectivityArray =
    viskores::cont::make_ArrayHandle<viskores::IdComponent>({ 0, 2, 4, 2, 1, 4, 1, 3, 4, 3, 0, 4 });
  testConnectivity(cellSet, expectedConnectivityArray);
}

void TestSingleTetraProjectionQuadInverseOrder()
{
  constexpr viskores::FloatDefault scalar1[4] = {
    -2.0f,
    0.0f,
    1.0f,
    0.0f,
  };

  constexpr viskores::FloatDefault scalar2[4] = {
    0.0f,
    2.0f,
    0.0f,
    -1.0f,
  };

  viskores::cont::DataSet ds =
    makeDataSet(tetraCoords, tetraShape, tetraIndex, tetraConnectivity, scalar1, scalar2);
  auto scatterPlot = executeFilter(ds);

  VISKORES_TEST_ASSERT(test_equal(scatterPlot.GetNumberOfCells(), 4),
                       "Wrong number of projected triangles in the continuous scatter plot");

  // Test connectivity
  viskores::cont::CellSetSingleType<> cellSet;
  scatterPlot.GetCellSet().AsCellSet(cellSet);
  auto expectedConnectivityArray = viskores::cont::make_ArrayHandle<viskores::IdComponent>(
    { 0,
      1,
      4,
      1,
      2,
      4,
      2,
      3,
      4,
      3,
      0,
      4 }); // Inverting the order of points should not change connectivity
  testConnectivity(cellSet, expectedConnectivityArray);
}

void TestSingleTetraProjectionQuadSelfIntersectSecond()
{
  constexpr viskores::FloatDefault scalar1[4] = {
    0.0f,
    1.0f,
    -2.0f,
    0.0f,
  };

  constexpr viskores::FloatDefault scalar2[4] = {
    -1.0f,
    0.0f,
    0.0f,
    2.0f,
  };

  viskores::cont::DataSet ds =
    makeDataSet(tetraCoords, tetraShape, tetraIndex, tetraConnectivity, scalar1, scalar2);
  auto scatterPlot = executeFilter(ds);

  VISKORES_TEST_ASSERT(test_equal(scatterPlot.GetNumberOfCells(), 4),
                       "Wrong number of projected triangles in the continuous scatter plot");

  // Test connectivity
  viskores::cont::CellSetSingleType<> cellSet;
  scatterPlot.GetCellSet().AsCellSet(cellSet);
  auto expectedConnectivityArray =
    viskores::cont::make_ArrayHandle<viskores::IdComponent>({ 0, 2, 4, 2, 3, 4, 3, 1, 4, 1, 0, 4 });
  testConnectivity(cellSet, expectedConnectivityArray);
}

void TestSingleTetra_projection_triangle_point0Inside()
{
  constexpr viskores::FloatDefault scalar1[4] = {
    3.0f,
    3.0f,
    4.0f,
    1.0f,

  };

  constexpr viskores::FloatDefault scalar2[4] = {
    1.0f,
    0.0f,
    2.0f,
    2.0f,
  };

  viskores::cont::DataSet ds =
    makeDataSet(tetraCoords, tetraShape, tetraIndex, tetraConnectivity, scalar1, scalar2);
  auto scatterPlot = executeFilter(ds);

  VISKORES_TEST_ASSERT(test_equal(scatterPlot.GetNumberOfCells(), 3),
                       "Wrong number of projected triangles in the continuous scatter plot");
  VISKORES_TEST_ASSERT(test_equal(scatterPlot.GetNumberOfPoints(), 4),
                       "Wrong number of projected points in the continuous scatter plot");

  // Test point positions
  auto positions = scatterPlot.GetCoordinateSystem().GetDataAsMultiplexer();
  auto positionsP = positions.ReadPortal();
  testCoordinates(positionsP, scalar1, scalar2, 3);

  // Test for triangle shapes
  viskores::cont::CellSetSingleType<> cellSet;
  scatterPlot.GetCellSet().AsCellSet(cellSet);
  testShapes(cellSet);


  // Test connectivity
  auto expectedConnectivityArray =
    viskores::cont::make_ArrayHandle<viskores::IdComponent>({ 1, 2, 0, 2, 3, 0, 3, 1, 0 });
  testConnectivity(cellSet, expectedConnectivityArray);

  // Test density values
  auto density = scatterPlot.GetField("density")
                   .GetData()
                   .AsArrayHandle<viskores::cont::ArrayHandle<viskores::FloatDefault>>()
                   .ReadPortal();
  testDensity(density, 0, 1.33333f);
}


void TestSingleTetra_projection_triangle_point1Inside()
{
  constexpr viskores::FloatDefault scalar1[4] = {
    3.0f,
    3.0f,
    4.0f,
    1.0f,

  };

  constexpr viskores::FloatDefault scalar2[4] = {
    0.0f,
    1.0f,
    2.0f,
    2.0f,
  };

  viskores::cont::DataSet ds =
    makeDataSet(tetraCoords, tetraShape, tetraIndex, tetraConnectivity, scalar1, scalar2);
  auto scatterPlot = executeFilter(ds);

  // Test connectivity
  viskores::cont::CellSetSingleType<> cellSet;
  scatterPlot.GetCellSet().AsCellSet(cellSet);
  auto expectedConnectivityArray =
    viskores::cont::make_ArrayHandle<viskores::IdComponent>({ 0, 2, 1, 2, 3, 1, 3, 0, 1 });
  testConnectivity(cellSet, expectedConnectivityArray);

  // Test density values
  auto density = scatterPlot.GetField("density")
                   .GetData()
                   .AsArrayHandle<viskores::cont::ArrayHandle<viskores::FloatDefault>>()
                   .ReadPortal();
  testDensity(density, 1, 1.33333f);
}

void TestSingleTetra_projection_triangle_point2Inside()
{
  constexpr viskores::FloatDefault scalar1[4] = {
    3.0f,
    4.0f,
    3.0f,
    1.0f,

  };

  constexpr viskores::FloatDefault scalar2[4] = {
    0.0f,
    2.0f,
    1.0f,
    2.0f,
  };

  viskores::cont::DataSet ds =
    makeDataSet(tetraCoords, tetraShape, tetraIndex, tetraConnectivity, scalar1, scalar2);
  auto scatterPlot = executeFilter(ds);

  // Test connectivity
  viskores::cont::CellSetSingleType<> cellSet;
  scatterPlot.GetCellSet().AsCellSet(cellSet);
  auto expectedConnectivityArray =
    viskores::cont::make_ArrayHandle<viskores::IdComponent>({ 0, 1, 2, 1, 3, 2, 3, 0, 2 });
  testConnectivity(cellSet, expectedConnectivityArray);

  // Test density values
  auto density = scatterPlot.GetField("density")
                   .GetData()
                   .AsArrayHandle<viskores::cont::ArrayHandle<viskores::FloatDefault>>()
                   .ReadPortal();
  testDensity(density, 2, 1.33333f);
}

void TestSingleTetra_projection_triangle_point3Inside()
{
  constexpr viskores::FloatDefault scalar1[4] = {
    3.0f,
    4.0f,
    1.0f,
    3.0f,
  };

  constexpr viskores::FloatDefault scalar2[4] = {
    0.0f,
    2.0f,
    2.0f,
    1.0f,
  };

  viskores::cont::DataSet ds =
    makeDataSet(tetraCoords, tetraShape, tetraIndex, tetraConnectivity, scalar1, scalar2);
  auto scatterPlot = executeFilter(ds);

  // Test connectivity
  viskores::cont::CellSetSingleType<> cellSet;
  scatterPlot.GetCellSet().AsCellSet(cellSet);
  auto expectedConnectivityArray =
    viskores::cont::make_ArrayHandle<viskores::IdComponent>({ 0, 1, 3, 1, 2, 3, 2, 0, 3 });
  testConnectivity(cellSet, expectedConnectivityArray);

  // Test density values
  auto density = scatterPlot.GetField("density")
                   .GetData()
                   .AsArrayHandle<viskores::cont::ArrayHandle<viskores::FloatDefault>>()
                   .ReadPortal();
  testDensity(density, 3, 1.33333f);
}

void TestNullSpatialVolume()
{
  constexpr viskores::FloatDefault scalar1[4] = {
    3.0f,
    3.0f,
    4.0f,
    1.0f,

  };

  constexpr viskores::FloatDefault scalar2[4] = {
    1.0f,
    0.0f,
    2.0f,
    2.0f,
  };

  std::vector<viskores::Vec3f> nullCoords{ viskores::Vec3f(1.0f, 1.0f, 1.0f),
                                           viskores::Vec3f(1.0f, 1.0f, 1.0f),
                                           viskores::Vec3f(1.0f, 1.0f, 1.0f),
                                           viskores::Vec3f(1.0f, 1.0f, 1.0f) };

  viskores::cont::DataSet ds =
    makeDataSet(nullCoords, tetraShape, tetraIndex, tetraConnectivity, scalar1, scalar2);
  auto scatterPlot = executeFilter(ds);

  // Test density values
  auto density = scatterPlot.GetField("density")
                   .GetData()
                   .AsArrayHandle<viskores::cont::ArrayHandle<viskores::FloatDefault>>()
                   .ReadPortal();

  VISKORES_TEST_ASSERT(viskores::IsInf(density.Get(0)),
                       "Density should be infinite for a null volume");
}

void TestNullDataSurface()
{
  constexpr viskores::FloatDefault scalar1[4] = {
    0.0f,
    1.0f,
    3.0f,
    2.0f,

  };

  constexpr viskores::FloatDefault scalar2[4] = {
    0.0f,
    1.0f,
    3.0f,
    2.0f,
  };

  viskores::cont::DataSet ds =
    makeDataSet(tetraCoords, tetraShape, tetraIndex, tetraConnectivity, scalar1, scalar2);
  auto scatterPlot = executeFilter(ds);

  auto density = scatterPlot.GetField("density")
                   .GetData()
                   .AsArrayHandle<viskores::cont::ArrayHandle<viskores::FloatDefault>>()
                   .ReadPortal();

  VISKORES_TEST_ASSERT(viskores::IsInf(density.Get(4)),
                       "Density should be infinite for a null volume");
}

void TestMultipleTetra()
{
  constexpr viskores::FloatDefault multiscalar1[12] = { 3.0f, 3.0f,  4.0f, 1.0f, 0.0f, 1.0f,
                                                        0.0f, -2.0f, 3.0f, 3.0f, 4.0f, 1.0f };

  constexpr viskores::FloatDefault multiscalar2[12] = { 1.0f, 0.0f, 2.0f, 2.0f, -1.0f, 0.0f,
                                                        2.0f, 0.0f, 1.0f, 0.0f, 2.0f,  2.0f };

  viskores::cont::DataSetBuilderExplicit dsb;
  viskores::cont::DataSet ds =
    dsb.Create(multiCoords, multiShapes, multiIndices, multiConnectivity);

  ds.AddPointField("scalar1", multiscalar1, 12);
  ds.AddPointField("scalar2", multiscalar2, 12);

  // Filtering
  auto scatterPlot = executeFilter(ds);

  VISKORES_TEST_ASSERT(test_equal(scatterPlot.GetNumberOfCells(), 10),
                       "Wrong number of projected triangles in the continuous scatter plot");
  VISKORES_TEST_ASSERT(test_equal(scatterPlot.GetNumberOfPoints(), 13),
                       "Wrong number of projected points in the continuous scatter plot");

  viskores::cont::CellSetSingleType<> cellSet;
  scatterPlot.GetCellSet().AsCellSet(cellSet);
  testShapes(cellSet);
}

void TestNonTetra()
{
  std::vector<viskores::Vec3f> wedgeCoords{
    viskores::Vec3f(0.0f, 0.0f, 0.0f), viskores::Vec3f(2.0f, 0.0f, 0.0f),
    viskores::Vec3f(2.0f, 4.0f, 0.0f), viskores::Vec3f(0.0f, 4.0f, 0.0f),
    viskores::Vec3f(1.0f, 0.0f, 3.0f), viskores::Vec3f(1.0f, 4.0f, 3.0f),
  };

  constexpr viskores::FloatDefault scalar1[6] = { 0.0f, 3.0f, 3.0f, 2.0f, 2.0f, 1.0f };

  constexpr viskores::FloatDefault scalar2[6] = { 0.0f, 1.0f, 3.0f, 2.0f, 0.0f, 1.0f };

  std::vector<viskores::UInt8> w_shape{ viskores::CELL_SHAPE_WEDGE };
  std::vector<viskores::IdComponent> w_indices{ 6 };
  std::vector<viskores::Id> w_connectivity{ 0, 1, 2, 3, 4, 5 };

  viskores::cont::DataSetBuilderExplicit dsb;
  viskores::cont::DataSet ds = dsb.Create(wedgeCoords, w_shape, w_indices, w_connectivity);

  ds.AddPointField("scalar1", scalar1, 6);
  ds.AddPointField("scalar2", scalar2, 6);

  auto scatterPlot = executeFilter(ds);

  VISKORES_TEST_ASSERT(test_equal(scatterPlot.GetNumberOfCells(), 12),
                       "Wrong number of projected triangles in the continuous scatter plot");
  VISKORES_TEST_ASSERT(test_equal(scatterPlot.GetNumberOfPoints(), 15),
                       "Wrong number of projected points in the continuous scatter plot");
}

void TestNonPointFields()
{
  constexpr viskores::FloatDefault cellField1[1] = { 1.0f };

  constexpr viskores::FloatDefault cellField2[1] = {
    0.0f,
  };
  viskores::cont::DataSetBuilderExplicit dsb;
  viskores::cont::DataSet ds = dsb.Create(tetraCoords, tetraShape, tetraIndex, tetraConnectivity);

  ds.AddCellField("scalar1", cellField1, 1);
  ds.AddCellField("scalar2", cellField2, 1);

  auto continuousSCP = viskores::filter::density_estimate::ContinuousScatterPlot();
  continuousSCP.SetActiveField(0, "scalar1", viskores::cont::Field::Association::Cells);
  continuousSCP.SetActiveField(1, "scalar2", viskores::cont::Field::Association::Cells);

  try
  {
    auto scatterPlot = continuousSCP.Execute(ds);
    VISKORES_TEST_FAIL(
      "Filter execution was not aborted after providing active fields not associated with points");
  }
  catch (const viskores::cont::ErrorFilterExecution&)
  {
    std::cout << "Execution successfully aborted" << std::endl;
  }
}

void TestDataTypes()
{
  constexpr viskores::Float32 scalar1_f32[4] = {
    -2.0f,
    0.0f,
    1.0f,
    0.0f,
  };

  constexpr viskores::Float32 scalar2_f32[4] = {
    0.0f,
    2.0f,
    0.0f,
    -1.0f,
  };

  constexpr viskores::Float64 scalar1_f64[4] = {
    -2.0,
    0.0,
    1.0,
    0.0,
  };

  constexpr viskores::Float64 scalar2_f64[4] = {
    0.0,
    2.0,
    0.0,
    -1.0,
  };

  // The filter should run whatever float precision we use for both fields
  viskores::cont::DataSet ds =
    makeDataSet(tetraCoords, tetraShape, tetraIndex, tetraConnectivity, scalar1_f32, scalar2_f32);
  auto scatterPlot = executeFilter(ds);
  auto density = scatterPlot.GetField("density")
                   .GetData()
                   .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>()
                   .ReadPortal();
  testDensity(density, 4, 0.888889f);

  viskores::cont::DataSet ds2 =
    makeDataSet(tetraCoords, tetraShape, tetraIndex, tetraConnectivity, scalar1_f32, scalar2_f64);
  auto scatterPlot2 = executeFilter(ds);
  auto density2 = scatterPlot.GetField("density")
                    .GetData()
                    .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>()
                    .ReadPortal();
  testDensity(density2, 4, 0.888889f);


  viskores::cont::DataSet ds3 =
    makeDataSet(tetraCoords, tetraShape, tetraIndex, tetraConnectivity, scalar1_f64, scalar2_f32);
  auto scatterPlot3 = executeFilter(ds);
  auto density3 = scatterPlot.GetField("density")
                    .GetData()
                    .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>()
                    .ReadPortal();
  testDensity(density3, 4, 0.888889f);


  viskores::cont::DataSet ds4 =
    makeDataSet(tetraCoords, tetraShape, tetraIndex, tetraConnectivity, scalar1_f64, scalar2_f64);
  auto scatterPlot4 = executeFilter(ds);
  auto density4 = scatterPlot.GetField("density")
                    .GetData()
                    .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>()
                    .ReadPortal();
  testDensity(density4, 4, 0.888889f);
}

void TestContinuousScatterPlot()
{
  // Projection forms 4 triangles in the data domain
  TestSingleTetraProjectionQuadConvex();
  TestSingleTetraProjectionQuadSelfIntersect();
  TestSingleTetraProjectionQuadInverseOrder();
  TestSingleTetraProjectionQuadSelfIntersectSecond();

  // 3 triangles in the data domain
  TestSingleTetra_projection_triangle_point0Inside();
  TestSingleTetra_projection_triangle_point1Inside();
  TestSingleTetra_projection_triangle_point2Inside();
  TestSingleTetra_projection_triangle_point3Inside();

  // // Edge cases
  TestNullSpatialVolume();
  TestNullDataSurface();

  TestMultipleTetra();
  TestNonTetra();
  TestNonPointFields();
  TestDataTypes();
}
}

int UnitTestContinuousScatterPlot(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestContinuousScatterPlot, argc, argv);
}
