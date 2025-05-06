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
#ifndef viskores_io_internal_VTKDataSetStructures_h
#define viskores_io_internal_VTKDataSetStructures_h

#include <string>

namespace viskores
{
namespace io
{
namespace internal
{

enum DataSetStructure
{
  DATASET_UNKNOWN = 0,
  DATASET_STRUCTURED_POINTS,
  DATASET_STRUCTURED_GRID,
  DATASET_UNSTRUCTURED_GRID,
  DATASET_POLYDATA,
  DATASET_RECTILINEAR_GRID,
  DATASET_FIELD
};

inline const char* DataSetStructureString(int id)
{
  static const char* strings[] = { "",
                                   "STRUCTURED_POINTS",
                                   "STRUCTURED_GRID",
                                   "UNSTRUCTURED_GRID",
                                   "POLYDATA",
                                   "RECTILINEAR_GRID",
                                   "FIELD" };
  return strings[id];
}

inline DataSetStructure DataSetStructureId(const std::string& str)
{
  DataSetStructure structure = DATASET_UNKNOWN;
  for (int id = 1; id < 7; ++id)
  {
    if (str == DataSetStructureString(id))
    {
      structure = static_cast<DataSetStructure>(id);
    }
  }

  return structure;
}
}
}
} // namespace viskores::io::internal

#endif // viskores_io_internal_VTKDataSetStructures_h
