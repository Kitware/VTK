// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkUnsignedLongArray.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkUnsignedLongArray);
vtkStandardExtendedNewMacro(vtkUnsignedLongArray);

//------------------------------------------------------------------------------
vtkUnsignedLongArray::vtkUnsignedLongArray() = default;

//------------------------------------------------------------------------------
vtkUnsignedLongArray::~vtkUnsignedLongArray() = default;

//------------------------------------------------------------------------------
void vtkUnsignedLongArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
