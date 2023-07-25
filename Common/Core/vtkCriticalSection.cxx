// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Hide VTK_DEPRECATED_IN_9_1_0() warnings for this class.
// Remove with VTK_DEPRECATED_IN_9_2_0 because it was not actually deprecated
// in 9.1.0.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkCriticalSection.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCriticalSection);

void vtkCriticalSection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
