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

#ifndef viskores_io_VTKVisItFileReader_h
#define viskores_io_VTKVisItFileReader_h

#include <string>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/io/viskores_io_export.h>

namespace viskores
{
namespace io
{

/// Reader for ".visit" files, a simple file format for partioned data sets.
/// The file format consists of the keyword "!NBLOCKS <N>", where N is the number of
/// partitions, followed by a list of the N files. For example:
///
/// ```
/// # This is a comment
/// !NBLOCKS 2
/// file1.vtk
/// file2.vtk
/// ```
///
/// Note: .visit files support time varying partitioned data, but it is not supported
/// in this reader.
///

class VISKORES_IO_EXPORT VTKVisItFileReader
{
public:
  VISKORES_CONT VTKVisItFileReader(const char* fileName);
  VISKORES_CONT VTKVisItFileReader(const std::string& fileName);

  VISKORES_CONT viskores::cont::PartitionedDataSet ReadPartitionedDataSet();

private:
  std::string FileName;
};

}
} //viskores::io

#endif //viskores_io_VTKVisItFileReader_h
