// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGPUInfoListArray
 * @brief   Internal class vtkGPUInfoList.
 *
 * vtkGPUInfoListArray is just a PIMPL mechanism for vtkGPUInfoList.
 */

#ifndef vtkGPUInfoListArray_h
#define vtkGPUInfoListArray_h

#include "vtkGPUInfo.h"
#include <vector> // STL Header

VTK_ABI_NAMESPACE_BEGIN
class vtkGPUInfoListArray
{
public:
  std::vector<vtkGPUInfo*> v;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkGPUInfoListArray.h
