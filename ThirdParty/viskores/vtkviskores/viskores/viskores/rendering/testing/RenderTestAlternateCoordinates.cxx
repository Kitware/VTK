//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/source/Wavelet.h>

#include <viskores/rendering/testing/RenderTest.h>

// Test alternate ways to represent coordinate arrays.

namespace
{

void ConvertCoordsToSOA(viskores::cont::DataSet& data)
{
  viskores::cont::CoordinateSystem coords = data.GetCoordinateSystem();
  viskores::cont::ArrayHandleSOA<viskores::Vec3f> soaCoordArray;
  viskores::cont::ArrayCopy(coords.GetData(), soaCoordArray);
  coords.SetData(soaCoordArray);
  data.AddCoordinateSystem(coords);
}

void TestStructuredGrid(const viskores::cont::DataSet& data)
{
  viskores::rendering::testing::RenderTestOptions options;
  options.ColorTable = viskores::cont::ColorTable(viskores::cont::ColorTable::Preset::Inferno);
  options.Mapper = viskores::rendering::testing::MapperType::RayTracer;
  options.CameraAzimuth = 225.0f;
  viskores::rendering::testing::RenderTest(
    data, "RTData", "rendering/raytracer/alt-coords.png", options);

  options.Mapper = viskores::rendering::testing::MapperType::GlyphScalar;
  options.GlyphType = viskores::rendering::GlyphType::Sphere;
  options.UseVariableRadius = true;
  options.Radius = 0.5f;
  viskores::rendering::testing::RenderTest(
    data, "RTData", "rendering/glyph_scalar/alt-coords.png", options);
}

void RunTests()
{
  viskores::source::Wavelet wavy;
  wavy.SetExtent({ 0, 0, 0 }, { 10, 10, 10 });
  viskores::cont::DataSet data = wavy.Execute();

  TestStructuredGrid(data);

  // Convert coords to SOA
  {
    viskores::cont::CoordinateSystem coords = data.GetCoordinateSystem();
    viskores::cont::ArrayHandleSOA<viskores::Vec3f> soaCoordArray;
    viskores::cont::ArrayCopy(coords.GetData(), soaCoordArray);
    coords.SetData(soaCoordArray);
    data.AddCoordinateSystem(coords);
  }
  VISKORES_TEST_ASSERT(
    data.GetCoordinateSystem().GetData().IsStorageType<viskores::cont::StorageTagSOA>());
  TestStructuredGrid(data);

  {
    // Convert to weird recombined strided array
    viskores::cont::CoordinateSystem coords = data.GetCoordinateSystem();
    auto stridedCoords = coords.GetData().ExtractArrayFromComponents<viskores::FloatDefault>();
    coords.SetData(stridedCoords);
    data.AddCoordinateSystem(coords);
  }
  TestStructuredGrid(data);
}

} // anonymous namespace

int RenderTestAlternateCoordinates(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunTests, argc, argv);
}
