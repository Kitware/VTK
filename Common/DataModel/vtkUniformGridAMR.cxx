// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#define VTK_DEPRECATION_LEVEL 0
#include "vtkUniformGridAMR.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkUniformGridAMR);

//------------------------------------------------------------------------------
vtkUniformGridAMR::vtkUniformGridAMR() = default;

//------------------------------------------------------------------------------
vtkUniformGridAMR::~vtkUniformGridAMR() = default;

//------------------------------------------------------------------------------
void vtkUniformGridAMR::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
