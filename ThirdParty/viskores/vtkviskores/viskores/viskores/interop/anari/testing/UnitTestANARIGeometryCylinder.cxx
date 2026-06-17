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

  const char** extensions = nullptr;
  anariGetProperty(d, d, "extension", ANARI_STRING_LIST, &extensions, sizeof(char**), ANARI_WAIT);
  bool cylindersSupported = false;
  for (int i = 0; extensions[i] != nullptr; ++i)
  {
    if (extensions[i] == std::string("KHR_GEOMETRY_CYLINDER") ||
        extensions[i] == std::string("ANARI_KHR_GEOMETRY_CYLINDER"))
    {
      cylindersSupported = true;
      break;
    }
  }
  if (!cylindersSupported)
  {
    VISKORES_TEST_SKIP("ANARI KHR_GEOMETRY_CYLINDER extension not supported by ANARI device.");
  }

  // Create ANARI cylinder geometry //////////////////////////////////////////

  auto vertices = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC3, 4);
  auto* positions = anari_cpp::map<viskores::Vec3f_32>(d, vertices);
  positions[0] = viskores::Vec3f_32(-0.8f, -0.25f, 0.f);
  positions[1] = viskores::Vec3f_32(0.6f, -0.25f, 0.f);
  positions[2] = viskores::Vec3f_32(-0.45f, 0.35f, -0.35f);
  positions[3] = viskores::Vec3f_32(0.4f, 0.35f, 0.35f);
  anari_cpp::unmap(d, vertices);

  auto indices = anari_cpp::newArray1D(d, ANARI_UINT32_VEC2, 2);
  auto* cylinders = anari_cpp::map<viskores::Vec2ui_32>(d, indices);
  cylinders[0] = viskores::Vec2ui_32(0, 1);
  cylinders[1] = viskores::Vec2ui_32(2, 3);
  anari_cpp::unmap(d, indices);

  auto radii = anari_cpp::newArray1D(d, ANARI_FLOAT32, 2);
  auto* cylinderRadii = anari_cpp::map<viskores::Float32>(d, radii);
  cylinderRadii[0] = 0.18f;
  cylinderRadii[1] = 0.12f;
  anari_cpp::unmap(d, radii);

  auto caps = anari_cpp::newArray1D(d, ANARI_UINT8, 4);
  auto* vertexCaps = anari_cpp::map<viskores::UInt8>(d, caps);
  vertexCaps[0] = 1;
  vertexCaps[1] = 1;
  vertexCaps[2] = 1;
  vertexCaps[3] = 0;
  anari_cpp::unmap(d, caps);

  auto attributes = anari_cpp::newArray1D(d, ANARI_FLOAT32, 2);
  auto* primitiveAttributes = anari_cpp::map<viskores::Float32>(d, attributes);
  primitiveAttributes[0] = 0.f;
  primitiveAttributes[1] = 1.f;
  anari_cpp::unmap(d, attributes);

  auto geometry = anari_cpp::newObject<anari_cpp::Geometry>(d, "cylinder");
  anari_cpp::setAndReleaseParameter(d, geometry, "vertex.position", vertices);
  anari_cpp::setAndReleaseParameter(d, geometry, "primitive.index", indices);
  anari_cpp::setAndReleaseParameter(d, geometry, "primitive.radius", radii);
  anari_cpp::setAndReleaseParameter(d, geometry, "vertex.cap", caps);
  anari_cpp::setAndReleaseParameter(d, geometry, "primitive.attribute0", attributes);
  anari_cpp::setParameter(d, geometry, "caps", "none");
  anari_cpp::commitParameters(d, geometry);

  // Assemble ANARI scene /////////////////////////////////////////////////////

  auto colorArray = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC4, 2);
  auto* colors = anari_cpp::map<viskores::Vec4f_32>(d, colorArray);
  colors[0] = viskores::Vec4f_32(0.95f, 0.15f, 0.1f, 1.f);
  colors[1] = viskores::Vec4f_32(0.1f, 0.55f, 0.95f, 1.f);
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
                       viskores::Vec3f_32(1.7f, -2.f, 1.2f),
                       viskores::Vec3f_32(-0.58f, 0.69f, -0.43f),
                       viskores::Vec3f_32(0.f, 0.f, 1.f),
                       "interop/anari/cylinder.png",
                       viskores::Vec2ui_32(512, 512));

  // Cleanup //////////////////////////////////////////////////////////////////

  anari_cpp::release(d, world);
  anari_cpp::release(d, d);
}

} // namespace

int UnitTestANARIGeometryCylinder(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RenderTests, argc, argv);
}
