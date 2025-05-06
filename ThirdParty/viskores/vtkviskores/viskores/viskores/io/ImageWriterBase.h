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
#ifndef viskores_io_ImageWriterBase_h
#define viskores_io_ImageWriterBase_h

#include <viskores/cont/DataSet.h>

#include <viskores/io/viskores_io_export.h>

namespace viskores
{
namespace io
{

/// \brief Manages writing, and loading data from images
///
/// `ImageWriterBase` implements methods for loading imaging data from a canvas or
/// ArrayHandle and storing that data in a viskores::cont::DataSet.  Image RGB values
/// are represented as a point field in a 2D uniform dataset.
///
/// `ImageWriterBase` can be constructed from a file, canvas, or ArrayHandle.  It can
/// also be empy constructed and filled in with a dataset later.
///
/// `ImageWriterBase` implements virtual methods for writing files.  Ideally,
/// these methods will be overriden in various subclasses to implement specific
/// functionality for writing data to specific image file-types.
///
class VISKORES_IO_EXPORT ImageWriterBase
{
public:
  using ColorArrayType = viskores::cont::ArrayHandle<viskores::Vec4f_32>;

  VISKORES_CONT ImageWriterBase(const char* filename);
  /// @brief Construct a writer to save data to the given file.
  VISKORES_CONT ImageWriterBase(const std::string& filename);
  VISKORES_CONT virtual ~ImageWriterBase() noexcept;
  ImageWriterBase(const ImageWriterBase&) = delete;
  ImageWriterBase& operator=(const ImageWriterBase&) = delete;

  /// \brief Write the color field of a data set to an image file.
  ///
  /// The `DataSet` must have a 2D structured cell set.
  ///
  /// The specified color field must be of type `ColorArrayType` (a basic
  /// `ArrayHandle` of `viskores::Vec4f_32`). If no color field name is given,
  /// the first point field that matches this criteria is written.
  ///
  VISKORES_CONT virtual void WriteDataSet(const viskores::cont::DataSet& dataSet,
                                          const std::string& colorField = {});

  enum class PixelDepth
  {
    PIXEL_8,
    PIXEL_16
  };

  /// @brief Specify the number of bits used by each color channel.
  VISKORES_CONT PixelDepth GetPixelDepth() const { return this->Depth; }
  /// @brief Specify the number of bits used by each color channel.
  VISKORES_CONT void SetPixelDepth(PixelDepth depth) { this->Depth = depth; }

  VISKORES_CONT const std::string& GetFileName() const { return this->FileName; }
  VISKORES_CONT void SetFileName(const std::string& filename) { this->FileName = filename; }

protected:
  std::string FileName;
  PixelDepth Depth = PixelDepth::PIXEL_8;

  VISKORES_CONT virtual void Write(viskores::Id width,
                                   viskores::Id height,
                                   const ColorArrayType& pixels) = 0;
};
}
}

#endif //viskores_io_ImageWriterBase_h
