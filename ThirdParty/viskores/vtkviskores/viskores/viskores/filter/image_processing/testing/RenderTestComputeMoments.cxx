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

#include <viskores/filter/image_processing/ComputeMoments.h>
#include <viskores/source/Wavelet.h>

#include <viskores/rendering/testing/RenderTest.h>
#include <viskores/rendering/testing/Testing.h>

namespace
{

void TestComputeMoments()
{
  viskores::source::Wavelet source;
  viskores::cont::DataSet data = source.Execute();

  viskores::filter::image_processing::ComputeMoments filter;
  filter.SetActiveField("RTData");
  filter.SetOrder(2);
  filter.SetRadius(2);
  viskores::cont::DataSet result = filter.Execute(data);

  viskores::rendering::testing::RenderTestOptions testOptions;
  testOptions.AllowedPixelErrorRatio = 0.001f;
  testOptions.ColorTable = viskores::cont::ColorTable("inferno");
  testOptions.EnableAnnotations = false;
  viskores::rendering::testing::RenderTest(result, "index", "filter/moments.png", testOptions);
  viskores::rendering::testing::RenderTest(result, "index0", "filter/moments0.png", testOptions);
  viskores::rendering::testing::RenderTest(result, "index12", "filter/moments12.png", testOptions);
}
} // namespace

int RenderTestComputeMoments(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestComputeMoments, argc, argv);
}
