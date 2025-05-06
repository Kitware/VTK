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

#include <viskores/io/ImageReaderPNG.h>

#include <viskores/io/PixelTypes.h>

VISKORES_THIRDPARTY_PRE_INCLUDE
#include <viskores/thirdparty/lodepng/viskoreslodepng/lodepng.h>
VISKORES_THIRDPARTY_POST_INCLUDE

namespace
{

VISKORES_CONT
template <typename PixelType>
viskores::io::ImageReaderBase::ColorArrayType ReadFromPNG(const std::string& fileName,
                                                          viskores::Id& width,
                                                          viskores::Id& height)
{
  unsigned char* imageData;
  unsigned uwidth, uheight;
  viskores::png::lodepng_decode_file(
    &imageData,
    &uwidth,
    &uheight,
    fileName.c_str(),
    static_cast<viskores::png::LodePNGColorType>(PixelType::GetColorType()),
    PixelType::GetBitDepth());

  width = static_cast<viskores::Id>(uwidth);
  height = static_cast<viskores::Id>(uheight);

  // Fill in the data starting from the end (Images are read Top-Left to Bottom-Right,
  // but are stored from Bottom-Left to Top-Right)
  viskores::cont::ArrayHandle<viskores::Vec4f_32> array;
  array.Allocate(width * height);
  auto portal = array.WritePortal();
  viskores::Id viskoresIndex = 0;
  for (viskores::Id yIndex = static_cast<viskores::Id>(height - 1); yIndex >= 0; yIndex--)
  {
    for (viskores::Id xIndex = 0; xIndex < static_cast<viskores::Id>(width); xIndex++)
    {
      viskores::Id pngIndex = static_cast<viskores::Id>(yIndex * width + xIndex);
      portal.Set(viskoresIndex, PixelType(imageData, pngIndex).ToVec4f());
      viskoresIndex++;
    }
  }

  free(imageData);
  return array;
}

} // anonymous namespace

namespace viskores
{
namespace io
{

ImageReaderPNG::~ImageReaderPNG() noexcept {}

void ImageReaderPNG::Read()
{
  viskores::Id width;
  viskores::Id height;
  viskores::io::ImageReaderBase::ColorArrayType pixelArray =
    ReadFromPNG<viskores::io::RGBPixel_16>(this->FileName, width, height);

  this->InitializeImageDataSet(width, height, pixelArray);
}
}
} // namespace viskores::io
