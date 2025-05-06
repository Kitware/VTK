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

#include <viskores/rendering/testing/RenderTest.h>

#include <viskores/rendering/Actor.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/MapperConnectivity.h>
#include <viskores/rendering/MapperCylinder.h>
#include <viskores/rendering/MapperGlyphScalar.h>
#include <viskores/rendering/MapperGlyphVector.h>
#include <viskores/rendering/MapperPoint.h>
#include <viskores/rendering/MapperQuad.h>
#include <viskores/rendering/MapperRayTracer.h>
#include <viskores/rendering/MapperVolume.h>
#include <viskores/rendering/MapperWireframer.h>
#include <viskores/rendering/TextAnnotationScreen.h>
#include <viskores/rendering/View1D.h>
#include <viskores/rendering/View2D.h>
#include <viskores/rendering/View3D.h>

#include <viskores/cont/RuntimeDeviceTracker.h>

namespace
{

using DataSetFieldVector = std::vector<std::pair<viskores::cont::DataSet, std::string>>;

void SetupView(viskores::rendering::View3D& view,
               const viskores::Bounds& bounds,
               const viskores::Range&,
               const viskores::rendering::testing::RenderTestOptions& options)
{
  viskores::rendering::Camera camera;
  camera.ResetToBounds(bounds, options.DataViewPadding);
  camera.Azimuth(options.CameraAzimuth);
  camera.Elevation(options.CameraElevation);
  view.SetCamera(camera);
}

void SetupView(viskores::rendering::View2D& view,
               const viskores::Bounds& bounds,
               const viskores::Range&,
               const viskores::rendering::testing::RenderTestOptions& options)
{
  viskores::rendering::Camera camera;
  camera.ResetToBounds(bounds, options.DataViewPadding);
  camera.SetClippingRange(options.ClippingRange);
  camera.SetViewport(options.Viewport);
  view.SetCamera(camera);
}

void SetupView(viskores::rendering::View1D& view,
               const viskores::Bounds& bounds,
               const viskores::Range& fieldRange,
               const viskores::rendering::testing::RenderTestOptions& options)
{
  viskores::rendering::Camera camera;
  // In a 1D view, the y bounds are determined by the field that is being x/y plotted.
  camera.ResetToBounds({ bounds.X, fieldRange, { 0, 0 } }, options.DataViewPadding);
  camera.SetClippingRange(options.ClippingRange);
  camera.SetViewport(options.Viewport);
  view.SetCamera(camera);

  view.SetLogX(options.LogX);
  view.SetLogY(options.LogY);
}

template <typename ViewType>
std::unique_ptr<viskores::rendering::View> MakeView(
  viskores::rendering::Canvas& canvas,
  viskores::rendering::Mapper& mapper,
  viskores::rendering::Scene& scene,
  const viskores::Bounds& bounds,
  const viskores::Range& fieldRange,
  const viskores::rendering::testing::RenderTestOptions& options)
{
  ViewType* view = new ViewType(scene, mapper, canvas, options.Background, options.Foreground);
  SetupView(*view, bounds, fieldRange, options);
  return std::unique_ptr<viskores::rendering::View>(view);
}

template <typename MapperType>
void SetupMapper(MapperType&, const viskores::rendering::testing::RenderTestOptions&)
{
}

void SetupMapper(viskores::rendering::MapperCylinder& mapper,
                 const viskores::rendering::testing::RenderTestOptions& options)
{
  mapper.UseVariableRadius(options.UseVariableRadius);
  if (options.Radius >= 0)
  {
    mapper.SetRadius(options.Radius);
  }
  mapper.SetRadiusDelta(0.5);
}

void SetupMapper(viskores::rendering::MapperPoint& mapper,
                 const viskores::rendering::testing::RenderTestOptions& options)
{
  mapper.UseVariableRadius(options.UseVariableRadius);
  if (options.Radius >= 0)
  {
    mapper.SetRadius(options.Radius);
  }
  mapper.SetRadiusDelta(0.5);
  if (options.RenderCells)
  {
    mapper.SetUseCells();
  }
}

void SetupMapper(viskores::rendering::MapperGlyphScalar& mapper,
                 const viskores::rendering::testing::RenderTestOptions& options)
{
  mapper.SetGlyphType(options.GlyphType);
  mapper.SetScaleByValue(options.UseVariableRadius);
  if (options.Radius >= 0)
  {
    mapper.SetBaseSize(options.Radius);
  }
  mapper.SetScaleDelta(0.5);
  if (options.RenderCells)
  {
    mapper.SetUseCells();
  }
}

void SetupMapper(viskores::rendering::MapperGlyphVector& mapper,
                 const viskores::rendering::testing::RenderTestOptions& options)
{
  mapper.SetGlyphType(options.GlyphType);
  mapper.SetScaleByValue(options.UseVariableRadius);
  if (options.Radius >= 0)
  {
    mapper.SetBaseSize(options.Radius);
  }
  mapper.SetScaleDelta(0.5);
  if (options.RenderCells)
  {
    mapper.SetUseCells();
  }
}

template <typename MapperType>
std::unique_ptr<viskores::rendering::Mapper> MakeMapper(
  const viskores::rendering::testing::RenderTestOptions& options)
{
  MapperType* mapper = new MapperType;
  SetupMapper(*mapper, options);
  return std::unique_ptr<viskores::rendering::Mapper>(mapper);
}

void DoRenderTest(viskores::rendering::Canvas& canvas,
                  viskores::rendering::Mapper& mapper,
                  const DataSetFieldVector& dataSetsFields,
                  const std::string& outputFile,
                  const viskores::rendering::testing::RenderTestOptions& options)
{
  std::size_t numFields = dataSetsFields.size();
  VISKORES_TEST_ASSERT(numFields > 0);

  viskores::rendering::Scene scene;
  viskores::Bounds bounds;
  viskores::Range fieldRange;
  for (std::size_t dataFieldId = 0; dataFieldId < numFields; ++dataFieldId)
  {
    viskores::cont::DataSet dataSet = dataSetsFields[dataFieldId].first;
    std::string fieldName = dataSetsFields[dataFieldId].second;
    if (options.Colors.empty())
    {
      scene.AddActor(viskores::rendering::Actor(dataSet.GetCellSet(),
                                                dataSet.GetCoordinateSystem(),
                                                dataSet.GetField(fieldName),
                                                options.ColorTable));
    }
    else
    {
      scene.AddActor(
        viskores::rendering::Actor(dataSet.GetCellSet(),
                                   dataSet.GetCoordinateSystem(),
                                   dataSet.GetField(fieldName),
                                   options.Colors[dataFieldId % options.Colors.size()]));
    }
    bounds.Include(dataSet.GetCoordinateSystem().GetBounds());
    fieldRange.Include(dataSet.GetField(fieldName).GetRange().ReadPortal().Get(0));
  }

  std::unique_ptr<viskores::rendering::View> viewPointer;
  switch (options.ViewDimension)
  {
    case 1:
      viewPointer =
        MakeView<viskores::rendering::View1D>(canvas, mapper, scene, bounds, fieldRange, options);
      break;
    case 2:
      viewPointer =
        MakeView<viskores::rendering::View2D>(canvas, mapper, scene, bounds, fieldRange, options);
      break;
    case 3:
      viewPointer =
        MakeView<viskores::rendering::View3D>(canvas, mapper, scene, bounds, fieldRange, options);
      break;
  }
  viskores::rendering::View& view = *viewPointer;

  view.AddTextAnnotation(std::unique_ptr<viskores::rendering::TextAnnotationScreen>(
    new viskores::rendering::TextAnnotationScreen(options.Title,
                                                  options.Foreground,
                                                  options.TitleScale,
                                                  options.TitlePosition,
                                                  options.TitleAngle)));
  view.SetRenderAnnotationsEnabled(options.EnableAnnotations);

  VISKORES_TEST_ASSERT(test_equal_images(view,
                                         outputFile,
                                         options.AverageRadius,
                                         options.PixelShiftRadius,
                                         options.AllowedPixelErrorRatio,
                                         options.Threshold));
}

void DoRenderTest(viskores::rendering::CanvasRayTracer& canvas,
                  const DataSetFieldVector& dataSetsFields,
                  const std::string& outputFile,
                  const viskores::rendering::testing::RenderTestOptions& options)
{
  std::unique_ptr<viskores::rendering::Mapper> mapper;
  switch (options.Mapper)
  {
    case viskores::rendering::testing::MapperType::RayTracer:
      mapper = MakeMapper<viskores::rendering::MapperRayTracer>(options);
      break;
    case viskores::rendering::testing::MapperType::Connectivity:
      mapper = MakeMapper<viskores::rendering::MapperConnectivity>(options);
      break;
    case viskores::rendering::testing::MapperType::Cylinder:
      mapper = MakeMapper<viskores::rendering::MapperCylinder>(options);
      break;
    case viskores::rendering::testing::MapperType::Point:
      mapper = MakeMapper<viskores::rendering::MapperPoint>(options);
      break;
    case viskores::rendering::testing::MapperType::Quad:
      mapper = MakeMapper<viskores::rendering::MapperQuad>(options);
      break;
    case viskores::rendering::testing::MapperType::Volume:
      mapper = MakeMapper<viskores::rendering::MapperVolume>(options);
      break;
    case viskores::rendering::testing::MapperType::Wireframer:
      mapper = MakeMapper<viskores::rendering::MapperWireframer>(options);
      break;
    case viskores::rendering::testing::MapperType::GlyphScalar:
      mapper = MakeMapper<viskores::rendering::MapperGlyphScalar>(options);
      break;
    case viskores::rendering::testing::MapperType::GlyphVector:
      mapper = MakeMapper<viskores::rendering::MapperGlyphVector>(options);
      break;
  }
  DoRenderTest(canvas, *mapper, dataSetsFields, outputFile, options);
}

} // annonymous namesapce

namespace viskores
{
namespace rendering
{
namespace testing
{

void RenderTest(const viskores::cont::DataSet& dataSet,
                const std::string& fieldName,
                const std::string& outputFile,
                const RenderTestOptions& options)
{
  RenderTest({ { dataSet, fieldName } }, outputFile, options);
}

void RenderTest(const DataSetFieldVector& dataSetsFields,
                const std::string& outputFile,
                const RenderTestOptions& options)
{
  std::unique_ptr<viskores::cont::ScopedRuntimeDeviceTracker> deviceScope;
  if (options.AllowAnyDevice)
  {
    deviceScope = std::make_unique<viskores::cont::ScopedRuntimeDeviceTracker>(
      viskores::cont::DeviceAdapterTagAny{});
  }

  if (options.Canvas != viskores::rendering::testing::CanvasType::RayTracer)
  {
    VISKORES_TEST_FAIL("Currently only the CanvasRayTracer canvas is supported.");
  }

  viskores::rendering::CanvasRayTracer canvas(options.CanvasWidth, options.CanvasHeight);
  DoRenderTest(canvas, dataSetsFields, outputFile, options);
}

} // namespace viskores::rendering::testing
} // namespace viskores::rendering
} // namespace viskores
