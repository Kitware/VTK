// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkIntArray.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkIntArray);
vtkStandardExtendedNewMacro(vtkIntArray);

//------------------------------------------------------------------------------
vtkIntArray::vtkIntArray() = default;

//------------------------------------------------------------------------------
vtkIntArray::~vtkIntArray() = default;

//------------------------------------------------------------------------------
void vtkIntArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
