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
#ifndef viskores_io_ImageReaderBase_h
#define viskores_io_ImageReaderBase_h

#include <viskores/cont/DataSet.h>

#include <viskores/io/viskores_io_export.h>

namespace viskores
{
namespace io
{

/// \brief Manages reading, and loading data from images
///
/// `ImageReaderBase` implements methods for loading imaging data from a canvas or
/// ArrayHandle and storing that data in a viskores::cont::DataSet.  Image RGB values
/// are represented as a point field in a 2D uniform dataset.
///
/// `ImageReaderBase` implements virtual methods for reading files.  Ideally,
/// these methods will be overriden in various subclasses to implement specific
/// functionality for reading data to specific image file-types.
///
class VISKORES_IO_EXPORT ImageReaderBase
{
public:
  using ColorArrayType = viskores::cont::ArrayHandle<viskores::Vec4f_32>;

  explicit VISKORES_CONT ImageReaderBase(const char* filename);
  /// @brief Construct a reader to load data from the given file.
  explicit VISKORES_CONT ImageReaderBase(const std::string& filename);
  virtual VISKORES_CONT ~ImageReaderBase() noexcept;
  ImageReaderBase(const ImageReaderBase&) = delete;
  ImageReaderBase& operator=(const ImageReaderBase&) = delete;

  /// @brief Load data from the file and return it in a `DataSet` object.
  VISKORES_CONT const viskores::cont::DataSet& ReadDataSet();

  VISKORES_CONT const viskores::cont::DataSet& GetDataSet() const { return this->DataSet; }

  /// @brief Get the name of the output field that will be created to hold color data.
  VISKORES_CONT const std::string& GetPointFieldName() const { return this->PointFieldName; }
  /// @brief Set the name of the output field that will be created to hold color data.
  VISKORES_CONT void SetPointFieldName(const std::string& name) { this->PointFieldName = name; }

  VISKORES_CONT const std::string& GetFileName() const { return this->FileName; }
  VISKORES_CONT void SetFileName(const std::string& filename) { this->FileName = filename; }

protected:
  VISKORES_CONT virtual void Read() = 0;

  /// Resets the `DataSet` to hold the given pixels.
  void InitializeImageDataSet(const viskores::Id& width,
                              const viskores::Id& height,
                              const ColorArrayType& pixels);

  std::string FileName;
  std::string PointFieldName = "color";
  viskores::cont::DataSet DataSet;
};
}
} // namespace viskores::io

#endif //viskores_io_ImageReaderBase_h
