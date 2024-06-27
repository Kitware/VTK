// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/*
 * VTXDataArray.h : wrapper around vtkDataArray adding adios2 relevant
 * information
 *
 *  Created on: Jun 4, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_VTX_COMMON_VTXDataArray_h
#define VTK_IO_ADIOS2_VTX_COMMON_VTXDataArray_h

#include "vtkDataArray.h"
#include "vtkSmartPointer.h"

#include <map>

#include <adios2.h>

namespace vtx
{
namespace types
{
VTK_ABI_NAMESPACE_BEGIN

class DataArray
{
public:
  std::vector<std::string> VectorVariables;
  vtkSmartPointer<vtkDataArray> Data;

  // required for global arrays
  adios2::Dims Shape;
  adios2::Dims Start;
  adios2::Dims Count;

  // required for local arrays, using maps for now
  /** key: blockID, value: block count */
  std::map<size_t, adios2::Dims> BlockCounts;

  /** true : uses the special vtkIdType for indexing
   *  false : uses other VTK supported type */
  bool IsIdType = false;

  /** true: tuples > 1, false: tuples = 1 */
  bool HasTuples = false;

  /** true: new value is found and read, false: not updated */
  bool IsUpdated = true;

  /**
   *  true: is struct of arrays (*x, *y, *z) or (XXXX, YYYY, ZZZZ)
   *  false (default): is array of structs (xyz, xyz, xyz) common case
   */
  bool IsSOA = false;

  DataArray() = default;
  ~DataArray() = default;

  bool IsScalar() const noexcept;

  /**
   * Convert internal vtkDataArray to a 3D VTK conforming array
   * number of components = 3 filling the values of absent
   * coordinates with the input fillValues (default = 0)
   * @param fillValues values to fill for missing coordinates
   */
  void ConvertTo3DVTK(const std::vector<double>& fillValues = std::vector<double>());
};

VTK_ABI_NAMESPACE_END
} // end namespace types
} // end namespace vtx

#endif /* VTK_IO_ADIOS2_VTX_COMMON_VTXDataArray_h */
