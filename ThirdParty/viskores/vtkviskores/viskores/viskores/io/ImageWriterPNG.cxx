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

#include <viskores/io/ImageWriterPNG.h>

#include <viskores/io/PixelTypes.h>

VISKORES_THIRDPARTY_PRE_INCLUDE
#include <viskores/thirdparty/lodepng/viskoreslodepng/lodepng.h>
VISKORES_THIRDPARTY_POST_INCLUDE

namespace viskores
{
namespace io
{

ImageWriterPNG::~ImageWriterPNG() noexcept {}

void ImageWriterPNG::Write(viskores::Id width, viskores::Id height, const ColorArrayType& pixels)
{
  switch (this->Depth)
  {
    case PixelDepth::PIXEL_8:
      this->WriteToFile<viskores::io::RGBPixel_8>(width, height, pixels);
      break;
    case PixelDepth::PIXEL_16:
      WriteToFile<viskores::io::RGBPixel_16>(width, height, pixels);
      break;
  }
}

template <typename PixelType>
void ImageWriterPNG::WriteToFile(viskores::Id width,
                                 viskores::Id height,
                                 const ColorArrayType& pixels)
{
  auto pixelPortal = pixels.ReadPortal();
  std::vector<unsigned char> imageData(static_cast<typename std::vector<unsigned char>::size_type>(
    pixels.GetNumberOfValues() * PixelType::BYTES_PER_PIXEL));

  // Write out the data starting from the end (Images are stored Bottom-Left to Top-Right,
  // but are viewed from Top-Left to Bottom-Right)
  viskores::Id pngIndex = 0;
  for (viskores::Id yIndex = height - 1; yIndex >= 0; yIndex--)
  {
    for (viskores::Id xIndex = 0; xIndex < width; xIndex++)
    {
      viskores::Id viskoresIndex = yIndex * width + xIndex;
      PixelType(pixelPortal.Get(viskoresIndex))
        .FillImageAtIndexWithPixel(imageData.data(), pngIndex);
      pngIndex++;
    }
  }

  viskores::png::lodepng_encode_file(
    this->FileName.c_str(),
    imageData.data(),
    static_cast<unsigned>(width),
    static_cast<unsigned>(height),
    static_cast<viskores::png::LodePNGColorType>(PixelType::GetColorType()),
    PixelType::GetBitDepth());
}
}
} // namespace viskores::io
