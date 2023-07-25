// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkFunctionSet.h"

VTK_ABI_NAMESPACE_BEGIN
vtkFunctionSet::vtkFunctionSet()
{
  this->NumFuncs = 0;
  this->NumIndepVars = 0;
}

void vtkFunctionSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Number of functions: " << this->NumFuncs << "\n";
  os << indent << "Number of independent variables: " << this->NumIndepVars << "\n";
}
VTK_ABI_NAMESPACE_END
