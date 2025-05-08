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

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/rendering/Actor.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/GlyphType.h>
#include <viskores/rendering/MapperPoint.h>
#include <viskores/rendering/Scene.h>
#include <viskores/rendering/View3D.h>
#include <viskores/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  viskores::cont::testing::MakeTestDataSet maker;

  viskores::rendering::testing::RenderTestOptions options;
  options.Mapper = viskores::rendering::testing::MapperType::GlyphVector;
  options.AllowAnyDevice = false;
  options.ColorTable = viskores::cont::ColorTable::Preset::Inferno;
  options.GlyphType = viskores::rendering::GlyphType::Arrow;
  options.UseVariableRadius = true;
  options.RadiusDelta = 4.0f;
  options.Radius = 0.02f;

  viskores::rendering::testing::RenderTest(maker.Make3DExplicitDataSetCowNose(),
                                           "point_vectors",
                                           "rendering/glyph_vector/points_arrows_cownose.png",
                                           options);
}

} //namespace

int UnitTestMapperGlyphVector(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RenderTests, argc, argv);
}
