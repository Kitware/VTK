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
#include <viskores/filter/field_conversion/CellAverage.h>
#include <viskores/io/VTKDataSetReader.h>
#include <viskores/rendering/Actor.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/MapperVolume.h>
#include <viskores/rendering/Scene.h>
#include <viskores/rendering/View3D.h>
#include <viskores/rendering/testing/RenderTest.h>
#include <viskores/source/Tangle.h>

namespace
{

void TestRectilinear()
{
  viskores::cont::ColorTable colorTable = viskores::cont::ColorTable::Preset::Inferno;
  colorTable.AddPointAlpha(0.0, 0.01f);
  colorTable.AddPointAlpha(0.4, 0.01f);
  colorTable.AddPointAlpha(0.7, 0.2f);
  colorTable.AddPointAlpha(1.0, 0.5f);

  viskores::rendering::testing::RenderTestOptions options;
  options.Mapper = viskores::rendering::testing::MapperType::Volume;
  options.AllowAnyDevice = false;
  options.ColorTable = colorTable;

  viskores::cont::DataSet rectDS, unsDS;
  std::string rectfname =
    viskores::cont::testing::Testing::DataPath("third_party/visit/example.vtk");
  viskores::io::VTKDataSetReader rectReader(rectfname);

  try
  {
    rectDS = rectReader.ReadDataSet();
  }
  catch (viskores::io::ErrorIO& e)
  {
    std::string message("Error reading: ");
    message += rectfname;
    message += ", ";
    message += e.GetMessage();

    VISKORES_TEST_FAIL(message.c_str());
  }

  viskores::rendering::testing::RenderTest(
    rectDS, "temp", "rendering/volume/rectilinear3D.png", options);

  viskores::filter::field_conversion::CellAverage cellAverage;
  cellAverage.SetActiveField("temp");
  cellAverage.SetOutputFieldName("temp_avg");
  viskores::cont::DataSet tempAvg = cellAverage.Execute(rectDS);

  viskores::rendering::testing::RenderTest(
    tempAvg, "temp_avg", "rendering/volume/rectilinear3D_cell.png", options);
}

void TestUniformGrid()
{
  viskores::cont::ColorTable colorTable = viskores::cont::ColorTable::Preset::Inferno;
  colorTable.AddPointAlpha(0.0, 0.2f);
  colorTable.AddPointAlpha(0.2, 0.0f);
  colorTable.AddPointAlpha(0.5, 0.0f);

  viskores::rendering::testing::RenderTestOptions options;
  options.Mapper = viskores::rendering::testing::MapperType::Volume;
  options.AllowAnyDevice = false;
  options.ColorTable = colorTable;
  // Rendering of AxisAnnotation3D is sensitive on the type
  // of FloatDefault, disable it before we know how to fix
  // it properly.
  options.EnableAnnotations = false;

  viskores::source::Tangle tangle;
  tangle.SetPointDimensions({ 50, 50, 50 });
  viskores::cont::DataSet tangleData = tangle.Execute();

  viskores::rendering::testing::RenderTest(
    tangleData, "tangle", "rendering/volume/uniform.png", options);

  viskores::filter::field_conversion::CellAverage cellAverage;
  cellAverage.SetActiveField("tangle");
  cellAverage.SetOutputFieldName("tangle_avg");
  viskores::cont::DataSet tangleAvg = cellAverage.Execute(tangleData);

  viskores::rendering::testing::RenderTest(
    tangleAvg, "tangle_avg", "rendering/volume/uniform_cell.png", options);
}

void RenderTests()
{
  TestRectilinear();
  TestUniformGrid();
}

} //namespace

int UnitTestMapperVolume(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RenderTests, argc, argv);
}
