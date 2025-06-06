// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLongLongArray.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLongLongArray);
vtkStandardExtendedNewMacro(vtkLongLongArray);

//------------------------------------------------------------------------------
vtkLongLongArray::vtkLongLongArray() = default;

//------------------------------------------------------------------------------
vtkLongLongArray::~vtkLongLongArray() = default;

//------------------------------------------------------------------------------
void vtkLongLongArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
