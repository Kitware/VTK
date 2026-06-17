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

anari_cpp::Array1D CreateSpherePositions(anari_cpp::Device d)
{
  auto vertices = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC3, 6);
  auto* positions = anari_cpp::map<viskores::Vec3f_32>(d, vertices);
  positions[0] = viskores::Vec3f_32(-0.65f, -0.25f, 0.f);
  positions[1] = viskores::Vec3f_32(-0.15f, 0.35f, 0.1f);
  positions[2] = viskores::Vec3f_32(0.45f, -0.12f, -0.12f);
  positions[3] = viskores::Vec3f_32(0.1f, -0.55f, 0.18f);
  positions[4] = viskores::Vec3f_32(0.65f, 0.38f, 0.f);
  positions[5] = viskores::Vec3f_32(0.f, 0.f, 0.8f);
  anari_cpp::unmap(d, vertices);

  return vertices;
}

anari_cpp::Array1D CreateSphereRadii(anari_cpp::Device d)
{
  auto radii = anari_cpp::newArray1D(d, ANARI_FLOAT32, 6);
  auto* sphereRadii = anari_cpp::map<viskores::Float32>(d, radii);
  sphereRadii[0] = 0.18f;
  sphereRadii[1] = 0.24f;
  sphereRadii[2] = 0.2f;
  sphereRadii[3] = 0.16f;
  sphereRadii[4] = 0.14f;
  sphereRadii[5] = 0.22f;
  anari_cpp::unmap(d, radii);

  return radii;
}

anari_cpp::Array1D CreateSphereAttributes(anari_cpp::Device d)
{
  auto attributes = anari_cpp::newArray1D(d, ANARI_FLOAT32, 6);
  auto* vertexAttributes = anari_cpp::map<viskores::Float32>(d, attributes);
  vertexAttributes[0] = 0.f;
  vertexAttributes[1] = 0.25f;
  vertexAttributes[2] = 0.5f;
  vertexAttributes[3] = 0.75f;
  vertexAttributes[4] = 1.f;
  vertexAttributes[5] = 0.1f;
  anari_cpp::unmap(d, attributes);

  return attributes;
}

anari_cpp::Array1D CreateSphereIndices(anari_cpp::Device d)
{
  auto indices = anari_cpp::newArray1D(d, ANARI_UINT32, 5);
  auto* spheres = anari_cpp::map<viskores::UInt32>(d, indices);
  spheres[0] = 0;
  spheres[1] = 2;
  spheres[2] = 4;
  spheres[3] = 1;
  spheres[4] = 3;
  anari_cpp::unmap(d, indices);

  return indices;
}

anari_cpp::Geometry CreateSphereGeometry(anari_cpp::Device d, bool indexed)
{
  auto geometry = anari_cpp::newObject<anari_cpp::Geometry>(d, "sphere");
  anari_cpp::setAndReleaseParameter(d, geometry, "vertex.position", CreateSpherePositions(d));
  if (indexed)
  {
    anari_cpp::setAndReleaseParameter(d, geometry, "primitive.index", CreateSphereIndices(d));
  }
  anari_cpp::setAndReleaseParameter(d, geometry, "vertex.radius", CreateSphereRadii(d));
  anari_cpp::setAndReleaseParameter(d, geometry, "vertex.attribute0", CreateSphereAttributes(d));
  anari_cpp::commitParameters(d, geometry);

  return geometry;
}

anari_cpp::Material CreateColorMaterial(anari_cpp::Device d)
{
  auto colorArray = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC4, 5);
  auto* colors = anari_cpp::map<viskores::Vec4f_32>(d, colorArray);
  colors[0] = viskores::Vec4f_32(0.95f, 0.15f, 0.1f, 1.f);
  colors[1] = viskores::Vec4f_32(0.1f, 0.55f, 0.95f, 1.f);
  colors[2] = viskores::Vec4f_32(0.15f, 0.75f, 0.25f, 1.f);
  colors[3] = viskores::Vec4f_32(0.95f, 0.85f, 0.1f, 1.f);
  colors[4] = viskores::Vec4f_32(0.65f, 0.25f, 0.9f, 1.f);
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

  return material;
}

anari_cpp::World CreateSphereWorld(anari_cpp::Device d, bool indexed)
{
  auto geometry = CreateSphereGeometry(d, indexed);
  auto material = CreateColorMaterial(d);

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

  return world;
}

void RenderTests()
{
  // Initialize ANARI /////////////////////////////////////////////////////////

  auto d = loadANARIDevice();

  const char** extensions = nullptr;
  anariGetProperty(d, d, "extension", ANARI_STRING_LIST, &extensions, sizeof(char**), ANARI_WAIT);
  bool spheresSupported = false;
  for (int i = 0; extensions[i] != nullptr; ++i)
  {
    if (extensions[i] == std::string("KHR_GEOMETRY_SPHERE") ||
        extensions[i] == std::string("ANARI_KHR_GEOMETRY_SPHERE"))
    {
      spheresSupported = true;
      break;
    }
  }
  if (!spheresSupported)
  {
    VISKORES_TEST_SKIP("ANARI KHR_GEOMETRY_SPHERE extension not supported by ANARI device.");
  }

  // Render indexed sphere geometry //////////////////////////////////////////

  auto indexedWorld = CreateSphereWorld(d, true);
  renderTestANARIImage(d,
                       indexedWorld,
                       viskores::Vec3f_32(1.8f, -2.2f, 1.4f),
                       viskores::Vec3f_32(-0.58f, 0.7f, -0.42f),
                       viskores::Vec3f_32(0.f, 0.f, 1.f),
                       "interop/anari/sphere.png",
                       viskores::Vec2ui_32(512, 512));
  anari_cpp::release(d, indexedWorld);

  // Render non-indexed sphere geometry //////////////////////////////////////

  auto unindexedWorld = CreateSphereWorld(d, false);
  renderTestANARIImage(d,
                       unindexedWorld,
                       viskores::Vec3f_32(1.8f, -2.2f, 1.4f),
                       viskores::Vec3f_32(-0.58f, 0.7f, -0.42f),
                       viskores::Vec3f_32(0.f, 0.f, 1.f),
                       "interop/anari/sphere-unindexed.png",
                       viskores::Vec2ui_32(512, 512));
  anari_cpp::release(d, unindexedWorld);

  // Cleanup //////////////////////////////////////////////////////////////////

  anari_cpp::release(d, d);
}

} // namespace

int UnitTestANARIGeometrySphere(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RenderTests, argc, argv);
}
