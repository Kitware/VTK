// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSerDesMockObject.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkSerDesMockObject);

vtkSerDesMockObject::vtkSerDesMockObject() = default;

vtkSerDesMockObject::~vtkSerDesMockObject() = default;

void vtkSerDesMockObject::PrintSelf(ostream& os, vtkIndent indent)
{
  os << "Tag: " << this->Tag << '\n';
  this->Superclass::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
