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
#include <viskores/rendering/MapperQuad.h>
#include <viskores/rendering/Scene.h>
#include <viskores/rendering/View3D.h>
#include <viskores/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  viskores::cont::testing::MakeTestDataSet maker;

  viskores::rendering::testing::RenderTestOptions options;
  options.Mapper = viskores::rendering::testing::MapperType::Quad;
  options.AllowAnyDevice = false;
  options.ColorTable = viskores::cont::ColorTable::Preset::Inferno;

  viskores::rendering::testing::RenderTest(
    maker.Make3DRegularDataSet0(), "pointvar", "rendering/quad/regular3D.png", options);
  viskores::rendering::testing::RenderTest(
    maker.Make3DRectilinearDataSet0(), "pointvar", "rendering/quad/rectilinear3D.png", options);
  viskores::rendering::testing::RenderTest(
    maker.Make3DExplicitDataSet4(), "pointvar", "rendering/quad/explicit3D.png", options);

  //hexahedron, wedge, pyramid, tetrahedron
  viskores::rendering::testing::RenderTest(
    maker.Make3DExplicitDataSet5(), "cellvar", "rendering/quad/mixed3D.png", options);

  options.ViewDimension = 2;
  viskores::rendering::testing::RenderTest(
    maker.Make2DUniformDataSet1(), "pointvar", "rendering/quad/uniform2D.png", options);
}

} //namespace

int UnitTestMapperQuads(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RenderTests, argc, argv);
}
