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
#ifndef viskores_io_ImageWriterPNG_h
#define viskores_io_ImageWriterPNG_h

#include <viskores/io/ImageWriterBase.h>

namespace viskores
{
namespace io
{

/// @brief Writes images using the PNG format.
///
/// `ImageWriterPNG` is constructed with the name of the file to write. The data
/// is written to the file by calling the `WriteDataSet` method.
///
/// When writing files, `ImageReaderPNG` automatically compresses data to optimal
/// sizes relative to the actual bit complexity of the provided image.
///
///
class VISKORES_IO_EXPORT ImageWriterPNG : public viskores::io::ImageWriterBase
{
  using Superclass = viskores::io::ImageWriterBase;

public:
  using Superclass::Superclass;
  VISKORES_CONT ~ImageWriterPNG() noexcept override;
  ImageWriterPNG(const ImageWriterPNG&) = delete;
  ImageWriterPNG& operator=(const ImageWriterPNG&) = delete;

protected:
  VISKORES_CONT void Write(viskores::Id width,
                           viskores::Id height,
                           const ColorArrayType& pixels) override;

  template <typename PixelType>
  VISKORES_CONT void WriteToFile(viskores::Id width,
                                 viskores::Id height,
                                 const ColorArrayType& pixels);
};
}
} // namespace viskores::io

#endif //viskores_io_ImageWriterPNG_h
