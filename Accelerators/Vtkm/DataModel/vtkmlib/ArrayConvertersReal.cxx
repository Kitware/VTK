// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#define vtkmlib_ArrayConverterExport_cxx
#include "ArrayConverters.hxx"

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN

VTK_EXPORT_REAL_ARRAY_CONVERSION_TO_VTKM(vtkAOSDataArrayTemplate)
VTK_EXPORT_REAL_ARRAY_CONVERSION_TO_VTKM(vtkSOADataArrayTemplate)

VTK_ABI_NAMESPACE_END
} // tovtkm
