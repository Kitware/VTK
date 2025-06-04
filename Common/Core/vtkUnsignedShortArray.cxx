// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkUnsignedShortArray.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkUnsignedShortArray);
vtkStandardExtendedNewMacro(vtkUnsignedShortArray);

//------------------------------------------------------------------------------
vtkUnsignedShortArray::vtkUnsignedShortArray() = default;

//------------------------------------------------------------------------------
vtkUnsignedShortArray::~vtkUnsignedShortArray() = default;

//------------------------------------------------------------------------------
void vtkUnsignedShortArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
