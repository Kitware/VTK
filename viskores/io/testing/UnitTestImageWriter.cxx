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

#include <viskores/cont/testing/Testing.h>
#include <viskores/io/ImageReaderPNG.h>
#include <viskores/io/ImageReaderPNM.h>
#include <viskores/io/ImageWriterPNG.h>
#include <viskores/io/ImageWriterPNM.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/Color.h>

#include <string>

namespace
{

using namespace viskores::io;
using namespace viskores::rendering;

void TestFilledImage(viskores::cont::DataSet& dataSet,
                     const std::string& fieldName,
                     const viskores::rendering::Canvas& canvas)
{
  VISKORES_TEST_ASSERT(dataSet.HasPointField(fieldName), "Point Field Not Found: " + fieldName);

  auto pointField = dataSet.GetPointField(fieldName);
  VISKORES_TEST_ASSERT(pointField.GetNumberOfValues() == canvas.GetWidth() * canvas.GetHeight(),
                       "wrong image dimensions");
  VISKORES_TEST_ASSERT(
    pointField.GetData().template IsType<viskores::cont::ArrayHandle<viskores::Vec4f_32>>(),
    "wrong ArrayHandle type");
  auto pixelPortal = pointField.GetData()
                       .template AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec4f_32>>()
                       .ReadPortal();

  auto colorPortal = canvas.GetColorBuffer().ReadPortal();

  VISKORES_TEST_ASSERT(test_equal_portals(pixelPortal, colorPortal));
}

void TestCreateImageDataSet(const viskores::rendering::Canvas& canvas)
{
  std::cout << "TestCreateImageDataSet" << std::endl;
  auto dataSet = canvas.GetDataSet("pixel-color");
  TestFilledImage(dataSet, "pixel-color", canvas);
}

void TestReadAndWritePNG(const viskores::rendering::Canvas& canvas,
                         std::string filename,
                         viskores::io::ImageWriterBase::PixelDepth pixelDepth)
{
  std::cout << "TestReadAndWritePNG - " << filename << std::endl;
  bool throws = false;
  try
  {
    viskores::io::ImageWriterPNG writer(filename);
    viskores::cont::DataSet dataSet;
    writer.WriteDataSet(dataSet);
  }
  catch (const viskores::cont::Error&)
  {
    throws = true;
  }
  VISKORES_TEST_ASSERT(throws, "Fill Image did not throw with empty data");

  {
    viskores::io::ImageWriterPNG writer(filename);
    writer.SetPixelDepth(pixelDepth);
    writer.WriteDataSet(canvas.GetDataSet());
  }
  {
    viskores::io::ImageReaderPNG reader(filename);
    viskores::cont::DataSet dataSet = reader.ReadDataSet();
  }
  {
    viskores::io::ImageWriterPNG writer(filename);
    writer.SetPixelDepth(pixelDepth);
    writer.WriteDataSet(canvas.GetDataSet());
  }
  {
    viskores::io::ImageReaderPNG reader(filename);
    viskores::cont::DataSet dataSet = reader.ReadDataSet();
    TestFilledImage(dataSet, reader.GetPointFieldName(), canvas);
  }
}

void TestReadAndWritePNM(const viskores::rendering::Canvas& canvas,
                         std::string filename,
                         viskores::io::ImageWriterBase::PixelDepth pixelDepth)
{
  std::cout << "TestReadAndWritePNM - " << filename << std::endl;
  bool throws = false;
  try
  {
    viskores::io::ImageWriterPNM writer(filename);
    viskores::cont::DataSet dataSet;
    writer.WriteDataSet(dataSet);
  }
  catch (const viskores::cont::Error&)
  {
    throws = true;
  }
  VISKORES_TEST_ASSERT(throws, "Fill Image did not throw with empty data");

  {
    viskores::io::ImageWriterPNM writer(filename);
    writer.SetPixelDepth(pixelDepth);
    writer.WriteDataSet(canvas.GetDataSet());
  }
  {
    viskores::io::ImageReaderPNM reader(filename);
    viskores::cont::DataSet dataSet = reader.ReadDataSet();
  }
  {
    viskores::io::ImageWriterPNM writer(filename);
    writer.SetPixelDepth(pixelDepth);
    writer.WriteDataSet(canvas.GetDataSet());
  }
  {
    viskores::io::ImageReaderPNM reader(filename);
    viskores::cont::DataSet dataSet = reader.ReadDataSet();
    TestFilledImage(dataSet, reader.GetPointFieldName(), canvas);
  }
}

void TestBaseImageMethods(const viskores::rendering::Canvas& canvas)
{
  TestCreateImageDataSet(canvas);
}

void TestPNMImage(const viskores::rendering::Canvas& canvas)
{
  TestReadAndWritePNM(
    canvas, "pnmRGB8Test.png", viskores::io::ImageWriterBase::PixelDepth::PIXEL_8);
  TestReadAndWritePNM(
    canvas, "pnmRGB16Test.png", viskores::io::ImageWriterBase::PixelDepth::PIXEL_16);
}

void TestPNGImage(const viskores::rendering::Canvas& canvas)
{
  TestReadAndWritePNG(
    canvas, "pngRGB8Test.png", viskores::io::ImageWriterBase::PixelDepth::PIXEL_8);
  TestReadAndWritePNG(
    canvas, "pngRGB16Test.png", viskores::io::ImageWriterBase::PixelDepth::PIXEL_16);
}

void TestImage()
{
  viskores::rendering::Canvas canvas(16, 16);
  canvas.SetBackgroundColor(viskores::rendering::Color::red);
  canvas.Clear();
  // Line from top left to bottom right, ensures correct transposedness
  canvas.AddLine(-0.9, 0.9, 0.9, -0.9, 2.0f, viskores::rendering::Color::black);
  viskores::Bounds colorBarBounds(-0.8, -0.6, -0.8, 0.8, 0, 0);
  canvas.AddColorBar(colorBarBounds, viskores::cont::ColorTable("inferno"), false);
  canvas.BlendBackground();
  canvas.SaveAs("baseline.ppm");

  TestBaseImageMethods(canvas);
  TestPNMImage(canvas);
  TestPNGImage(canvas);
}
}

int UnitTestImageWriter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestImage, argc, argv);
}
