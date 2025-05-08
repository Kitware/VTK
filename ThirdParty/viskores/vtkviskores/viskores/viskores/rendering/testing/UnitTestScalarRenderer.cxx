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

#include <viskores/io/VTKDataSetWriter.h>
#include <viskores/rendering/ScalarRenderer.h>
#include <viskores/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  viskores::cont::testing::MakeTestDataSet maker;
  viskores::cont::DataSet dataset = maker.Make3DRegularDataSet0();
  viskores::Bounds bounds = dataset.GetCoordinateSystem().GetBounds();

  viskores::rendering::Camera camera;
  camera.ResetToBounds(bounds);
  camera.Azimuth(-40.f);
  camera.Elevation(15.f);

  viskores::rendering::ScalarRenderer renderer;
  renderer.SetInput(dataset);
  viskores::rendering::ScalarRenderer::Result res = renderer.Render(camera);

  viskores::cont::DataSet result = res.ToDataSet();
  viskores::io::VTKDataSetWriter writer("scalar.vtk");
  writer.WriteDataSet(result);
}

} //namespace

int UnitTestScalarRenderer(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RenderTests, argc, argv);
}
