// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkContextItem.h"
#include "vtkContextTransform.h"

VTK_ABI_NAMESPACE_BEGIN
vtkCxxSetObjectMacro(vtkContextItem, Transform, vtkContextTransform);

//------------------------------------------------------------------------------
vtkContextItem::~vtkContextItem()
{
  this->SetTransform(nullptr);
}

//------------------------------------------------------------------------------
void vtkContextItem::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Opacity: " << this->Opacity << endl;
}
VTK_ABI_NAMESPACE_END
