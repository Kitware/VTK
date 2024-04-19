// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLevelIdScalars.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLevelIdScalars);

//------------------------------------------------------------------------------
vtkLevelIdScalars::vtkLevelIdScalars() = default;

//------------------------------------------------------------------------------
vtkLevelIdScalars::~vtkLevelIdScalars() = default;

//------------------------------------------------------------------------------
void vtkLevelIdScalars::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
