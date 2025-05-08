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
#ifndef viskores_io_ImageReaderPNM_h
#define viskores_io_ImageReaderPNM_h

#include <viskores/io/ImageReaderBase.h>

namespace viskores
{
namespace io
{

/// \brief Reads images using the PNM format.
///
/// `ImageReaderPNM` is constructed with the name of the file to read. The data
/// from the file is read by calling the `ReadDataSet` method.
///
/// Currently, `ImageReaderPNM` only supports files using the portable pixmap (PPM)
/// format (with magic number ``P6''). These files are most commonly stored with a
/// `.ppm` extension although the `.pnm` extension is also valid.
/// More details on the PNM format can be found here at
/// http://netpbm.sourceforge.net/doc/ppm.html
///
/// By default, the colors are stored in a field named "colors", but the name of the
/// field can optionally be changed using the `SetPointFieldName` method.
///
class VISKORES_IO_EXPORT ImageReaderPNM : public ImageReaderBase
{
  using Superclass = ImageReaderBase;

public:
  using Superclass::Superclass;
  VISKORES_CONT ~ImageReaderPNM() noexcept override;
  ImageReaderPNM(const ImageReaderPNM&) = delete;
  ImageReaderPNM& operator=(const ImageReaderPNM&) = delete;

protected:
  VISKORES_CONT void Read() override;

  /// Reads image data from the provided inStream with the supplied width/height
  /// Stores the data in a vector of PixelType which is converted to an DataSet
  ///
  template <typename PixelType>
  void DecodeFile(std::ifstream& inStream, const viskores::Id& width, const viskores::Id& height);
};
}
} // namespace viskores::io

#endif //viskores_io_ImageReaderPNM_h
