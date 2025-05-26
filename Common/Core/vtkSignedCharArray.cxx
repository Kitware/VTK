// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSignedCharArray.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSignedCharArray);
vtkStandardExtendedNewMacro(vtkSignedCharArray);

//------------------------------------------------------------------------------
vtkSignedCharArray::vtkSignedCharArray() = default;

//------------------------------------------------------------------------------
vtkSignedCharArray::~vtkSignedCharArray() = default;

//------------------------------------------------------------------------------
void vtkSignedCharArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
