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
#include <viskores/io/PixelTypes.h>

#include <string>

using namespace viskores::io;

template <typename PixelType>
void TestPixelTypeOperations(const viskores::UInt16& numPixels = 10)
{
  using ValType = typename PixelType::ComponentType;
  const ValType numBytes = static_cast<ValType>(PixelType::NUM_BYTES);
  const ValType numChannels = static_cast<ValType>(PixelType::NUM_CHANNELS);

  // Fill in the imageData through FillPixelData
  std::vector<unsigned char> imageData(numPixels * numBytes * numChannels);
  std::vector<PixelType> pixelVector(numPixels);
  for (ValType i = 0; i < numPixels; i++)
  {
    ValType pixelVal = 0;
    for (ValType j = 0, shift = numBytes - 1; j < numBytes; shift--, j++)
    {
      pixelVal += (i + j) << (shift * 8);
    }

    PixelType pixel(pixelVal);
    pixelVector[i] = pixel;
    pixel.FillImageAtIndexWithPixel(imageData.data(), i);
  }

  // Test that the imageData values were set correctly
  VISKORES_TEST_ASSERT(static_cast<viskores::Id>(imageData.size()) ==
                         numPixels * numChannels * numBytes,
                       "Wrong number of elements");
  for (ValType j = 0; j < numBytes; j++)
  {
    for (ValType i = 0; i < numPixels; i++)
    {
      for (ValType k = numChannels * i; k < numChannels * i + numChannels; k++)
      {
        VISKORES_TEST_ASSERT(imageData[k * numBytes + j] == i + j,
                             "Wrong value at index[" + std::to_string(k * numBytes + j) +
                               "]: " + std::to_string(imageData[k * numBytes + j]) +
                               " != " + std::to_string(i + j));
      }
    }
  }

  // Test that a pixel can be retreived from the filled out data vector
  for (viskores::Id i = 0; i < numPixels; i++)
  {
    VISKORES_TEST_ASSERT(pixelVector[static_cast<typename std::vector<PixelType>::size_type>(i)] ==
                           PixelType(imageData.data(), i),
                         "Incorrect pixel value");
  }
}

void TestDifferentPixelTypes()
{
  VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Testing 8 bit RGB");
  TestPixelTypeOperations<RGBPixel_8>();

  VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Testing 8 bit Grey");
  TestPixelTypeOperations<GreyPixel_8>();

  VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Testing 16 bit RGB");
  TestPixelTypeOperations<RGBPixel_16>();

  VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Testing 16 bit Grey");
  TestPixelTypeOperations<GreyPixel_16>();
}

void TestGreyPixelConstructors()
{
  std::vector<unsigned char> initData{ 1, 2 };

  auto pixel_1 = GreyPixel_8(1);
  auto pixel_2 = GreyPixel_8(1);
  auto pixel_3 = GreyPixel_8(2);
  auto pixel_4 = GreyPixel_8(initData.data(), 0);
  auto pixel_5 = GreyPixel_8(initData.data(), 1);
  auto pixel_6 = GreyPixel_16(initData.data(), 0);

  float color = 10.0f / GreyPixel_16::MAX_COLOR_VALUE;
  auto pixel_7 = GreyPixel_16({ color, color, color, 5 });

  VISKORES_TEST_ASSERT(viskores::UInt16(1) == pixel_1[0], "Type mis-match");
  VISKORES_TEST_ASSERT(viskores::FloatDefault(0) == pixel_1.Diff(pixel_2), "Incorrect Diff");
  VISKORES_TEST_ASSERT(viskores::FloatDefault(1) == pixel_1.Diff(pixel_3), "Incorrect Diff");
  VISKORES_TEST_ASSERT(viskores::Vec4f_32(1.0f / 255, 1.0f / 255, 1.0f / 255, 1) ==
                         pixel_1.ToVec4f(),
                       "Incorrect Conversion");
  VISKORES_TEST_ASSERT(viskores::Vec<viskores::UInt8, 1>(1) == pixel_4,
                       "Bad 1st value 8 bit construct");
  VISKORES_TEST_ASSERT(viskores::Vec<viskores::UInt8, 1>(2) == pixel_5,
                       "Bad 2nd value 8 bit construct");
  VISKORES_TEST_ASSERT(viskores::Vec<viskores::UInt16, 1>(258) == pixel_6, "Bad 16 bit construct");
  VISKORES_TEST_ASSERT(viskores::Vec4f_32(258.0f / 65535, 258.0f / 65535, 258.0f / 65535, 1) ==
                         pixel_6.ToVec4f(),
                       "Incorrect Conversion");
  VISKORES_TEST_ASSERT(viskores::Vec<viskores::UInt16, 1>(10) == pixel_7,
                       "Bad Vec4f_32 construction");

  VISKORES_TEST_ASSERT(GreyPixel<16>::GetBitDepth() == 16, "Bad BitDepth");
  VISKORES_TEST_ASSERT(GreyPixel<16>::BIT_DEPTH == 16, "Bad BitDepth");
  VISKORES_TEST_ASSERT(GreyPixel<16>::NUM_BYTES == 2, "Bad NumBytes");
  VISKORES_TEST_ASSERT(GreyPixel<16>::MAX_COLOR_VALUE == 65535, "Bad NumBytes");
  VISKORES_TEST_ASSERT(GreyPixel<16>::NUM_CHANNELS == 1, "Bad NumChannels");
  VISKORES_TEST_ASSERT(GreyPixel<16>::BYTES_PER_PIXEL == 2, "Wrong Pixel Byte distance");

  // Shouldn't compile
  // auto pixel_4 = RGBPixel_8(1, 1, 1);
  // pixel_1.Diff(pixel_4);
}

