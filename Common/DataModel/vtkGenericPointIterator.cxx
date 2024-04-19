// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGenericPointIterator.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkGenericPointIterator::vtkGenericPointIterator() = default;

//------------------------------------------------------------------------------
vtkGenericPointIterator::~vtkGenericPointIterator() = default;

//------------------------------------------------------------------------------
void vtkGenericPointIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
