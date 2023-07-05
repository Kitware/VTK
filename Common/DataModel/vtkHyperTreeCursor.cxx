// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeCursor.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkHyperTreeCursor::vtkHyperTreeCursor() = default;

//------------------------------------------------------------------------------
vtkHyperTreeCursor::~vtkHyperTreeCursor() = default;

//------------------------------------------------------------------------------
void vtkHyperTreeCursor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
