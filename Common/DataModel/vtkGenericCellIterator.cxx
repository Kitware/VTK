// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGenericCellIterator.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkGenericCellIterator::vtkGenericCellIterator() = default;

//------------------------------------------------------------------------------
vtkGenericCellIterator::~vtkGenericCellIterator() = default;

//------------------------------------------------------------------------------
void vtkGenericCellIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
