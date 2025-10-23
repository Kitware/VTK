// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkArrayDispatchDataSetArrayList_h
#define vtkArrayDispatchDataSetArrayList_h

#include "vtkAOSDataArrayTemplate.h" // For vtkAOSDataArrayTemplate
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

VTK_ABI_NAMESPACE_END
}

#endif // vtkArrayDispatchDataSetArrayList_h
