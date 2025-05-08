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
#include <viskores/io/EncodePNG.h>
#include <viskores/io/FileUtils.h>

#include <viskores/cont/Logging.h>
#include <viskores/internal/Configure.h>

VISKORES_THIRDPARTY_PRE_INCLUDE
#include <viskores/thirdparty/lodepng/viskoreslodepng/lodepng.h>
VISKORES_THIRDPARTY_POST_INCLUDE

namespace viskores
{
namespace io
{

viskores::UInt32 EncodePNG(std::vector<unsigned char> const& image,
                           unsigned long width,
                           unsigned long height,
                           std::vector<unsigned char>& output_png)
{
  // The default is 8 bit RGBA; does anyone care to have more options?
  // We can certainly add them in a backwards-compatible way if need be.
  viskores::UInt32 error = viskores::png::lodepng::encode(
    output_png, image, static_cast<unsigned int>(width), static_cast<unsigned int>(height));
  if (error)
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Error,
                   "LodePNG Encoder error number " << error << ": "
                                                   << png::lodepng_error_text(error));
  }
  return error;
}


viskores::UInt32 SavePNG(std::string const& filename,
                         std::vector<unsigned char> const& image,
                         unsigned long width,
                         unsigned long height)
{
  if (!viskores::io::EndsWith(filename, ".png"))
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Error,
                   "File " << filename << " does not end with .png; this is required.");
  }

  std::vector<unsigned char> output_png;
  viskores::UInt32 error = EncodePNG(image, width, height, output_png);
  if (!error)
  {
    viskores::png::lodepng::save_file(output_png, filename);
  }
  return error;
}
}
} // namespace viskores::io
