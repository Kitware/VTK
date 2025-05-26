// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkIdTypeArray.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkIdTypeArray);
vtkStandardExtendedNewMacro(vtkIdTypeArray);

//------------------------------------------------------------------------------
vtkIdTypeArray::vtkIdTypeArray() = default;

//------------------------------------------------------------------------------
vtkIdTypeArray::~vtkIdTypeArray() = default;

//------------------------------------------------------------------------------
void vtkIdTypeArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
