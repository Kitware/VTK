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

#include <viskores/Math.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/filter/clean_grid/CleanGrid.h>
#include <viskores/filter/contour/Contour.h>
#include <viskores/filter/field_conversion/CellAverage.h>
#include <viskores/filter/field_transform/PointElevation.h>
#include <viskores/source/Tangle.h>

#include <viskores/io/VTKDataSetReader.h>

#include <viskores/rendering/testing/RenderTest.h>
#include <viskores/rendering/testing/Testing.h>

namespace
{
void TestContourFilterWedge()
{
  std::cout << "Generate Image for Contour filter on an unstructured grid" << std::endl;

  auto pathname = viskores::cont::testing::Testing::DataPath("unstructured/wedge_cells.vtk");
  viskores::io::VTKDataSetReader reader(pathname);
  viskores::cont::DataSet dataSet = reader.ReadDataSet();

  viskores::filter::contour::Contour contour;
  contour.SetIsoValues({ -1, 0, 1 });
  contour.SetActiveField("gyroid");
  contour.SetFieldsToPass({ "gyroid", "cellvar" });
  contour.SetMergeDuplicatePoints(true);
  auto result = contour.Execute(dataSet);

  result.PrintSummary(std::cout);

  viskores::rendering::testing::RenderTestOptions testOptions;
  viskores::rendering::testing::RenderTest(
    result, "gyroid", "filter/contour-wedge.png", testOptions);
}

void TestContourFilterUniform()
{
  std::cout << "Generate Image for Contour filter on a uniform grid" << std::endl;

  viskores::cont::testing::MakeTestDataSet maker;
  viskores::cont::DataSet inputData = maker.Make3DUniformDataSet0();
  std::string fieldName = "pointvar";
  VISKORES_TEST_ASSERT(inputData.HasField(fieldName));

  viskores::filter::contour::Contour contour;
  contour.SetGenerateNormals(false);
  contour.SetMergeDuplicatePoints(true);
  contour.SetIsoValues({ 50, 100, 150 });
  contour.SetActiveField(fieldName);
  viskores::cont::DataSet result = contour.Execute(inputData);

  result.PrintSummary(std::cout);

  //Y axis Flying Edge algorithm has subtle differences at a couple of boundaries
  viskores::rendering::testing::RenderTestOptions testOptions;
  viskores::rendering::testing::RenderTest(
    result, "pointvar", "filter/contour-uniform.png", testOptions);

  std::cout << "Generate image for contour filter on a uniform grid with a cell field" << std::endl;
  inputData = maker.Make3DUniformDataSet2();
  VISKORES_TEST_ASSERT(inputData.HasField(fieldName));

  std::string cellFieldName = "elevation";
  viskores::filter::field_transform::PointElevation elevation;
  viskores::Bounds bounds = inputData.GetCoordinateSystem().GetBounds();
  elevation.SetLowPoint(bounds.MinCorner());
  elevation.SetHighPoint(bounds.MaxCorner());
  elevation.SetRange(0, 1);
  elevation.SetOutputFieldName(cellFieldName);
  elevation.SetUseCoordinateSystemAsField(true);
  inputData = elevation.Execute(inputData);

  viskores::filter::field_conversion::CellAverage point2cell;
  point2cell.SetActiveField(cellFieldName);
  point2cell.SetFieldsToPass(fieldName);
  inputData = point2cell.Execute(inputData);

  VISKORES_TEST_ASSERT(inputData.HasPointField(fieldName));
  VISKORES_TEST_ASSERT(inputData.HasCellField(cellFieldName));

  contour.SetIsoValues({ 80 });
  result = contour.Execute(inputData);

  result.PrintSummary(std::cout);

  viskores::rendering::testing::RenderTest(
    result, cellFieldName, "filter/contour-uniform-cellfield.png", testOptions);
}

void TestContourFilterUniformBoundaries()
{
  std::cout << "Generate Image for Contour filter on a uniform grid that goes through boundaries"
            << std::endl;
  // There was a bug in flying edges that did not identify boundaries when the dimension
  // sizes were not the same.

  viskores::cont::DataSetBuilderUniform dsb;
  viskores::cont::DataSet dataSet = dsb.Create(
    { 9, 5, 3 }, viskores::Vec3f_64{ 0.0, 0.0, 0.0 }, viskores::Vec3f_64{ 0.125, 0.25, 0.5 });

  std::string fieldName = "pointvar";
  viskores::filter::field_transform::PointElevation elevation;
  elevation.SetLowPoint(1.0, 0.0, 0.0);
  elevation.SetHighPoint(0.0, 1.0, 1.0);
  elevation.SetOutputFieldName(fieldName);
  elevation.SetUseCoordinateSystemAsField(true);
  dataSet = elevation.Execute(dataSet);

  viskores::filter::contour::Contour contour;
  contour.SetGenerateNormals(true);
  contour.SetMergeDuplicatePoints(true);
  contour.SetIsoValues({ 0.25, 0.5, 0.75 });
  contour.SetActiveField(fieldName);
  viskores::cont::DataSet result = contour.Execute(dataSet);

  result.PrintSummary(std::cout);

  //Y axis Flying Edge algorithm has subtle differences at a couple of boundaries
  viskores::rendering::testing::RenderTestOptions testOptions;
  viskores::rendering::testing::RenderTest(
    result, fieldName, "filter/contour-uniform-boundaries.png", testOptions);
}

void TestContourFilterTangle()
{
  std::cout << "Generate Image for Contour filter on a uniform tangle grid" << std::endl;

  viskores::source::Tangle tangle;
  tangle.SetCellDimensions({ 4, 4, 4 });
  viskores::cont::DataSet dataSet = tangle.Execute();

  viskores::filter::contour::Contour contour;
  contour.SetGenerateNormals(true);
  contour.SetIsoValue(0, 1);
  contour.SetActiveField("tangle");
  contour.SetFieldsToPass("tangle");
  auto result = contour.Execute(dataSet);

  result.PrintSummary(std::cout);

  //Y axis Flying Edge algorithm has subtle differences at a couple of boundaries
  viskores::rendering::testing::RenderTestOptions testOptions;
  testOptions.Colors = { { 0.20f, 0.80f, 0.20f } };
  testOptions.EnableAnnotations = false;
  viskores::rendering::testing::RenderTest(
    result, "tangle", "filter/contour-tangle.png", testOptions);
}

void TestContourFilterPoly()
{
  std::cout << "Generate Image for Contour filter on polygons" << std::endl;

  auto pathname = viskores::cont::testing::Testing::DataPath("unstructured/poly_contour_cases.vtk");
  viskores::io::VTKDataSetReader reader(pathname);
  viskores::cont::DataSet dataSet = reader.ReadDataSet();

  viskores::filter::contour::Contour contour;
  contour.SetIsoValues({ -0.20, -0.12, -0.04, 0.04, 0.12, 0.20 });
  contour.SetActiveField("PerlinNoise");
  contour.SetMergeDuplicatePoints(true);
  auto result = contour.Execute(dataSet);

  result.PrintSummary(std::cout);

  viskores::rendering::testing::RenderTestOptions testOptions;
  testOptions.Mapper = viskores::rendering::testing::MapperType::Cylinder;
  testOptions.Radius = 0.01f;
  viskores::rendering::testing::RenderTest(
    result, "PerlinNoise", "filter/contour-poly.png", testOptions);
}

void TestContourFilter()
{
  TestContourFilterUniform();
  TestContourFilterUniformBoundaries();
  TestContourFilterTangle();
  TestContourFilterWedge();
  TestContourFilterPoly();
}
} // namespace

int RenderTestContourFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestContourFilter, argc, argv);
}