void TestRGBPixelConstructors()
{
  std::vector<unsigned char> initData{ 1, 2, 3, 4, 5, 6 };

  auto pixel_1 = RGBPixel_8(1, 1, 1);
  auto pixel_2 = RGBPixel_8(1, 1, 1);
  auto pixel_3 = RGBPixel_8(1);
  auto pixel_4 = RGBPixel_8(2, 2, 2);
  auto pixel_5 = RGBPixel_8(initData.data(), 0);
  auto pixel_6 = RGBPixel_8(initData.data(), 1);
  auto pixel_7 = RGBPixel_16(initData.data(), 0);

  float color = 10.0f / RGBPixel_16::MAX_COLOR_VALUE;
  auto pixel_8 = RGBPixel_16({ color, color, color, 5 });

  VISKORES_TEST_ASSERT(viskores::Vec3ui_8(1, 1, 1) == pixel_1, "Type mis-match");
  VISKORES_TEST_ASSERT(viskores::FloatDefault(0) == pixel_1.Diff(pixel_2), "Incorrect Diff");
  VISKORES_TEST_ASSERT(viskores::FloatDefault(0) == pixel_1.Diff(pixel_3), "Incorrect Diff");
  VISKORES_TEST_ASSERT(viskores::FloatDefault(3) == pixel_1.Diff(pixel_4), "Incorrect Diff");
  VISKORES_TEST_ASSERT(viskores::Vec4f_32(1.0f / 255, 1.0f / 255, 1.0f / 255, 1) ==
                         pixel_1.ToVec4f(),
                       "Incorrect Conversion");
  VISKORES_TEST_ASSERT(viskores::Vec3ui_8(1, 2, 3) == pixel_5, "Bad 1st value 8 bit construct");
  VISKORES_TEST_ASSERT(viskores::Vec3ui_8(4, 5, 6) == pixel_6, "Bad 2nd value 8 bit construct");
  VISKORES_TEST_ASSERT(viskores::Vec3ui_16(258, 772, 1286) == pixel_7, "Bad 16 bit construct");
  VISKORES_TEST_ASSERT(viskores::Vec4f_32(258.0f / 65535, 772.0f / 65535, 1286.0f / 65535, 1) ==
                         pixel_7.ToVec4f(),
                       "Incorrect Conversion");
  VISKORES_TEST_ASSERT(viskores::Vec<viskores::UInt16, 3>(10, 10, 10) == pixel_8,
                       "Bad Vec4f_32 construction");

  VISKORES_TEST_ASSERT(RGBPixel<16>::GetBitDepth() == 16, "Bad BitDepth");
  VISKORES_TEST_ASSERT(RGBPixel<16>::BIT_DEPTH == 16, "Bad BitDepth");
  VISKORES_TEST_ASSERT(RGBPixel<16>::NUM_BYTES == 2, "Bad NumBytes");
  VISKORES_TEST_ASSERT(RGBPixel<16>::MAX_COLOR_VALUE == 65535, "Bad NumBytes");
  VISKORES_TEST_ASSERT(RGBPixel<16>::NUM_CHANNELS == 3, "Bad NumChannels");
  VISKORES_TEST_ASSERT(RGBPixel<16>::BYTES_PER_PIXEL == 6, "Wrong Pixel Byte distance");

  // Shouldn't compile
  // auto pixel_8 = GreyPixel_8(1);
  // pixel_1.Diff(pixel_8);
}

void TestPixelTypes()
{
  VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Testing RGBPixel");
  TestRGBPixelConstructors();

  VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Testing GreyPixel");
  TestGreyPixelConstructors();

  VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Testing Pixel Types");
  TestDifferentPixelTypes();
}

int UnitTestPixelTypes(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestPixelTypes, argc, argv);
}
