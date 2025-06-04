// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkFloatArray.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkFloatArray);
vtkStandardExtendedNewMacro(vtkFloatArray);

//------------------------------------------------------------------------------
vtkFloatArray::vtkFloatArray() = default;

//------------------------------------------------------------------------------
vtkFloatArray::~vtkFloatArray() = default;

//------------------------------------------------------------------------------
void vtkFloatArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
