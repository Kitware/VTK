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

#include <viskores/io/ImageReaderPNM.h>

#include <viskores/io/PixelTypes.h>

VISKORES_THIRDPARTY_PRE_INCLUDE
#include <viskores/thirdparty/lodepng/viskoreslodepng/lodepng.h>
VISKORES_THIRDPARTY_POST_INCLUDE

namespace viskores
{
namespace io
{

ImageReaderPNM::~ImageReaderPNM() noexcept {}

VISKORES_CONT
void ImageReaderPNM::Read()
{
  std::ifstream inStream(this->FileName.c_str(), std::ios_base::binary | std::ios_base::in);

  // Currently, the only magic number supported is P6
  std::string magicNum;
  inStream >> magicNum;
  if (magicNum != "P6")
  {
    throw viskores::cont::ErrorBadValue("MagicNumber: " + magicNum + " in file: " + this->FileName +
                                        " did not match: P6");
  }

  viskores::Id width;
  viskores::Id height;
  viskores::Id maxColorValue;
  inStream >> width >> height >> maxColorValue;
  inStream.get();

  if ((maxColorValue > 0) && (maxColorValue <= 255))
  {
    this->DecodeFile<viskores::io::RGBPixel_8>(inStream, width, height);
  }
  else if ((maxColorValue > 255) && (maxColorValue <= 65535))
  {
    this->DecodeFile<viskores::io::RGBPixel_16>(inStream, width, height);
  }
  else
  {
    throw viskores::cont::ErrorBadValue("MaxColorValue: " + std::to_string(maxColorValue) +
                                        " from file: " + this->FileName +
                                        " is not in valid range of [1, 65535]");
  }
}

VISKORES_CONT
template <typename PixelType>
void ImageReaderPNM::DecodeFile(std::ifstream& inStream,
                                const viskores::Id& width,
                                const viskores::Id& height)
{
  viskores::UInt32 imageSize =
    static_cast<viskores::UInt32>(width * height * PixelType::BYTES_PER_PIXEL);
  std::vector<unsigned char> imageData(imageSize);
  inStream.read(reinterpret_cast<char*>(imageData.data()), imageSize);

  // Fill in the data starting from the end (Images are read Top-Left to Bottom-Right,
  // but are stored from Bottom-Left to Top-Right)
  viskores::io::ImageReaderBase::ColorArrayType array;
  array.Allocate(width * height);
  auto portal = array.WritePortal();
  viskores::Id viskoresIndex = 0;
  for (viskores::Id yIndex = height - 1; yIndex >= 0; yIndex--)
  {
    for (viskores::Id xIndex = 0; xIndex < width; xIndex++)
    {
      viskores::Id pnmIndex = yIndex * width + xIndex;
      portal.Set(viskoresIndex, PixelType(imageData.data(), pnmIndex).ToVec4f());
      viskoresIndex++;
    }
  }

  this->InitializeImageDataSet(width, height, array);
}
}
} // namespace viskores::io
