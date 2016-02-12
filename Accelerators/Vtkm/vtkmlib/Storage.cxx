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

#define vtkmlib_Storage_cxx
#include "Storage.h"

namespace vtkm
{
namespace cont
{
namespace internal
{

// T extern template instantiations
VTKM_TEMPLATE_IMPORT_Storage(char, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::Int8, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::UInt8, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::Int16, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::UInt16, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::Int32, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::UInt32, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::Int64, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::UInt64, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::Float32, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::Float64, tovtkm::vtkAOSArrayContainerTag);

VTKM_TEMPLATE_IMPORT_Storage(char, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::Int8, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::UInt8, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::Int16, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::UInt16, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::Int32, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::UInt32, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::Int64, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::UInt64, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::Float32, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(vtkm::Float64, tovtkm::vtkSOAArrayContainerTag);

#if VTKM_SIZE_LONG_LONG == 8
VTKM_TEMPLATE_IMPORT_Storage(long, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(unsigned long, tovtkm::vtkAOSArrayContainerTag);

VTKM_TEMPLATE_IMPORT_Storage(long, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_IMPORT_Storage(unsigned long, tovtkm::vtkSOAArrayContainerTag);
#endif

template class VTKACCELERATORSVTKM_EXPORT
    Storage<vtkIdType, tovtkm::vtkCellArrayContainerTag>;
}
}
}
