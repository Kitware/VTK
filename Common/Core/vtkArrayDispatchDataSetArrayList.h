// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkArrayDispatchDataSetArrayList_h
#define vtkArrayDispatchDataSetArrayList_h

#include "vtkAOSDataArrayTemplate.h" // For vtkAOSDataArrayTemplate
#include "vtkAffineTypeInt32Array.h" // For vtkAffineTypeInt32Array
#include "vtkAffineTypeInt64Array.h" // For vtkAffineTypeInt64Array
#include "vtkSOADataArrayTemplate.h" // For vtkSOADataArrayTemplate
#include "vtkStructuredPointArray.h" // For vtkStructuredPointArray
#include "vtkTypeInt32Array.h"       // For vtkTypeInt32Array
#include "vtkTypeInt64Array.h"       // For vtkTypeInt64Array
#include "vtkTypeList.h"             // For vtkTypeList

namespace vtkArrayDispatch
{
VTK_ABI_NAMESPACE_BEGIN

/**
 * The type list of AOS point arrays. Should be used when creating array for output points based
 * on a data type.
 */
using AOSPointArrays =
  vtkTypeList::Create<vtkAOSDataArrayTemplate<float>, vtkAOSDataArrayTemplate<double>>;

/**
 * The type list of AOS & SOA point arrays. Should be used when processing explicit point arrays.
 * It should be sufficient for most input points.
 */
using PointArrays = vtkTypeList::Append<AOSPointArrays,
  vtkTypeList::Create<vtkSOADataArrayTemplate<float>, vtkSOADataArrayTemplate<double>>>::Result;

/**
 * The type list of AOS, SOA, structured point arrays. Should be used when processing
 * `vtkDataSet::GetPoints()`.
 */
using AllPointArrays =
  vtkTypeList::Append<PointArrays, vtkTypeList::Create<vtkStructuredPointArray<double>>>::Result;

///@{
/**
 * List of possible array types used for storage of vtkCellArray. May be used with
 * vtkArrayDispatch::Dispatch[2]ByArray to process internal arrays.
 * Both the Connectivity and Offset arrays are guaranteed to have the same value type.
 *
 * @sa vtkCellArray::Dispatch () for a simpler mechanism.
 */
using StorageConnectivityArrays = vtkTypeList::Create<vtkTypeInt32Array, vtkTypeInt64Array>;
using StorageOffsetsArrays = vtkTypeList::Create<vtkTypeInt32Array, vtkTypeInt64Array,
  vtkAffineTypeInt32Array, vtkAffineTypeInt64Array>;
///@}

///@{
/**
 * List of possible ArrayTypes that are compatible with internal storage of vtkCellArray.
 *
 * This can be used with vtkArrayDispatch::DispatchByArray, etc to
 * check input arrays before assigning them to a cell array.
 */
using InputOffsetsArrays = vtkTypeList::Unique<vtkTypeList::Create<vtkAOSDataArrayTemplate<int>,
  vtkAOSDataArrayTemplate<long>, vtkAOSDataArrayTemplate<long long>, vtkAffineArray<int>,
  vtkAffineArray<long>, vtkAffineArray<long long>>>::Result;
using InputConnectivityArrays =
  vtkTypeList::Unique<vtkTypeList::Create<vtkAOSDataArrayTemplate<int>,
    vtkAOSDataArrayTemplate<long>, vtkAOSDataArrayTemplate<long long>>>::Result;
///@}

VTK_ABI_NAMESPACE_END
}

#endif // vtkArrayDispatchDataSetArrayList_h
