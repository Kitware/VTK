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
#include <viskores/cont/testing/Testing.h>
#include <viskores/io/VTKDataSetReader.h>

#include <viskores/filter/vector_analysis/SurfaceNormals.h>

#include <viskores/rendering/testing/RenderTest.h>
#include <viskores/rendering/testing/Testing.h>

namespace
{
void TestSurfaceNormals()
{
  std::cout << "Generate Image for SurfaceNormals filter" << std::endl;

  // NOTE: This dataset stores a shape value of 7 for polygons.  The
  // VTKDataSetReader currently converts all polygons with 4 verticies to
  // quads (shape 9).
  auto pathname =
    viskores::cont::testing::Testing::DataPath("unstructured/SurfaceNormalsTestDataSet.vtk");
  viskores::io::VTKDataSetReader reader(pathname);
  auto dataSet = reader.ReadDataSet();

  viskores::filter::vector_analysis::SurfaceNormals surfaceNormals;
  surfaceNormals.SetGeneratePointNormals(true);
  surfaceNormals.SetAutoOrientNormals(true);

  auto result = surfaceNormals.Execute(dataSet);
  result.PrintSummary(std::cout);

  viskores::rendering::testing::RenderTestOptions testOptions;
  testOptions.ColorTable = viskores::cont::ColorTable::Preset::Inferno;
  testOptions.EnableAnnotations = false;
  viskores::rendering::testing::RenderTest(
    result, "pointvar", "filter/surface-normals.png", testOptions);
}
} // namespace

int RenderTestSurfaceNormals(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestSurfaceNormals, argc, argv);
}
