// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRendererDelegate.h"

VTK_ABI_NAMESPACE_BEGIN
vtkRendererDelegate::vtkRendererDelegate()
{
  this->Used = false;
}

vtkRendererDelegate::~vtkRendererDelegate() = default;

void vtkRendererDelegate::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Used: ";
  if (this->Used)
  {
    os << "On";
  }
  else
  {
    os << "Off";
  }
  os << endl;
}
VTK_ABI_NAMESPACE_END
