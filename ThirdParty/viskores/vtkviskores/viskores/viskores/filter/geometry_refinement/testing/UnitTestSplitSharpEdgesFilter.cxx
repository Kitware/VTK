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
#include <viskores/filter/contour/Contour.h>
#include <viskores/filter/field_conversion/CellAverage.h>
#include <viskores/filter/geometry_refinement/SplitSharpEdges.h>
#include <viskores/filter/vector_analysis/SurfaceNormals.h>

#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/source/Wavelet.h>

namespace
{

using NormalsArrayHandle = viskores::cont::ArrayHandle<viskores::Vec3f>;

const viskores::Vec3f expectedCoords[24] = {
  { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 1.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0 },
  { 1.0, 1.0, 0.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 },
  { 1.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 1.0, 0.0, 1.0 }, { 1.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 },
  { 0.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 1.0, 1.0, 0.0 }, { 1.0, 1.0, 0.0 },
  { 1.0, 1.0, 1.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 1.0, 1.0 }, { 0.0, 1.0, 1.0 }
};

const std::vector<viskores::Id> expectedConnectivityArray91{ 0, 1, 5, 4, 1, 2, 6, 5, 2, 3, 7, 6,
                                                             3, 0, 4, 7, 4, 5, 6, 7, 0, 3, 2, 1 };
const std::vector<viskores::FloatDefault> expectedPointvar{
  10.1f, 20.1f, 30.2f, 40.2f, 50.3f, 60.3f, 70.3f, 80.3f, 10.1f, 10.1f, 20.1f, 20.1f,
  30.2f, 30.2f, 40.2f, 40.2f, 50.3f, 50.3f, 60.3f, 60.3f, 70.3f, 70.3f, 80.3f, 80.3f
};

viskores::cont::DataSet Make3DExplicitSimpleCube()
{
  viskores::cont::DataSet dataSet;
  viskores::cont::DataSetBuilderExplicit dsb;

  const int nVerts = 8;
  const int nCells = 6;
  using CoordType = viskores::Vec3f;
  std::vector<CoordType> coords = {
    CoordType(0, 0, 0), // 0
    CoordType(1, 0, 0), // 1
    CoordType(1, 0, 1), // 2
    CoordType(0, 0, 1), // 3
    CoordType(0, 1, 0), // 4
    CoordType(1, 1, 0), // 5
    CoordType(1, 1, 1), // 6
    CoordType(0, 1, 1)  // 7
  };

  //Connectivity
  std::vector<viskores::UInt8> shapes;
  std::vector<viskores::IdComponent> numIndices;
  for (size_t i = 0; i < 6; i++)
  {
    shapes.push_back(viskores::CELL_SHAPE_QUAD);
    numIndices.push_back(4);
  }


  std::vector<viskores::Id> conn;
  // Down face
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(5);
  conn.push_back(4);
  // Right face
  conn.push_back(1);
  conn.push_back(2);
  conn.push_back(6);
  conn.push_back(5);
  // Top face
  conn.push_back(2);
  conn.push_back(3);
  conn.push_back(7);
  conn.push_back(6);
  // Left face
  conn.push_back(3);
  conn.push_back(0);
  conn.push_back(4);
  conn.push_back(7);
  // Front face
  conn.push_back(4);
  conn.push_back(5);
  conn.push_back(6);
  conn.push_back(7);
  // Back face
  conn.push_back(0);
  conn.push_back(3);
  conn.push_back(2);
  conn.push_back(1);

  //Create the dataset.
  dataSet = dsb.Create(coords, shapes, numIndices, conn, "coordinates");

  viskores::FloatDefault vars[nVerts] = { 10.1f, 20.1f, 30.2f, 40.2f, 50.3f, 60.3f, 70.3f, 80.3f };
  viskores::FloatDefault cellvar[nCells] = { 100.1f, 200.2f, 300.3f, 400.4f, 500.5f, 600.6f };

  dataSet.AddPointField("pointvar", vars, nVerts);
  dataSet.AddCellField("cellvar", cellvar, nCells);

  return dataSet;
}

viskores::cont::DataSet Make3DWavelet()
{

  viskores::source::Wavelet wavelet;
  wavelet.SetExtent({ -25 }, { 25 });
  wavelet.SetFrequency({ 60, 30, 40 });
  wavelet.SetMagnitude({ 5 });

  viskores::cont::DataSet result = wavelet.Execute();
  return result;
}


void TestSplitSharpEdgesFilterSplitEveryEdge(
  viskores::cont::DataSet& simpleCubeWithSN,
  viskores::filter::geometry_refinement::SplitSharpEdges& splitSharpEdgesFilter)
{
  // Split every edge
  viskores::FloatDefault featureAngle = 89.0;
  splitSharpEdgesFilter.SetFeatureAngle(featureAngle);
  splitSharpEdgesFilter.SetActiveField("Normals", viskores::cont::Field::Association::Cells);
  viskores::cont::DataSet result = splitSharpEdgesFilter.Execute(simpleCubeWithSN);

  auto newCoords = result.GetCoordinateSystem().GetDataAsMultiplexer();
  auto newCoordsP = newCoords.ReadPortal();
  viskores::cont::ArrayHandle<viskores::FloatDefault> newPointvarField;
  result.GetField("pointvar").GetData().AsArrayHandle(newPointvarField);

  for (viskores::IdComponent i = 0; i < newCoords.GetNumberOfValues(); i++)
  {
    VISKORES_TEST_ASSERT(test_equal(newCoordsP.Get(i)[0], expectedCoords[i][0]),
                         "result value does not match expected value");
    VISKORES_TEST_ASSERT(test_equal(newCoordsP.Get(i)[1], expectedCoords[i][1]),
                         "result value does not match expected value");
    VISKORES_TEST_ASSERT(test_equal(newCoordsP.Get(i)[2], expectedCoords[i][2]),
                         "result value does not match expected value");
  }

  auto newPointvarFieldPortal = newPointvarField.ReadPortal();
  for (viskores::IdComponent i = 0; i < newPointvarField.GetNumberOfValues(); i++)
  {
    VISKORES_TEST_ASSERT(test_equal(newPointvarFieldPortal.Get(static_cast<viskores::Id>(i)),
                                    expectedPointvar[static_cast<unsigned long>(i)]),
                         "point field array result does not match expected value");
  }
}

void TestSplitSharpEdgesFilterNoSplit(
  viskores::cont::DataSet& simpleCubeWithSN,
  viskores::filter::geometry_refinement::SplitSharpEdges& splitSharpEdgesFilter)
{
  // Do nothing
  viskores::FloatDefault featureAngle = 91.0;
  splitSharpEdgesFilter.SetFeatureAngle(featureAngle);
  splitSharpEdgesFilter.SetActiveField("Normals", viskores::cont::Field::Association::Cells);
  viskores::cont::DataSet result = splitSharpEdgesFilter.Execute(simpleCubeWithSN);

  auto newCoords = result.GetCoordinateSystem().GetDataAsMultiplexer();
  viskores::cont::CellSetExplicit<> newCellset =
    result.GetCellSet().AsCellSet<viskores::cont::CellSetExplicit<>>();
  auto newCoordsP = newCoords.ReadPortal();
  viskores::cont::ArrayHandle<viskores::FloatDefault> newPointvarField;
  result.GetField("pointvar").GetData().AsArrayHandle(newPointvarField);

  for (viskores::IdComponent i = 0; i < newCoords.GetNumberOfValues(); i++)
  {
    VISKORES_TEST_ASSERT(test_equal(newCoordsP.Get(i)[0], expectedCoords[i][0]),
                         "result value does not match expected value");
    VISKORES_TEST_ASSERT(test_equal(newCoordsP.Get(i)[1], expectedCoords[i][1]),
                         "result value does not match expected value");
    VISKORES_TEST_ASSERT(test_equal(newCoordsP.Get(i)[2], expectedCoords[i][2]),
                         "result value does not match expected value");
  }

  const auto& connectivityArray = newCellset.GetConnectivityArray(
    viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
  auto connectivityArrayPortal = connectivityArray.ReadPortal();
  for (viskores::IdComponent i = 0; i < connectivityArray.GetNumberOfValues(); i++)
  {
    VISKORES_TEST_ASSERT(connectivityArrayPortal.Get(static_cast<viskores::Id>(i)) ==
                           expectedConnectivityArray91[static_cast<unsigned long>(i)],
                         "connectivity array result does not match expected value");
  }

  auto newPointvarFieldPortal = newPointvarField.ReadPortal();
  for (viskores::IdComponent i = 0; i < newPointvarField.GetNumberOfValues(); i++)
  {
    VISKORES_TEST_ASSERT(test_equal(newPointvarFieldPortal.Get(static_cast<viskores::Id>(i)),
                                    expectedPointvar[static_cast<unsigned long>(i)]),
                         "point field array result does not match expected value");
  }
}

void TestWithExplicitData()
{
  viskores::cont::DataSet simpleCube = Make3DExplicitSimpleCube();

  // Generate surface normal field
  viskores::filter::vector_analysis::SurfaceNormals surfaceNormalsFilter;
  surfaceNormalsFilter.SetGenerateCellNormals(true);
  viskores::cont::DataSet simpleCubeWithSN = surfaceNormalsFilter.Execute(simpleCube);
  VISKORES_TEST_ASSERT(simpleCubeWithSN.HasCellField("Normals"), "Cell normals missing.");
  VISKORES_TEST_ASSERT(simpleCubeWithSN.HasPointField("pointvar"), "point field pointvar missing.");


  viskores::filter::geometry_refinement::SplitSharpEdges splitSharpEdgesFilter;

  TestSplitSharpEdgesFilterSplitEveryEdge(simpleCubeWithSN, splitSharpEdgesFilter);
  TestSplitSharpEdgesFilterNoSplit(simpleCubeWithSN, splitSharpEdgesFilter);
}


void TestWithStructuredData()
{
  // Generate a wavelet:
  viskores::cont::DataSet dataSet = Make3DWavelet();

  // Cut a contour:
  viskores::filter::contour::Contour contour;
  contour.SetActiveField("RTData", viskores::cont::Field::Association::Points);
  contour.SetNumberOfIsoValues(1);
  contour.SetIsoValue(192);
  contour.SetMergeDuplicatePoints(true);
  contour.SetGenerateNormals(true);
  contour.SetComputeFastNormals(true);
  contour.SetNormalArrayName("normals");
  dataSet = contour.Execute(dataSet);

  // Compute cell normals:
  viskores::filter::field_conversion::CellAverage cellNormals;
  cellNormals.SetActiveField("normals", viskores::cont::Field::Association::Points);
  dataSet = cellNormals.Execute(dataSet);

  // Split sharp edges:
  std::cout << dataSet.GetNumberOfCells() << std::endl;
  std::cout << dataSet.GetNumberOfPoints() << std::endl;
  viskores::filter::geometry_refinement::SplitSharpEdges split;
  split.SetActiveField("normals", viskores::cont::Field::Association::Cells);
  dataSet = split.Execute(dataSet);
}


void TestSplitSharpEdgesFilter()
{
  TestWithExplicitData();
  TestWithStructuredData();
}

} // anonymous namespace

int UnitTestSplitSharpEdgesFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestSplitSharpEdgesFilter, argc, argv);
}
