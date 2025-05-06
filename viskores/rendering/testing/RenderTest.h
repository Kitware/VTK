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
#ifndef viskores_rendering_testing_RenderTest_h
#define viskores_rendering_testing_RenderTest_h

// Because the testing directory is reserved for test executables and not
// libraries, the viskores_rendering_testing module has to put this file in
// viskores/rendering/testlib instead of viskores/rendering/testing where you normally
// would expect it.
#include <viskores/rendering/testlib/viskores_rendering_testing_export.h>

#include <viskores/Bounds.h>
#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/rendering/Actor.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/Color.h>
#include <viskores/rendering/GlyphType.h>
#include <viskores/rendering/Mapper.h>
#include <viskores/rendering/Scene.h>
#include <viskores/rendering/TextAnnotationScreen.h>
#include <viskores/rendering/View1D.h>
#include <viskores/rendering/View2D.h>
#include <viskores/rendering/View3D.h>
#include <viskores/rendering/testing/Testing.h>

#include <memory>

namespace viskores
{
namespace rendering
{
namespace testing
{

enum struct CanvasType
{
  RayTracer
};

enum struct MapperType
{
  RayTracer,
  Connectivity,
  Cylinder,
  Point,
  Quad,
  Volume,
  Wireframer,
  GlyphScalar,
  GlyphVector
};

struct RenderTestOptions
{
  // Options for comparing images (i.e. test_equal_images)
  viskores::IdComponent AverageRadius = 0;
  viskores::IdComponent PixelShiftRadius = 0;
  viskores::FloatDefault AllowedPixelErrorRatio = 0.00025f;
  viskores::FloatDefault Threshold = 0.05f;

  // Options that set up rendering
  CanvasType Canvas = CanvasType::RayTracer;
  viskores::IdComponent ViewDimension = 3;
  MapperType Mapper = MapperType::RayTracer;
  viskores::Id CanvasWidth = 512;
  viskores::Id CanvasHeight = 512;
  bool EnableAnnotations = true;
  viskores::Float64 DataViewPadding = 0;
  viskores::rendering::Color Foreground = viskores::rendering::Color::black;
  viskores::rendering::Color Background = viskores::rendering::Color::white;

  // By default, scalar values will be mapped by this ColorTable to make colors.
  viskores::cont::ColorTable ColorTable;
  // If you want constant colors (per DataSet or field or partition), then you can
  // set this vector to the colors you want to use. If one color is specified, it
  // will be used for everything. If multiple colors are specified, each will be
  // used for a different DataSet/field/partition.
  std::vector<viskores::rendering::Color> Colors;

  // For 3D rendering
  viskores::Float32 CameraAzimuth = 45.0f;
  viskores::Float32 CameraElevation = 45.0f;

  // For 2D/1D rendering
  viskores::Range ClippingRange = { 1, 100 };
  viskores::Bounds Viewport = { { -0.7, 0.7 }, { -0.7, 0.7 }, { 0.0, 0.0 } };

  // For 1D rendering
  bool LogX = false;
  bool LogY = false;

  std::string Title;
  viskores::Float32 TitleScale = 0.075f;
  viskores::Vec2f_32 TitlePosition = { -0.11f, 0.92f };
  viskores::Float32 TitleAngle = 0;

  // Usually when calling RenderTest, you are not specifically testing rendering.
  // Rather, you are testing something else and using render to check the results.
  // Regardless of what device you are using for testing, you probably want to
  // use the best available device for rendering.
  bool AllowAnyDevice = true;

  // Special options for some glyph and glyph-like mappers
  viskores::rendering::GlyphType GlyphType = viskores::rendering::GlyphType::Cube;
  bool UseVariableRadius = false;
  viskores::Float32 Radius = -1.0f;
  viskores::Float32 RadiusDelta = 0.5f;
  bool RenderCells = false;
};

VISKORES_RENDERING_TESTING_EXPORT
void RenderTest(const viskores::cont::DataSet& dataSet,
                const std::string& fieldName,
                const std::string& outputFile,
                const RenderTestOptions& options = RenderTestOptions{});

VISKORES_RENDERING_TESTING_EXPORT
void RenderTest(const std::vector<std::pair<viskores::cont::DataSet, std::string>>& dataSetsFields,
                const std::string& outputFile,
                const RenderTestOptions& options = RenderTestOptions{});

} // namespace viskores::rendering::testing
} // namespace viskores::rendering
} // namespace viskores

#endif //viskores_rendering_testing_RenderTest_h
