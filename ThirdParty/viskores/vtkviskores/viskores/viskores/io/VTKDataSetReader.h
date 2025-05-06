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
#ifndef viskores_io_VTKDataSetReader_h
#define viskores_io_VTKDataSetReader_h

#include <viskores/io/VTKDataSetReaderBase.h>

namespace viskores
{
namespace io
{

/// @brief Reads a legacy VTK file.
///
/// By convention, legacy VTK files have a `.vtk` extension.
/// This class should be constructed with a filename, and the data
/// read with `ReadDataSet`.
class VISKORES_IO_EXPORT VTKDataSetReader : public VTKDataSetReaderBase
{
public:
  VISKORES_CONT VTKDataSetReader(const char* fileName);
  /// @brief Construct a reader to load data from the given file.
  VISKORES_CONT VTKDataSetReader(const std::string& fileName);
  VISKORES_CONT ~VTKDataSetReader() override;

  VTKDataSetReader(const VTKDataSetReader&) = delete;
  void operator=(const VTKDataSetReader&) = delete;

  VISKORES_CONT void PrintSummary(std::ostream& out) const override;

private:
  VISKORES_CONT void CloseFile() override;
  VISKORES_CONT void Read() override;

  std::unique_ptr<VTKDataSetReaderBase> Reader;
};

}
} // viskores::io

#endif // viskores_io_VTKReader_h
