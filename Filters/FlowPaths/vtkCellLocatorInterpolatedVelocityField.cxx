// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// VTK_DEPRECATED_IN_9_2_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkCellLocatorInterpolatedVelocityField.h"

#include "vtkCellLocatorStrategy.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCellLocatorInterpolatedVelocityField);

//------------------------------------------------------------------------------
vtkCellLocatorInterpolatedVelocityField::vtkCellLocatorInterpolatedVelocityField()
{
  // Create the default FindCellStrategy. Note that it is deleted by the
  // superclass.
  this->FindCellStrategy = vtkCellLocatorStrategy::New();
}

//------------------------------------------------------------------------------
vtkCellLocatorInterpolatedVelocityField::~vtkCellLocatorInterpolatedVelocityField() = default;

//------------------------------------------------------------------------------
void vtkCellLocatorInterpolatedVelocityField::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
