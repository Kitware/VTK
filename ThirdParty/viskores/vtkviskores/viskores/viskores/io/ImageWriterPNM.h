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
#ifndef viskores_io_ImageWriterPNM_h
#define viskores_io_ImageWriterPNM_h

#include <viskores/io/ImageWriterBase.h>

namespace viskores
{
namespace io
{

/// \brief Writes images using the PNM format.
///
/// `ImageWriterPNM` is constructed with the name of the file to write. The data
/// is written to the file by calling the `WriteDataSet` method.
///
/// `ImageWriterPNM` writes images in PNM format (for magic number P6).
/// These files are most commonly stored with a `.ppm` extension although the
/// `.pnm` extension is also valid. More details on the PNM format can be found at
/// http://netpbm.sourceforge.net/doc/ppm.html
///
class VISKORES_IO_EXPORT ImageWriterPNM : public viskores::io::ImageWriterBase
{
  using Superclass = viskores::io::ImageWriterBase;

public:
  using Superclass::Superclass;
  VISKORES_CONT ~ImageWriterPNM() noexcept override;
  ImageWriterPNM(const ImageWriterPNM&) = delete;
  ImageWriterPNM& operator=(const ImageWriterPNM&) = delete;

  /// Attempts to write the ImageDataSet to a PNM file. The MaxColorValue
  /// set in the file with either be selected from the stored MaxColorValue
  /// member variable, or from the templated type if MaxColorValue hasn't been
  /// set from a read file.
  ///
  VISKORES_CONT void Write(viskores::Id width,
                           viskores::Id height,
                           const ColorArrayType& pixels) override;

protected:
  template <typename PixelType>
  VISKORES_CONT void WriteToFile(viskores::Id width,
                                 viskores::Id height,
                                 const ColorArrayType& pixels);
};
}
} // namespace viskores::io

#endif //viskores_io_ImageWriterPNM_h
