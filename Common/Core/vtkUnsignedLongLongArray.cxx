// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkUnsignedLongLongArray.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkUnsignedLongLongArray);
vtkStandardExtendedNewMacro(vtkUnsignedLongLongArray);

//------------------------------------------------------------------------------
vtkUnsignedLongLongArray::vtkUnsignedLongLongArray() = default;

//------------------------------------------------------------------------------
vtkUnsignedLongLongArray::~vtkUnsignedLongLongArray() = default;

//------------------------------------------------------------------------------
void vtkUnsignedLongLongArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
