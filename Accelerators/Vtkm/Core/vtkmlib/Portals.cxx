// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#define vtkmlib_Portals_cxx
#include "Portals.h"

#include <vtkm/cont/internal/ArrayPortalFromIterators.h>

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN
// T extern template instantiations
template class VTKACCELERATORSVTKMCORE_EXPORT vtkPointsPortal<vtkm::Vec<vtkm::Float32, 3> const>;
template class VTKACCELERATORSVTKMCORE_EXPORT vtkPointsPortal<vtkm::Vec<vtkm::Float64, 3> const>;
template class VTKACCELERATORSVTKMCORE_EXPORT vtkPointsPortal<vtkm::Vec<vtkm::Float32, 3>>;
template class VTKACCELERATORSVTKMCORE_EXPORT vtkPointsPortal<vtkm::Vec<vtkm::Float64, 3>>;
VTK_ABI_NAMESPACE_END
}
