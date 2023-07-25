// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkNonMergingPointLocator.h"

#include "vtkObjectFactory.h"
#include "vtkPoints.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkNonMergingPointLocator);

//------------------------------------------------------------------------------
int vtkNonMergingPointLocator::InsertUniquePoint(const double x[3], vtkIdType& ptId)
{
  ptId = this->Points->InsertNextPoint(x);
  return 1;
}

//------------------------------------------------------------------------------
void vtkNonMergingPointLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
