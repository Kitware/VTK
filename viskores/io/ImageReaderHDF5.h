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
#ifndef viskores_io_ImageReaderHDF5_h
#define viskores_io_ImageReaderHDF5_h

#include <viskores/io/ImageReaderBase.h>

namespace viskores
{
namespace io
{
/// \brief Reading images using HDF5 Image format
///
/// \c ImageReaderHDF5 extends viskores::io::ImageWriterBase and implements writing image
/// HDF5 file format. It conforms to the HDF5 Image Specification
/// https://portal.hdfgroup.org/display/HDF5/HDF5+Image+and+Palette+Specification%2C+Version+1.2
class VISKORES_IO_EXPORT ImageReaderHDF5 : public ImageReaderBase
{
  using Superclass = ImageReaderBase;

public:
  using Superclass::Superclass;
  VISKORES_CONT ~ImageReaderHDF5() noexcept override;
  ImageReaderHDF5(const ImageReaderHDF5&) = delete;
  ImageReaderHDF5& operator=(const ImageReaderHDF5&) = delete;

protected:
  VISKORES_CONT void Read() override;

private:
};
}
}
#endif // vviskores_io_ImageReaderHDF5_h
