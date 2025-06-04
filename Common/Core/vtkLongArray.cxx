// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLongArray.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLongArray);
vtkStandardExtendedNewMacro(vtkLongArray);

//------------------------------------------------------------------------------
vtkLongArray::vtkLongArray() = default;

//------------------------------------------------------------------------------
vtkLongArray::~vtkLongArray() = default;

//------------------------------------------------------------------------------
void vtkLongArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
