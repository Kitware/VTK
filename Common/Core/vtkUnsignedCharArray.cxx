// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkUnsignedCharArray.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkUnsignedCharArray);
vtkStandardExtendedNewMacro(vtkUnsignedCharArray);

//------------------------------------------------------------------------------
vtkUnsignedCharArray::vtkUnsignedCharArray() = default;

//------------------------------------------------------------------------------
vtkUnsignedCharArray::~vtkUnsignedCharArray() = default;

//------------------------------------------------------------------------------
void vtkUnsignedCharArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
