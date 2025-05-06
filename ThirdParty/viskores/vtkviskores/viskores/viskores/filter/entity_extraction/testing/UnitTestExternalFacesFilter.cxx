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
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/filter/clean_grid/CleanGrid.h>
#include <viskores/filter/entity_extraction/ExternalFaces.h>

using viskores::cont::testing::MakeTestDataSet;

namespace
{

// convert a 5x5x5 uniform grid to unstructured grid
viskores::cont::DataSet MakeDataTestSet1()
{
  viskores::cont::DataSet ds = MakeTestDataSet().Make3DUniformDataSet1();

  viskores::filter::clean_grid::CleanGrid clean;
  clean.SetCompactPointFields(false);
  clean.SetMergePoints(false);
  return clean.Execute(ds);
}

viskores::cont::DataSet MakeDataTestSet2()
{
  return MakeTestDataSet().Make3DExplicitDataSet5();
}

viskores::cont::DataSet MakeDataTestSet3()
{
  return MakeTestDataSet().Make3DUniformDataSet1();
}

viskores::cont::DataSet MakeDataTestSet4()
{
  return MakeTestDataSet().Make3DRectilinearDataSet0();
}

viskores::cont::DataSet MakeDataTestSet5()
{
  return MakeTestDataSet().Make3DExplicitDataSet6();
}

viskores::cont::DataSet MakeUniformDataTestSet()
{
  return MakeTestDataSet().Make3DUniformDataSet1();
}

viskores::cont::DataSet MakeCurvilinearDataTestSet()
{
  viskores::cont::DataSet data = MakeUniformDataTestSet();
  viskores::cont::ArrayHandle<viskores::Vec3f> coords;
  viskores::cont::CoordinateSystem oldCoords = data.GetCoordinateSystem();
  viskores::cont::ArrayCopy(oldCoords.GetData(), coords);
  data.AddCoordinateSystem(oldCoords.GetName(), coords);
  return data;
}

void TestExternalFacesExplicitGrid(const viskores::cont::DataSet& ds,
                                   bool compactPoints,
                                   viskores::Id numExpectedExtFaces,
                                   viskores::Id numExpectedPoints = 0,
                                   bool passPolyData = true)
{
  //Run the External Faces filter
  viskores::filter::entity_extraction::ExternalFaces externalFaces;
  externalFaces.SetCompactPoints(compactPoints);
  externalFaces.SetPassPolyData(passPolyData);
  viskores::cont::DataSet resultds = externalFaces.Execute(ds);

  // verify cellset
  const viskores::Id numOutputExtFaces = resultds.GetNumberOfCells();
  VISKORES_TEST_ASSERT(numOutputExtFaces == numExpectedExtFaces,
                       "Number of External Faces mismatch");

  // verify fields
  VISKORES_TEST_ASSERT(resultds.HasField("pointvar"), "Point field not mapped successfully");
  VISKORES_TEST_ASSERT(resultds.HasField("cellvar"), "Cell field not mapped successfully");

  // verify CompactPoints
  if (compactPoints)
  {
    viskores::Id numOutputPoints = resultds.GetCoordinateSystem(0).GetNumberOfPoints();
    VISKORES_TEST_ASSERT(numOutputPoints == numExpectedPoints,
                         "Incorrect number of points after compacting");
  }
}

void TestWithHexahedraMesh()
{
  std::cout << "Testing with Hexahedra mesh\n";
  viskores::cont::DataSet ds = MakeDataTestSet1();
  std::cout << "Compact Points Off\n";
  TestExternalFacesExplicitGrid(ds, false, 96); // 4x4 * 6 = 96
  std::cout << "Compact Points On\n";
  TestExternalFacesExplicitGrid(ds, true, 96, 98); // 5x5x5 - 3x3x3 = 98
}

void TestWithHeterogeneousMesh()
{
  std::cout << "Testing with Heterogeneous mesh\n";
  viskores::cont::DataSet ds = MakeDataTestSet2();
  std::cout << "Compact Points Off\n";
  TestExternalFacesExplicitGrid(ds, false, 12);
  std::cout << "Compact Points On\n";
  TestExternalFacesExplicitGrid(ds, true, 12, 11);
}

void TestWithUniformMesh()
{
  std::cout << "Testing with Uniform mesh\n";
  viskores::cont::DataSet ds = MakeDataTestSet3();
  std::cout << "Compact Points Off\n";
  TestExternalFacesExplicitGrid(ds, false, 16 * 6);
  std::cout << "Compact Points On\n";
  TestExternalFacesExplicitGrid(ds, true, 16 * 6, 98);
}

void TestWithRectilinearMesh()
{
  std::cout << "Testing with Rectilinear mesh\n";
  viskores::cont::DataSet ds = MakeDataTestSet4();
  std::cout << "Compact Points Off\n";
  TestExternalFacesExplicitGrid(ds, false, 16);
  std::cout << "Compact Points On\n";
  TestExternalFacesExplicitGrid(ds, true, 16, 18);
}

void TestWithMixed2Dand3DMesh()
{
  std::cout << "Testing with mixed poly data and 3D mesh\n";
  viskores::cont::DataSet ds = MakeDataTestSet5();
  std::cout << "Compact Points Off, Pass Poly Data On\n";
  TestExternalFacesExplicitGrid(ds, false, 12);
  std::cout << "Compact Points On, Pass Poly Data On\n";
  TestExternalFacesExplicitGrid(ds, true, 12, 8);
  std::cout << "Compact Points Off, Pass Poly Data Off\n";
  TestExternalFacesExplicitGrid(ds, false, 6, 8, false);
  std::cout << "Compact Points On, Pass Poly Data Off\n";
  TestExternalFacesExplicitGrid(ds, true, 6, 5, false);
}

void TestExternalFacesStructuredGrid(const viskores::cont::DataSet& ds, bool compactPoints)
{
  // Get the dimensions of the grid.
  viskores::cont::CellSetStructured<3> cellSet;
  ds.GetCellSet().AsCellSet(cellSet);
  viskores::Id3 pointDims = cellSet.GetPointDimensions();
  viskores::Id3 cellDims = cellSet.GetCellDimensions();

  //Run the External Faces filter
  viskores::filter::entity_extraction::ExternalFaces externalFaces;
  externalFaces.SetCompactPoints(compactPoints);
  viskores::cont::DataSet resultds = externalFaces.Execute(ds);

  // verify cellset
  viskores::Id numExpectedExtFaces = ((2 * cellDims[0] * cellDims[1]) + // x-y faces
                                      (2 * cellDims[0] * cellDims[2]) + // x-z faces
                                      (2 * cellDims[1] * cellDims[2])); // y-z faces
  const viskores::Id numOutputExtFaces = resultds.GetNumberOfCells();
  VISKORES_TEST_ASSERT(numOutputExtFaces == numExpectedExtFaces,
                       "Number of External Faces mismatch");

  // verify fields
  VISKORES_TEST_ASSERT(resultds.HasField("pointvar"), "Point field not mapped successfully");
  VISKORES_TEST_ASSERT(resultds.HasField("cellvar"), "Cell field not mapped successfully");

  // verify CompactPoints
  if (compactPoints)
  {
    viskores::Id numExpectedPoints = ((2 * pointDims[0] * pointDims[1])   // x-y faces
                                      + (2 * pointDims[0] * pointDims[2]) // x-z faces
                                      + (2 * pointDims[1] * pointDims[2]) // y-z faces
                                      - (4 * pointDims[0])                // overcounted x edges
                                      - (4 * pointDims[1])                // overcounted y edges
                                      - (4 * pointDims[2])                // overcounted z edges
                                      + 8);                               // undercounted corners
    viskores::Id numOutputPoints = resultds.GetNumberOfPoints();
    VISKORES_TEST_ASSERT(numOutputPoints == numExpectedPoints);
  }
  else
  {
    VISKORES_TEST_ASSERT(resultds.GetNumberOfPoints() == ds.GetNumberOfPoints());
  }
}

void TestWithUniformGrid()
{
  std::cout << "Testing with uniform grid\n";
  viskores::cont::DataSet ds = MakeUniformDataTestSet();
  std::cout << "Compact Points Off\n";
  TestExternalFacesStructuredGrid(ds, false);
  std::cout << "Compact Points On\n";
  TestExternalFacesStructuredGrid(ds, true);
}

void TestWithCurvilinearGrid()
{
  std::cout << "Testing with curvilinear grid\n";
  viskores::cont::DataSet ds = MakeCurvilinearDataTestSet();
  std::cout << "Compact Points Off\n";
  TestExternalFacesStructuredGrid(ds, false);
  std::cout << "Compact Points On\n";
  TestExternalFacesStructuredGrid(ds, true);
}

void TestExternalFacesFilter()
{
  TestWithHeterogeneousMesh();
  TestWithHexahedraMesh();
  TestWithUniformMesh();
  TestWithRectilinearMesh();
  TestWithMixed2Dand3DMesh();
  TestWithUniformGrid();
  TestWithCurvilinearGrid();
}

} // anonymous namespace

int UnitTestExternalFacesFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestExternalFacesFilter, argc, argv);
}
