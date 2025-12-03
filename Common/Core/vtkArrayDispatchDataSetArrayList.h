// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkArrayDispatchDataSetArrayList_h
#define vtkArrayDispatchDataSetArrayList_h

#include "vtkAOSDataArrayTemplate.h" // For vtkAOSDataArrayTemplate
#include "vtkAffineArray.h"          // For vtkAffineArray
#include "vtkConstantArray.h"        // For vtkConstantArray
#include "vtkSOADataArrayTemplate.h" // For vtkSOADataArrayTemplate
#include "vtkStructuredPointArray.h" // For vtkStructuredPointArray
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
 * vtkArrayDispatch::Dispatch[2]ByArray to process arrays.
 * Both the Connectivity and Offset arrays are guaranteed to have the same value type.
 *
 * @sa vtkCellArray::Dispatch () for a simpler mechanism.
 */
using ConnectivityArrays =
  vtkTypeList::Create<vtkAOSDataArrayTemplate<vtkTypeInt32>, vtkAOSDataArrayTemplate<vtkTypeInt64>>;
using OffsetsArrays =
  vtkTypeList::Create<vtkAOSDataArrayTemplate<vtkTypeInt32>, vtkAOSDataArrayTemplate<vtkTypeInt64>,
    vtkAffineArray<vtkTypeInt32>, vtkAffineArray<vtkTypeInt64>>;
///@}

/**
 * List of possible array types to use for vtkUnstructuredGrid's cell types array.
 */
using CellTypesArrays =
  vtkTypeList::Create<vtkAOSDataArrayTemplate<unsigned char>, vtkConstantArray<unsigned char>>;

VTK_ABI_NAMESPACE_END
}

#endif // vtkArrayDispatchDataSetArrayList_h
