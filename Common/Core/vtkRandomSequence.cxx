// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRandomSequence.h"

#include <cassert>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkRandomSequence::vtkRandomSequence() = default;

//------------------------------------------------------------------------------
vtkRandomSequence::~vtkRandomSequence() = default;

//------------------------------------------------------------------------------
void vtkRandomSequence::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
double vtkRandomSequence::GetNextValue()
{
  this->Next();
  return this->GetValue();
}
VTK_ABI_NAMESPACE_END
