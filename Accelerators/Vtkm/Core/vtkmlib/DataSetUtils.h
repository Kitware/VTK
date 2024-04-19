// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#ifndef vtkmlib_DataSetUtils_h
#define vtkmlib_DataSetUtils_h

#include "vtkAcceleratorsVTKmCoreModule.h" //required for correct implementation
#include "vtkmConfigCore.h"                //required for general vtkm setup

#include <vtkm/cont/DataSet.h>

VTK_ABI_NAMESPACE_BEGIN

/**
 * Get Fields indices of VTKm DataSet excluding the Coordinate Systems fields indices.
 */
VTKACCELERATORSVTKMCORE_EXPORT
std::vector<vtkm::Id> GetFieldsIndicesWithoutCoords(const vtkm::cont::DataSet& input);

VTK_ABI_NAMESPACE_END

#endif // vtkmlib_DataSetUtils_h
/* VTK-HeaderTest-Exclude: DataSetUtils.h */
