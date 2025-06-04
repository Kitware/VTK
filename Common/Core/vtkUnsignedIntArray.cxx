// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkUnsignedIntArray.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkUnsignedIntArray);
vtkStandardExtendedNewMacro(vtkUnsignedIntArray);

//------------------------------------------------------------------------------
vtkUnsignedIntArray::vtkUnsignedIntArray() = default;

//------------------------------------------------------------------------------
vtkUnsignedIntArray::~vtkUnsignedIntArray() = default;

//------------------------------------------------------------------------------
void vtkUnsignedIntArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
