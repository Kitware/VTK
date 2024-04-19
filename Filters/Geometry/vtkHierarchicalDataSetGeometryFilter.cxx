// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
