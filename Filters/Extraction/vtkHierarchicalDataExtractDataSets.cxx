// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHierarchicalDataExtractDataSets.h"

#include "vtkObjectFactory.h"
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHierarchicalDataExtractDataSets);

//------------------------------------------------------------------------------
vtkHierarchicalDataExtractDataSets::vtkHierarchicalDataExtractDataSets() = default;

//------------------------------------------------------------------------------
vtkHierarchicalDataExtractDataSets::~vtkHierarchicalDataExtractDataSets() = default;

//------------------------------------------------------------------------------
void vtkHierarchicalDataExtractDataSets::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
