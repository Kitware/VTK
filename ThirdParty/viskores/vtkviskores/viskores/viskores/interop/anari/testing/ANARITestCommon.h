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

#ifndef viskores_interop_anari_testing_ANARITestCommon_h
#define viskores_interop_anari_testing_ANARITestCommon_h

// viskores
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/interop/anari/ANARIMapper.h>
#include <viskores/rendering/testing/Testing.h>
#include <viskores/testing/Testing.h>

namespace
{

static void StatusFunc(const void* userData,
                       ANARIDevice /*device*/,
                       ANARIObject source,
                       ANARIDataType /*sourceType*/,
                       ANARIStatusSeverity severity,
                       ANARIStatusCode /*code*/,
                       const char* message)
{
  bool verbose = *(bool*)userData;
  if (!verbose)
    return;

  if (severity == ANARI_SEVERITY_FATAL_ERROR)
  {
    fprintf(stderr, "[FATAL][%p] %s\n", source, message);
  }
  else if (severity == ANARI_SEVERITY_ERROR)
  {
    fprintf(stderr, "[ERROR][%p] %s\n", source, message);
  }
  else if (severity == ANARI_SEVERITY_WARNING)
  {
    fprintf(stderr, "[WARN ][%p] %s\n", source, message);
  }
  else if (severity == ANARI_SEVERITY_PERFORMANCE_WARNING)
  {
    fprintf(stderr, "[PERF ][%p] %s\n", source, message);
  }
  else if (severity == ANARI_SEVERITY_INFO)
  {
    fprintf(stderr, "[INFO ][%p] %s\n", source, message);
  }
  else if (severity == ANARI_SEVERITY_DEBUG)
  {
    fprintf(stderr, "[DEBUG][%p] %s\n", source, message);
  }
}

static void setColorMap(anari_cpp::Device d, viskores::interop::anari::ANARIMapper& mapper)
{
  auto colorArray = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC3, 3);
  auto* colors = anari_cpp::map<viskores::Vec3f_32>(d, colorArray);
  colors[0] = viskores::Vec3f_32(0.f, 0.f, 1.f);
  colors[1] = viskores::Vec3f_32(0.f, 1.f, 0.f);
  colors[2] = viskores::Vec3f_32(1.f, 0.f, 0.f);
  anari_cpp::unmap(d, colorArray);

  auto opacityArray = anari_cpp::newArray1D(d, ANARI_FLOAT32, 2);
  auto* opacities = anari_cpp::map<float>(d, opacityArray);
  opacities[0] = 0.f;
  opacities[1] = 1.f;
  anari_cpp::unmap(d, opacityArray);

  mapper.SetANARIColorMap(colorArray, opacityArray, true);
  mapper.SetANARIColorMapValueRange(viskores::Vec2f_32(0.f, 10.f));
  mapper.SetANARIColorMapOpacityScale(0.5f);
}

static anari_cpp::Device loadANARIDevice()
{
  viskores::testing::FloatingPointExceptionTrapDisable();
  auto* libraryName = std::getenv("VISKORES_TEST_ANARI_LIBRARY");
  static bool verbose = std::getenv("VISKORES_TEST_ANARI_VERBOSE") != nullptr;
  auto lib = anari_cpp::loadLibrary(libraryName ? libraryName : "helide", StatusFunc, &verbose);
  auto d = anari_cpp::newDevice(lib, "default");
  anari_cpp::unloadLibrary(lib);
  return d;
}

static void renderTestANARIImage(anari_cpp::Device d,
                                 anari_cpp::World w,
                                 viskores::Vec3f_32 cam_pos,
                                 viskores::Vec3f_32 cam_dir,
                                 viskores::Vec3f_32 cam_up,
                                 const std::string& imgName,
                                 viskores::Vec2ui_32 imgSize = viskores::Vec2ui_32(1024, 768))
{
  auto renderer = anari_cpp::newObject<anari_cpp::Renderer>(d, "default");
  anari_cpp::setParameter(d, renderer, "background", viskores::Vec4f_32(0.3f, 0.3f, 0.3f, 1.f));
  anari_cpp::setParameter(d, renderer, "pixelSamples", 64);
  anari_cpp::commitParameters(d, renderer);

  auto camera = anari_cpp::newObject<anari_cpp::Camera>(d, "perspective");
  anari_cpp::setParameter(d, camera, "aspect", imgSize[0] / float(imgSize[1]));
  anari_cpp::setParameter(d, camera, "position", cam_pos);
  anari_cpp::setParameter(d, camera, "direction", cam_dir);
  anari_cpp::setParameter(d, camera, "up", cam_up);
  anari_cpp::commitParameters(d, camera);

  auto frame = anari_cpp::newObject<anari_cpp::Frame>(d);
  anari_cpp::setParameter(d, frame, "size", imgSize);
  anari_cpp::setParameter(d, frame, "channel.color", ANARI_FLOAT32_VEC4);
  anari_cpp::setParameter(d, frame, "world", w);
  anari_cpp::setParameter(d, frame, "camera", camera);
  anari_cpp::setParameter(d, frame, "renderer", renderer);
  anari_cpp::commitParameters(d, frame);

  anari_cpp::release(d, camera);
  anari_cpp::release(d, renderer);

  anari_cpp::render(d, frame);
  anari_cpp::wait(d, frame);

  const auto fb = anari_cpp::map<viskores::Vec4f_32>(d, frame, "channel.color");

  viskores::cont::DataSetBuilderUniform builder;
  viskores::cont::DataSet image = builder.Create(viskores::Id2(fb.width, fb.height));

  // NOTE: We are only copying the pixel data into a Viskores array for the
  //       purpose of using Viskores's image comparison test code. Applications
  //       would not normally do this and instead just use the pixel data
  //       directly, such as displaying it in an interactive window.

  viskores::cont::ArrayHandle<viskores::Vec4f_32> colorArray =
    viskores::cont::make_ArrayHandle(fb.data, fb.width * fb.height, viskores::CopyFlag::On);

  anari_cpp::unmap(d, frame, "channel.color");
  anari_cpp::release(d, frame);

  image.AddPointField("color", colorArray);

  VISKORES_TEST_ASSERT(test_equal_images(image, imgName));
}

} // namespace

#endif
