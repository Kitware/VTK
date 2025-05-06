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

#include <viskores/source/PerlinNoise.h>

#include <viskores/filter/contour/Contour.h>

#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/MapperRayTracer.h>
#include <viskores/rendering/View3D.h>

#include <viskores/rendering/testing/RenderTest.h>
#include <viskores/rendering/testing/Testing.h>

namespace
{

void TestPerlinNoise()
{
  viskores::source::PerlinNoise noiseSource;
  noiseSource.SetCellDimensions(viskores::Id3(16));
  noiseSource.SetSeed(77698);
  viskores::cont::DataSet noise = noiseSource.Execute();

  noise.PrintSummary(std::cout);

  viskores::filter::contour::Contour contourFilter;
  contourFilter.SetIsoValues({ 0.3, 0.4, 0.5, 0.6, 0.7 });
  contourFilter.SetActiveField("perlinnoise");
  viskores::cont::DataSet contours = contourFilter.Execute(noise);

  // CUDA seems to make the contour slightly different, so relax comparison options.
  viskores::rendering::testing::RenderTestOptions options;
  options.AllowedPixelErrorRatio = 0.01f;
  options.Threshold = 0.1f;

  viskores::rendering::testing::RenderTest(
    contours, "perlinnoise", "source/perlin-noise.png", options);
}

} // anonymous namespace

int RenderTestPerlinNoise(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestPerlinNoise, argc, argv);
}
