//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

// viskores
#include <viskores/cont/testing/Testing.h>
#include <viskores/io/EncodePNG.h>

#include "ANARITestCommon.h"

namespace
{

void RenderTests()
{
  // Initialize ANARI /////////////////////////////////////////////////////////

  auto d = loadANARIDevice();

  // Create ANARI quad geometry //////////////////////////////////////////////

  auto vertices = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC3, 8);
  auto* positions = anari_cpp::map<viskores::Vec3f_32>(d, vertices);
  positions[0] = viskores::Vec3f_32(-0.6f, -0.6f, -0.6f);
  positions[1] = viskores::Vec3f_32(0.6f, -0.6f, -0.6f);
  positions[2] = viskores::Vec3f_32(0.6f, 0.6f, -0.6f);
  positions[3] = viskores::Vec3f_32(-0.6f, 0.6f, -0.6f);
  positions[4] = viskores::Vec3f_32(-0.6f, -0.6f, 0.6f);
  positions[5] = viskores::Vec3f_32(0.6f, -0.6f, 0.6f);
  positions[6] = viskores::Vec3f_32(0.6f, 0.6f, 0.6f);
  positions[7] = viskores::Vec3f_32(-0.6f, 0.6f, 0.6f);
  anari_cpp::unmap(d, vertices);

  auto indices = anari_cpp::newArray1D(d, ANARI_UINT32_VEC4, 6);
  auto* quads = anari_cpp::map<viskores::Vec4ui_32>(d, indices);
  quads[0] = viskores::Vec4ui_32(0, 3, 2, 1);
  quads[1] = viskores::Vec4ui_32(4, 5, 6, 7);
  quads[2] = viskores::Vec4ui_32(0, 4, 7, 3);
  quads[3] = viskores::Vec4ui_32(1, 2, 6, 5);
  quads[4] = viskores::Vec4ui_32(0, 1, 5, 4);
  quads[5] = viskores::Vec4ui_32(3, 7, 6, 2);
  anari_cpp::unmap(d, indices);

  auto attributes = anari_cpp::newArray1D(d, ANARI_FLOAT32, 6);
  auto* faceAttributes = anari_cpp::map<viskores::Float32>(d, attributes);
  faceAttributes[0] = 0.f;
  faceAttributes[1] = 0.2f;
  faceAttributes[2] = 0.4f;
  faceAttributes[3] = 0.6f;
  faceAttributes[4] = 0.8f;
  faceAttributes[5] = 1.f;
  anari_cpp::unmap(d, attributes);

  auto geometry = anari_cpp::newObject<anari_cpp::Geometry>(d, "quad");
  anari_cpp::setAndReleaseParameter(d, geometry, "vertex.position", vertices);
  anari_cpp::setAndReleaseParameter(d, geometry, "primitive.index", indices);
  anari_cpp::setAndReleaseParameter(d, geometry, "primitive.attribute0", attributes);
  anari_cpp::commitParameters(d, geometry);

  // Assemble ANARI scene /////////////////////////////////////////////////////

  auto colorArray = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC4, 6);
  auto* colors = anari_cpp::map<viskores::Vec4f_32>(d, colorArray);
  colors[0] = viskores::Vec4f_32(0.95f, 0.15f, 0.1f, 1.f);
  colors[1] = viskores::Vec4f_32(0.1f, 0.55f, 0.95f, 1.f);
  colors[2] = viskores::Vec4f_32(0.15f, 0.75f, 0.25f, 1.f);
  colors[3] = viskores::Vec4f_32(0.95f, 0.85f, 0.1f, 1.f);
  colors[4] = viskores::Vec4f_32(0.65f, 0.25f, 0.9f, 1.f);
  colors[5] = viskores::Vec4f_32(0.95f, 0.45f, 0.1f, 1.f);
  anari_cpp::unmap(d, colorArray);

  auto sampler = anari_cpp::newObject<anari_cpp::Sampler>(d, "image1D");
  anari_cpp::setAndReleaseParameter(d, sampler, "image", colorArray);
  anari_cpp::setParameter(d, sampler, "filter", "nearest");
  anari_cpp::setParameter(d, sampler, "wrapMode", "clampToEdge");
  anari_cpp::setParameter(d, sampler, "inAttribute", "attribute0");
  anari_cpp::commitParameters(d, sampler);

  auto material = anari_cpp::newObject<anari_cpp::Material>(d, "matte");
  anari_cpp::setParameter(d, material, "color", sampler);
  anari_cpp::commitParameters(d, material);
  anari_cpp::release(d, sampler);

  auto surface = anari_cpp::newObject<anari_cpp::Surface>(d);
  anari_cpp::setParameter(d, surface, "geometry", geometry);
  anari_cpp::setParameter(d, surface, "material", material);
  anari_cpp::commitParameters(d, surface);
  anari_cpp::release(d, geometry);
  anari_cpp::release(d, material);

  auto world = anari_cpp::newObject<anari_cpp::World>(d);
  anari_cpp::setParameterArray1D(d, world, "surface", &surface, 1);
  anari_cpp::commitParameters(d, world);
  anari_cpp::release(d, surface);

  // Render a frame ///////////////////////////////////////////////////////////

  renderTestANARIImage(d,
                       world,
                       viskores::Vec3f_32(2.f, 1.5f, 2.5f),
                       viskores::Vec3f_32(-0.57f, -0.43f, -0.7f),
                       viskores::Vec3f_32(0.f, 1.f, 0.f),
                       "interop/anari/quad.png",
                       viskores::Vec2ui_32(512, 512));

  // Cleanup //////////////////////////////////////////////////////////////////

  anari_cpp::release(d, world);
  anari_cpp::release(d, d);
}

} // namespace

int UnitTestANARIGeometryQuad(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RenderTests, argc, argv);
}
