// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// VTK_DEPRECATED_IN_9_2_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkInterpolatedVelocityField.h"

#include "vtkClosestPointStrategy.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkInterpolatedVelocityField);

//------------------------------------------------------------------------------
vtkInterpolatedVelocityField::vtkInterpolatedVelocityField()
{
  // Create the default FindCellStrategy. Note that it is deleted by the
  // superclass.
  this->FindCellStrategy = vtkClosestPointStrategy::New();
}

//------------------------------------------------------------------------------
void vtkInterpolatedVelocityField::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
