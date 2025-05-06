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

#include <viskores/rendering/Actor.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/MapperConnectivity.h>
#include <viskores/rendering/MapperRayTracer.h>
#include <viskores/rendering/MapperVolume.h>
#include <viskores/rendering/Scene.h>
#include <viskores/rendering/View3D.h>
#include <viskores/rendering/testing/RenderTest.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

// Multi-mapper rendering is something of a hack right now. A view only supports one mapper
// at a time, so to use different mapper types you have to render the mappers yourself.
template <typename MapperType1, typename MapperType2>
void MultiMapperRender(const viskores::cont::DataSet& ds1,
                       const viskores::cont::DataSet& ds2,
                       const std::string& fieldNm,
                       const viskores::cont::ColorTable& colorTable1,
                       const viskores::cont::ColorTable& colorTable2,
                       const std::string& outputFile)
{
  MapperType1 mapper1;
  MapperType2 mapper2;

  viskores::rendering::CanvasRayTracer canvas(300, 300);
  canvas.SetBackgroundColor(viskores::rendering::Color(0.8f, 0.8f, 0.8f, 1.0f));
  canvas.Clear();

  viskores::Bounds totalBounds =
    ds1.GetCoordinateSystem().GetBounds() + ds2.GetCoordinateSystem().GetBounds();
  viskores::rendering::Camera camera;
  camera.ResetToBounds(totalBounds);
  camera.Azimuth(45.0f);
  camera.Elevation(45.0f);

  mapper1.SetCanvas(&canvas);
  mapper1.SetActiveColorTable(colorTable1);
  mapper1.SetCompositeBackground(false);

  mapper2.SetCanvas(&canvas);
  mapper2.SetActiveColorTable(colorTable2);

  const viskores::cont::Field field1 = ds1.GetField(fieldNm);
  viskores::Range range1;
  field1.GetRange(&range1);

  const viskores::cont::Field field2 = ds2.GetField(fieldNm);
  viskores::Range range2;
  field2.GetRange(&range2);

  mapper1.RenderCells(
    ds1.GetCellSet(), ds1.GetCoordinateSystem(), field1, colorTable1, camera, range1);

  mapper2.RenderCells(
    ds2.GetCellSet(), ds2.GetCoordinateSystem(), field2, colorTable2, camera, range2);

  VISKORES_TEST_ASSERT(test_equal_images(canvas, outputFile));
}

void RenderTests()
{
  using M1 = viskores::rendering::MapperVolume;
  using M2 = viskores::rendering::MapperConnectivity;
  using R = viskores::rendering::MapperRayTracer;

  viskores::cont::testing::MakeTestDataSet maker;
  viskores::cont::ColorTable colorTable("inferno");


  viskores::cont::ColorTable colorTable2("cool to warm");
  colorTable2.AddPointAlpha(0.0, .02f);
  colorTable2.AddPointAlpha(1.0, .02f);

  MultiMapperRender<R, M2>(maker.Make3DExplicitDataSetPolygonal(),
                           maker.Make3DRectilinearDataSet0(),
                           "pointvar",
                           colorTable,
                           colorTable2,
                           "rendering/multimapper/raytracer-connectivity.png");

  MultiMapperRender<R, M1>(maker.Make3DExplicitDataSet4(),
                           maker.Make3DRectilinearDataSet0(),
                           "pointvar",
                           colorTable,
                           colorTable2,
                           "rendering/multimapper/raytracer-volume.png");
}

} //namespace

int UnitTestMultiMapper(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RenderTests, argc, argv);
}
