// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHierarchicalDataExtractLevel.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHierarchicalDataExtractLevel);

//------------------------------------------------------------------------------
vtkHierarchicalDataExtractLevel::vtkHierarchicalDataExtractLevel() = default;

//------------------------------------------------------------------------------
vtkHierarchicalDataExtractLevel::~vtkHierarchicalDataExtractLevel() = default;

//------------------------------------------------------------------------------
void vtkHierarchicalDataExtractLevel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
