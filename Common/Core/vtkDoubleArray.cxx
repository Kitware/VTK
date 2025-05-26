// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDoubleArray.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDoubleArray);
vtkStandardExtendedNewMacro(vtkDoubleArray);

//------------------------------------------------------------------------------
vtkDoubleArray::vtkDoubleArray() = default;

//------------------------------------------------------------------------------
vtkDoubleArray::~vtkDoubleArray() = default;

//------------------------------------------------------------------------------
void vtkDoubleArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
