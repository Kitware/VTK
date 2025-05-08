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
#include <viskores/Particle.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/io/VTKDataSetReader.h>

#include <viskores/filter/flow/Streamline.h>
#include <viskores/filter/geometry_refinement/Tube.h>

#include <viskores/rendering/testing/RenderTest.h>
#include <viskores/rendering/testing/Testing.h>

namespace
{
void TestStreamline()
{
  std::cout << "Generate Image for Streamline filter" << std::endl;

  auto pathname = viskores::cont::testing::Testing::DataPath("uniform/StreamlineTestDataSet.vtk");
  viskores::io::VTKDataSetReader reader(pathname);
  viskores::cont::DataSet dataSet = reader.ReadDataSet();
  viskores::cont::ArrayHandle<viskores::Particle> seedArray =
    viskores::cont::make_ArrayHandle({ viskores::Particle(viskores::Vec3f(.2f, 1.0f, .2f), 0),
                                       viskores::Particle(viskores::Vec3f(.2f, 2.0f, .2f), 1),
                                       viskores::Particle(viskores::Vec3f(.2f, 3.0f, .2f), 2) });

  viskores::filter::flow::Streamline streamline;
  streamline.SetStepSize(0.1f);
  streamline.SetNumberOfSteps(20);
  streamline.SetSeeds(seedArray);
  streamline.SetActiveField("vector");
  auto result = streamline.Execute(dataSet);

  // Some sort of color map is needed when rendering the coordinates of a dataset
  // so create a zeroed array for the coordinates.
  std::vector<viskores::FloatDefault> colorMap(
    static_cast<std::vector<viskores::FloatDefault>::size_type>(
      result.GetCoordinateSystem().GetNumberOfPoints()));
  for (std::vector<viskores::FloatDefault>::size_type i = 0; i < colorMap.size(); i++)
  {
    colorMap[i] = static_cast<viskores::FloatDefault>(i);
  }
  result.AddPointField("pointvar", colorMap);

  // The streamline by itself doesn't generate renderable geometry, so surround the
  // streamlines in tubes.
  viskores::filter::geometry_refinement::Tube tube;
  tube.SetCapping(true);
  tube.SetNumberOfSides(3);
  tube.SetRadius(static_cast<viskores::FloatDefault>(0.2));
  result = tube.Execute(result);
  result.PrintSummary(std::cout);

  viskores::rendering::testing::RenderTestOptions testOptions;
  testOptions.ColorTable = viskores::cont::ColorTable::Preset::Inferno;
  testOptions.EnableAnnotations = false;
  viskores::rendering::testing::RenderTest(
    result, "pointvar", "filter/streamline.png", testOptions);
}
} // namespace

int RenderTestStreamline(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestStreamline, argc, argv);
}
