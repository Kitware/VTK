// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLinearContourLineInterpolator.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLinearContourLineInterpolator);

//------------------------------------------------------------------------------
vtkLinearContourLineInterpolator::vtkLinearContourLineInterpolator() = default;

//------------------------------------------------------------------------------
vtkLinearContourLineInterpolator::~vtkLinearContourLineInterpolator() = default;

//------------------------------------------------------------------------------
int vtkLinearContourLineInterpolator::InterpolateLine(vtkRenderer* vtkNotUsed(ren),
  vtkContourRepresentation* vtkNotUsed(rep), int vtkNotUsed(idx1), int vtkNotUsed(idx2))
{
  return 1;
}

//------------------------------------------------------------------------------
void vtkLinearContourLineInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
