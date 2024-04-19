// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCuller.h"

VTK_ABI_NAMESPACE_BEGIN
vtkCuller::vtkCuller() = default;

vtkCuller::~vtkCuller() = default;

//------------------------------------------------------------------------------
void vtkCuller::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
