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
#ifndef viskores_io_VTKStructuredPointsReader_h
#define viskores_io_VTKStructuredPointsReader_h

#include <viskores/io/VTKDataSetReaderBase.h>

namespace viskores
{
namespace io
{

class VISKORES_IO_EXPORT VTKStructuredPointsReader : public VTKDataSetReaderBase
{
public:
  explicit VISKORES_CONT VTKStructuredPointsReader(const char* fileName);
  explicit VISKORES_CONT VTKStructuredPointsReader(const std::string& fileName);

private:
  VISKORES_CONT void Read() override;
};
}
} // namespace viskores::io

#endif // viskores_io_VTKStructuredPointsReader_h
