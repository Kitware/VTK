// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkShortArray.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkShortArray);
vtkStandardExtendedNewMacro(vtkShortArray);

//------------------------------------------------------------------------------
vtkShortArray::vtkShortArray() = default;

//------------------------------------------------------------------------------
vtkShortArray::~vtkShortArray() = default;

//------------------------------------------------------------------------------
void vtkShortArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
