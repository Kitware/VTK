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
#include <viskores/rendering/Scene.h>
#include <viskores/rendering/View3D.h>
#include <viskores/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  viskores::cont::testing::MakeTestDataSet maker;

  viskores::rendering::testing::RenderTestOptions options;
  options.Mapper = viskores::rendering::testing::MapperType::GlyphScalar;
  options.AllowAnyDevice = false;
  options.ColorTable = viskores::cont::ColorTable::Preset::Inferno;

  options.GlyphType = viskores::rendering::GlyphType::Cube;
  viskores::rendering::testing::RenderTest(
    maker.Make3DUniformDataSet1(), "pointvar", "rendering/glyph_scalar/regular3D.png", options);

  options.UseVariableRadius = true;
  options.RadiusDelta = 4.0f;
  options.Radius = 0.25f;
  viskores::rendering::testing::RenderTest(maker.Make3DUniformDataSet1(),
                                           "pointvar",
                                           "rendering/glyph_scalar/variable_regular3D.png",
                                           options);

  options.GlyphType = viskores::rendering::GlyphType::Sphere;
  viskores::rendering::testing::RenderTest(maker.Make3DUniformDataSet3({ 7 }),
                                           "pointvar",
                                           "rendering/glyph_scalar/variable_spheres_regular3D.png",
                                           options);

  options.GlyphType = viskores::rendering::GlyphType::Axes;
  viskores::rendering::testing::RenderTest(maker.Make3DUniformDataSet3({ 7 }),
                                           "pointvar",
                                           "rendering/glyph_scalar/variable_axes_regular3D.png",
                                           options);

  options.GlyphType = viskores::rendering::GlyphType::Quad;
  options.Radius = 5.0f;
  options.RadiusDelta = 0.75f;
  viskores::rendering::testing::RenderTest(maker.Make3DUniformDataSet3({ 7 }),
                                           "pointvar",
                                           "rendering/glyph_scalar/variable_quads_regular3D.png",
                                           options);

  // restore defaults
  options.RadiusDelta = 0.5f;
  options.UseVariableRadius = false;
  options.GlyphType = viskores::rendering::GlyphType::Cube;

  options.RenderCells = true;
  options.Radius = 1.f;
  viskores::rendering::testing::RenderTest(
    maker.Make3DExplicitDataSet7(), "cellvar", "rendering/glyph_scalar/cells.png", options);
}

} //namespace

int UnitTestMapperGlyphScalar(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RenderTests, argc, argv);
}
