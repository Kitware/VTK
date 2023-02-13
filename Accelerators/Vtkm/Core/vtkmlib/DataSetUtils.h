//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

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
