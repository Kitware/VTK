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

#define vtkmlib_Portals_cxx
#include "Portals.h"
#include "vtkmTags.h"

#include <vtkm/cont/internal/ArrayPortalFromIterators.h>

namespace tovtkm
{

// T extern template instantiations
VTKM_TEMPLATE_IMPORT_ArrayPortal(char, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::Int8, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::UInt8, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::Int16, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::UInt16, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::Int32, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::UInt32, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::Int64, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::UInt64, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::Float32, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::Float64, vtkAOSDataArrayTemplate);

VTKM_TEMPLATE_IMPORT_ArrayPortal(char, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::Int8, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::UInt8, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::Int16, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::UInt16, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::Int32, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::UInt32, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::Int64, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::UInt64, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::Float32, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(vtkm::Float64, vtkSOADataArrayTemplate);

#if VTKM_SIZE_LONG_LONG == 8
VTKM_TEMPLATE_IMPORT_ArrayPortal(long, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(unsigned long, vtkAOSDataArrayTemplate);

VTKM_TEMPLATE_IMPORT_ArrayPortal(long, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_IMPORT_ArrayPortal(unsigned long, vtkSOADataArrayTemplate);
#endif

template class VTKACCELERATORSVTKM_EXPORT
    vtkPointsPortal<vtkm::Vec<vtkm::Float32, 3> const>;
template class VTKACCELERATORSVTKM_EXPORT
    vtkPointsPortal<vtkm::Vec<vtkm::Float64, 3> const>;
template class VTKACCELERATORSVTKM_EXPORT
    vtkPointsPortal<vtkm::Vec<vtkm::Float32, 3>>;
template class VTKACCELERATORSVTKM_EXPORT
    vtkPointsPortal<vtkm::Vec<vtkm::Float64, 3>>;
}
