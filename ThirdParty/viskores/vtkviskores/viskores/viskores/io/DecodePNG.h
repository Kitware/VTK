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
#ifndef viskores_io_DecodePNG_h
#define viskores_io_DecodePNG_h

#include <viskores/Types.h>
#include <viskores/io/viskores_io_export.h>

#include <vector>

namespace viskores
{
namespace io
{

/// Decodes a PNG file buffer in memory, into a raw pixel buffer
/// Output is RGBA 32-bit (8 bit per channel) color format
/// no matter what color type the original PNG image had. This gives predictable,
/// usable data from any random input PNG.
///
VISKORES_IO_EXPORT
viskores::UInt32 DecodePNG(std::vector<unsigned char>& out_image,
                           unsigned long& image_width,
                           unsigned long& image_height,
                           const unsigned char* in_png,
                           std::size_t in_size);
}
} // viskores::io

#endif //viskores_io_DecodePNG_h
