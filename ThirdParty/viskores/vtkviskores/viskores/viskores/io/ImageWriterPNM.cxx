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

#include <viskores/io/ImageWriterPNM.h>

#include <viskores/io/PixelTypes.h>

namespace viskores
{
namespace io
{

ImageWriterPNM::~ImageWriterPNM() noexcept {}

void ImageWriterPNM::Write(viskores::Id width, viskores::Id height, const ColorArrayType& pixels)
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
void ImageWriterPNM::WriteToFile(viskores::Id width,
                                 viskores::Id height,
                                 const ColorArrayType& pixels)
{
  std::ofstream outStream(this->FileName.c_str(), std::ios_base::binary | std::ios_base::out);
  outStream << "P6\n" << width << " " << height << "\n";

  outStream << PixelType::MAX_COLOR_VALUE << "\n";
  auto pixelPortal = pixels.ReadPortal();

  viskores::UInt32 imageSize =
    static_cast<viskores::UInt32>(pixels.GetNumberOfValues() * PixelType::BYTES_PER_PIXEL);
  std::vector<unsigned char> imageData(imageSize);

  // Write out the data starting from the end (Images are stored Bottom-Left to Top-Right,
  // but are viewed from Top-Left to Bottom-Right)
  viskores::Id pnmIndex = 0;
  for (viskores::Id yIndex = height - 1; yIndex >= 0; yIndex--)
  {
    for (viskores::Id xIndex = 0; xIndex < width; xIndex++, pnmIndex++)
    {
      viskores::Id viskoresIndex = yIndex * width + xIndex;
      PixelType(pixelPortal.Get(viskoresIndex))
        .FillImageAtIndexWithPixel(imageData.data(), pnmIndex);
    }
  }
  outStream.write((char*)imageData.data(), imageSize);
  outStream.close();
}
}
} // namespace viskores::io
