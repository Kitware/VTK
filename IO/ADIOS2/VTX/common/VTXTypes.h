// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/*
 * VTXTypes.h : header-only type definitions needed by the VTK::IOADIOS2 module
 *
 *  Created on: May 14, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_VTX_COMMON_VTXTypes_h
#define VTK_IO_ADIOS2_VTX_COMMON_VTXTypes_h

#include <map>
#include <vector>

#include "VTXDataArray.h"

#include <adios2.h>

namespace vtx
{
namespace types
{
VTK_ABI_NAMESPACE_BEGIN

/** key: variable name, value: DataArray */
using DataSet = std::map<std::string, DataArray>;

enum class DataSetType
{
  CellData,
  PointData,
  Points,
  Coordinates,
  Cells,
  Verts,
  Lines,
  Strips,
  Polys
};

using Piece = std::map<DataSetType, DataSet>;

#define VTK_IO_ADIOS2_VTX_ARRAY_TYPE(MACRO)                                                        \
  MACRO(int32_t)                                                                                   \
  MACRO(uint32_t)                                                                                  \
  MACRO(int64_t)                                                                                   \
  MACRO(uint64_t)                                                                                  \
  MACRO(float)                                                                                     \
  MACRO(double)

VTK_ABI_NAMESPACE_END
} // end namespace types
} // end namespace vtx

#endif /* VTK_IO_ADIOS2_VTX_COMMON_VTXTypes_h */
