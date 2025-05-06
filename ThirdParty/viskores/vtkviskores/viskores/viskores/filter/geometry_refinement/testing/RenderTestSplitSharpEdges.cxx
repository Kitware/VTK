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
#include <viskores/cont/testing/Testing.h>
#include <viskores/io/VTKDataSetReader.h>

#include <viskores/filter/geometry_refinement/SplitSharpEdges.h>
#include <viskores/filter/vector_analysis/SurfaceNormals.h>

#include <viskores/rendering/testing/RenderTest.h>
#include <viskores/rendering/testing/Testing.h>

namespace
{
void TestSplitSharpEdges()
{
  std::cout << "Generate Image for SplitSharpEdges filter" << std::endl;

  auto pathname =
    viskores::cont::testing::Testing::DataPath("unstructured/SplitSharpEdgesTestDataSet.vtk");
  viskores::io::VTKDataSetReader reader(pathname);
  auto dataSet = reader.ReadDataSet();

  viskores::filter::geometry_refinement::SplitSharpEdges splitSharpEdges;
  splitSharpEdges.SetFeatureAngle(89.0);
  splitSharpEdges.SetActiveField("Normals", viskores::cont::Field::Association::Cells);

  auto result = splitSharpEdges.Execute(dataSet);
  result.PrintSummary(std::cout);

  viskores::rendering::testing::RenderTestOptions testOptions;
  testOptions.ColorTable = viskores::cont::ColorTable::Preset::Inferno;
  testOptions.EnableAnnotations = false;
  viskores::rendering::testing::RenderTest(
    result, "pointvar", "filter/split-sharp-edges.png", testOptions);
}
} // namespace

int RenderTestSplitSharpEdges(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestSplitSharpEdges, argc, argv);
}
