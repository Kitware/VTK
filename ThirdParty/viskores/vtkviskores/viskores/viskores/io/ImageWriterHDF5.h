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
#ifndef viskores_io_ImageWriterHDF5_H
#define viskores_io_ImageWriterHDF5_H

#include <viskores/io/ImageWriterBase.h>

namespace viskores
{
namespace io
{

/// \brief Writing images using HDF5 Image format
///
/// \c ImageWriterHDF5 extends viskores::io::ImageWriterBase and implements writing image
/// HDF5 file format. It conforms to the HDF5 Image Specification
/// https://portal.hdfgroup.org/display/HDF5/HDF5+Image+and+Palette+Specification%2C+Version+1.2
class VISKORES_IO_EXPORT ImageWriterHDF5 : public viskores::io::ImageWriterBase
{
  using Superclass = viskores::io::ImageWriterBase;

public:
  using Superclass::Superclass;
  VISKORES_CONT ~ImageWriterHDF5() noexcept override;
  ImageWriterHDF5(const ImageWriterHDF5&) = delete;
  ImageWriterHDF5& operator=(const ImageWriterHDF5&) = delete;

  VISKORES_CONT void WriteDataSet(const viskores::cont::DataSet& dataSet,
                                  const std::string& colorField = {});

protected:
  VISKORES_CONT void Write(viskores::Id width,
                           viskores::Id height,
                           const ColorArrayType& pixels) override;

private:
  template <typename PixelType>
  VISKORES_CONT int WriteToFile(viskores::Id width,
                                viskores::Id height,
                                const ColorArrayType& pixels);

  std::int64_t fileid = 0;
  // FIXME: a hack for the moment, design a better API.
  std::string fieldName;

  static constexpr auto IMAGE_CLASS = "IMAGE";
  static constexpr auto IMAGE_VERSION = "1.2";
};
}
}
#endif //viskores_io_ImageWriterHDF5_H
