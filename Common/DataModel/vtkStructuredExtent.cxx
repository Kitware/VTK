// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStructuredExtent.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkStructuredExtent);
//------------------------------------------------------------------------------
vtkStructuredExtent::vtkStructuredExtent() = default;

//------------------------------------------------------------------------------
vtkStructuredExtent::~vtkStructuredExtent() = default;

//------------------------------------------------------------------------------
void vtkStructuredExtent::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
