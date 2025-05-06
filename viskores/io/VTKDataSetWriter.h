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
#ifndef viskores_io_DataSetWriter_h
#define viskores_io_DataSetWriter_h

#include <viskores/cont/DataSet.h>

#include <viskores/io/viskores_io_export.h>

namespace viskores
{
namespace io
{

// Might want to place this somewhere else.
enum struct FileType
{
  ASCII,
  BINARY
};

/// @brief Reads a legacy VTK file.
///
/// By convention, legacy VTK files have a `.vtk` extension.
/// This class should be constructed with a filename, and the data
/// read with `ReadDataSet`.
class VISKORES_IO_EXPORT VTKDataSetWriter
{
public:
  VISKORES_CONT VTKDataSetWriter(const char* fileName);
  /// @brief Construct a writer to save data to the given file.
  VISKORES_CONT VTKDataSetWriter(const std::string& fileName);

  /// @brief Write data from the given `DataSet` object to the file specified in the constructor.
  VISKORES_CONT void WriteDataSet(const viskores::cont::DataSet& dataSet) const;

  /// @brief Get whether the file will be written in ASCII or binary format.
  ///
  VISKORES_CONT viskores::io::FileType GetFileType() const;

  /// @brief Set whether the file will be written in ASCII or binary format.
  VISKORES_CONT void SetFileType(viskores::io::FileType type);
  /// @brief Set whether the file will be written in ASCII or binary format.
  VISKORES_CONT void SetFileTypeToAscii() { this->SetFileType(viskores::io::FileType::ASCII); }
  /// @brief Set whether the file will be written in ASCII or binary format.
  VISKORES_CONT void SetFileTypeToBinary() { this->SetFileType(viskores::io::FileType::BINARY); }

private:
  std::string FileName;
  viskores::io::FileType FileType = viskores::io::FileType::ASCII;

}; //struct VTKDataSetWriter
}
} //namespace viskores::io

#endif //viskores_io_DataSetWriter_h
