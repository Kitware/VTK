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
#include <viskores/io/FileUtils.h>
#include <viskores/io/ImageUtils.h>
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

void TestVolumeRenderOccludesAnnotations()
{
  viskores::cont::ColorTable colorTable = viskores::cont::ColorTable::Preset::Inferno;
  colorTable.AddPointAlpha(0.0, 0.2f);
  colorTable.AddPointAlpha(0.2, 0.0f);
  colorTable.AddPointAlpha(0.5, 0.0f);

  viskores::source::Tangle tangle;
  tangle.SetPointDimensions({ 50, 50, 50 });
  viskores::cont::DataSet tangleData = tangle.Execute();

  const viskores::cont::Field field = tangleData.GetField("tangle");
  viskores::Range range;
  field.GetRange(&range);

  viskores::rendering::CanvasRayTracer canvas(128, 128);
  canvas.Clear();

  viskores::rendering::Camera camera;
  camera.ResetToBounds(tangleData.GetCoordinateSystem().GetBounds());
  camera.Azimuth(45.0f);
  camera.Elevation(45.0f);

  viskores::rendering::MapperVolume mapper;
  mapper.SetActiveColorTable(colorTable);
  mapper.SetCompositeBackground(false);

  const viskores::Bounds bounds = tangleData.GetCoordinateSystem().GetBounds();
  viskores::Vec3f_32 viewDir = camera.GetLookAt() - camera.GetPosition();
  viskores::Normalize(viewDir);
  viskores::Vec3f_32 rightDir = viskores::Cross(viewDir, camera.GetViewUp());
  viskores::Normalize(rightDir);

  const auto boundsCenter = bounds.Center();
  const viskores::Vec3f_32 center(static_cast<viskores::Float32>(boundsCenter[0]),
                                  static_cast<viskores::Float32>(boundsCenter[1]),
                                  static_cast<viskores::Float32>(boundsCenter[2]));
  const viskores::Vec3f_32 extents(static_cast<viskores::Float32>(bounds.X.Length()),
                                   static_cast<viskores::Float32>(bounds.Y.Length()),
                                   static_cast<viskores::Float32>(bounds.Z.Length()));
  const viskores::Float32 diagonal = viskores::Magnitude(extents);
  const viskores::Float32 annotationOffset = 0.2f * diagonal;
  const viskores::Float32 annotationHalfLength = 0.35f * diagonal;

  auto renderVolume = [&](viskores::rendering::CanvasRayTracer& targetCanvas)
  {
    targetCanvas.Clear();
    mapper.SetCanvas(&targetCanvas);
    mapper.RenderCells(
      tangleData.GetCellSet(), tangleData.GetCoordinateSystem(), field, colorTable, camera, range);
  };

  auto drawAnnotationLine = [&](viskores::rendering::CanvasRayTracer& targetCanvas,
                                const viskores::Vec3f_32& lineCenter,
                                const viskores::rendering::Color& color)
  {
    targetCanvas.SetViewToWorldSpace(camera, true);
    std::unique_ptr<viskores::rendering::WorldAnnotator> annotator(
      targetCanvas.CreateWorldAnnotator());
    annotator->BeginLineRenderingBatch();
    annotator->AddLine(lineCenter - rightDir * annotationHalfLength,
                       lineCenter + rightDir * annotationHalfLength,
                       1.0f,
                       color);
    annotator->EndLineRenderingBatch();
  };

  auto writeCanvasImage =
    [&](viskores::rendering::CanvasRayTracer& targetCanvas, const std::string& fileName)
  {
    targetCanvas.RefreshColorBuffer();
    viskores::io::WriteImageFile(targetCanvas.GetDataSet("color", ""),
                                 viskores::cont::testing::Testing::WriteDirPath(
                                   viskores::io::PrefixStringToFilename(fileName, "test-")),
                                 "color");
  };

  auto countChangedPixels = [&](const viskores::rendering::CanvasRayTracer& lhs,
                                const viskores::rendering::CanvasRayTracer& rhs)
  {
    const auto lhsPortal = lhs.GetColorBuffer().ReadPortal();
    const auto rhsPortal = rhs.GetColorBuffer().ReadPortal();
    viskores::Id changedPixels = 0;
    for (viskores::Id y = 0; y < canvas.GetHeight(); ++y)
    {
      for (viskores::Id x = 0; x < canvas.GetWidth(); ++x)
      {
        const viskores::Id pixelIndex = y * canvas.GetWidth() + x;
        if (!test_equal(lhsPortal.Get(pixelIndex), rhsPortal.Get(pixelIndex)))
        {
          ++changedPixels;
        }
      }
    }
    return changedPixels;
  };

  viskores::rendering::CanvasRayTracer baseCanvas(128, 128);
  renderVolume(baseCanvas);

  viskores::rendering::CanvasRayTracer behindCanvas(128, 128);
  renderVolume(behindCanvas);
  drawAnnotationLine(behindCanvas,
                     center + viewDir * annotationOffset,
                     viskores::rendering::Color(1.f, 0.f, 0.f, 1.f));
  writeCanvasImage(behindCanvas, "rendering/volume/annotation-occlusion-behind.png");

  viskores::rendering::CanvasRayTracer frontCanvas(128, 128);
  renderVolume(frontCanvas);
  drawAnnotationLine(frontCanvas,
                     center - viewDir * annotationOffset,
                     viskores::rendering::Color(0.f, 1.f, 0.f, 1.f));
  writeCanvasImage(frontCanvas, "rendering/volume/annotation-occlusion-front.png");

  VISKORES_TEST_ASSERT(countChangedPixels(frontCanvas, baseCanvas) > 0,
                       "The control annotation line did not render in front of the volume.");
  VISKORES_TEST_ASSERT(
    countChangedPixels(frontCanvas, behindCanvas) > 0,
    "An annotation drawn behind a pure volume render matched the front-of-volume result.");
}

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
  TestVolumeRenderOccludesAnnotations();
  TestRectilinear();
  TestUniformGrid();
}

} //namespace

int UnitTestMapperVolume(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RenderTests, argc, argv);
}
