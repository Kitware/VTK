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
#ifndef viskores_io_BOVDataSetReader_h
#define viskores_io_BOVDataSetReader_h

#include <viskores/cont/DataSet.h>

#include <viskores/io/viskores_io_export.h>

namespace viskores
{
namespace io
{

class VISKORES_IO_EXPORT BOVDataSetReader
{
public:
  VISKORES_CONT BOVDataSetReader(const char* fileName);
  VISKORES_CONT BOVDataSetReader(const std::string& fileName);

  VISKORES_CONT const viskores::cont::DataSet& ReadDataSet();

private:
  VISKORES_CONT void LoadFile();

  std::string FileName;
  bool Loaded;
  viskores::cont::DataSet DataSet;
};
}
} // viskores::io

#endif // viskores_io_BOVReader_h
