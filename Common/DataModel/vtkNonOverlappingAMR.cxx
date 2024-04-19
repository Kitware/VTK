// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkNonOverlappingAMR.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkNonOverlappingAMR);

//------------------------------------------------------------------------------
vtkNonOverlappingAMR::vtkNonOverlappingAMR() = default;

//------------------------------------------------------------------------------
vtkNonOverlappingAMR::~vtkNonOverlappingAMR() = default;

//------------------------------------------------------------------------------
void vtkNonOverlappingAMR::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
