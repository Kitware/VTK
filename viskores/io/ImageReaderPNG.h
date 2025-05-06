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
#ifndef viskores_io_ImageReaderPNG_h
#define viskores_io_ImageReaderPNG_h

#include <viskores/io/ImageReaderBase.h>

namespace viskores
{
namespace io
{

/// @brief Reads images using the PNG format.
///
/// `ImageReaderPNG` is constructed with the name of the file to read. The data
/// from the file is read by calling the `ReadDataSet` method.
///
/// `ImageReaderPNG` will automatically upsample/downsample read image data
/// to a 16 bit RGB no matter how the image is compressed. It is up to the user to
/// decide the pixel format for input PNGs
///
/// By default, the colors are stored in a field named "colors", but the name of the
/// field can optionally be changed using the `SetPointFieldName` method.
class VISKORES_IO_EXPORT ImageReaderPNG : public ImageReaderBase
{
  using Superclass = ImageReaderBase;

public:
  using Superclass::Superclass;
  VISKORES_CONT ~ImageReaderPNG() noexcept override;
  ImageReaderPNG(const ImageReaderPNG&) = delete;
  ImageReaderPNG& operator=(const ImageReaderPNG&) = delete;

protected:
  VISKORES_CONT void Read() override;
};

}
} // namespace viskores::io

#endif //viskores_io_ImageReaderPNG_h
