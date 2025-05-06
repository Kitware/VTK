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

#include <viskores/io/DecodePNG.h>

#include <viskores/cont/Logging.h>
#include <viskores/internal/Configure.h>

VISKORES_THIRDPARTY_PRE_INCLUDE
#include <viskores/thirdparty/lodepng/viskoreslodepng/lodepng.h>
VISKORES_THIRDPARTY_POST_INCLUDE

namespace viskores
{
namespace io
{

viskores::UInt32 DecodePNG(std::vector<unsigned char>& out_image,
                           unsigned long& image_width,
                           unsigned long& image_height,
                           const unsigned char* in_png,
                           std::size_t in_size)
{
  using namespace viskores::png;
  constexpr std::size_t bitdepth = 8;
  viskores::UInt32 iw = 0;
  viskores::UInt32 ih = 0;

  auto retcode = lodepng::decode(out_image, iw, ih, in_png, in_size, LCT_RGBA, bitdepth);
  image_width = iw;
  image_height = ih;
  return retcode;
}
}
} // namespace viskores::io
