// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkScivisRepresentation.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkScivisRepresentation::vtkScivisRepresentation() = default;
//------------------------------------------------------------------------------
vtkScivisRepresentation::~vtkScivisRepresentation() = default;
//------------------------------------------------------------------------------
void vtkScivisRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
