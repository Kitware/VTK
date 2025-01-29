// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// VTK_DEPRECATED_IN_9_5_0()
#define VTK_DEPRECATION_LEVEL 0

#include "vtkHierarchicalDataSetGeometryFilter.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHierarchicalDataSetGeometryFilter);
//------------------------------------------------------------------------------
vtkHierarchicalDataSetGeometryFilter::vtkHierarchicalDataSetGeometryFilter() = default;

//------------------------------------------------------------------------------
vtkHierarchicalDataSetGeometryFilter::~vtkHierarchicalDataSetGeometryFilter() = default;

//------------------------------------------------------------------------------
void vtkHierarchicalDataSetGeometryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
