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
#include <viskores/io/ErrorIO.h>
#include <viskores/io/ImageReaderHDF5.h>
#include <viskores/io/PixelTypes.h>

#include <hdf5.h>
#include <hdf5_hl.h>

namespace viskores
{
namespace io
{

ImageReaderHDF5::~ImageReaderHDF5() noexcept = default;

void ImageReaderHDF5::Read()
{
  // need to find width, height and pixel type.
  auto fileid = H5Fopen(this->FileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

  const auto fieldName = this->PointFieldName.c_str();
  if (!H5IMis_image(fileid, fieldName))
  {
    throw viskores::io::ErrorIO{ "Not an HDF5 image file" };
  }

  hsize_t width, height, nplanes;
  hssize_t npals;
  char interlace[16];
  if (H5IMget_image_info(fileid, fieldName, &width, &height, &nplanes, interlace, &npals) < 0)
  {
    throw viskores::io ::ErrorIO{ "Can not get image info" };
  }

  // We don't use the H5IMread_image() since it only supports 8 bit pixel.
  hid_t did;
  if ((did = H5Dopen2(fileid, fieldName, H5P_DEFAULT)) < 0)
  {
    throw viskores::io::ErrorIO{ "Can not open image dataset" };
  }

  if (strncmp(interlace, "INTERLACE_PIXEL", 15) != 0)
  {
    std::string message = "Unsupported interlace mode: ";
    message += interlace;
    message +=
      ". Currently, only the INTERLACE_PIXEL mode is supported. See "
      "https://portal.hdfgroup.org/display/HDF5/HDF5+Image+and+Palette+Specification%2C+Version+1.2"
      " for more details on the HDF5 image convention.";
    throw viskores::io::ErrorIO{ message };
  }

  std::vector<unsigned char> buffer;
  auto type_size = H5LDget_dset_type_size(did, nullptr);
  buffer.resize(width * height * 3 * type_size);
  switch (type_size)
  {
    case 1:
      H5Dread(did, H5T_NATIVE_UCHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());
      break;
    case 2:
      H5Dread(did, H5T_NATIVE_UINT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());
      break;
    default:
      throw viskores::io::ErrorIO{ "Unsupported pixel type" };
  }

  H5Dclose(did);
  H5Fclose(fileid);

  // convert PixelType to Vec4f_32
  viskores::cont::ArrayHandle<viskores::Vec4f_32> pixelArray;
  pixelArray.Allocate(width * height);
  auto portal = pixelArray.WritePortal();
  viskores::Id viskoresIndex = 0;
  for (viskores::Id yIndex = 0; yIndex < static_cast<viskores::Id>(height); yIndex++)
  {
    for (viskores::Id xIndex = 0; xIndex < static_cast<viskores::Id>(width); xIndex++)
    {
      viskores::Id hdfIndex = static_cast<viskores::Id>(yIndex * width + xIndex);
      if (type_size == 1)
      {
        portal.Set(viskoresIndex, viskores::io::RGBPixel_8(buffer.data(), hdfIndex).ToVec4f());
      }
      else
      {
        portal.Set(viskoresIndex, viskores::io::RGBPixel_16(buffer.data(), hdfIndex).ToVec4f());
      }
      viskoresIndex++;
    }
  }

  this->InitializeImageDataSet(width, height, pixelArray);
} // Read()

}
}
