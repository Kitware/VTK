// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWrapped.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkWrapped);

vtkWrapped::vtkWrapped() = default;

vtkWrapped::~vtkWrapped() = default;

void vtkWrapped::PrintSelf(std::ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

std::string vtkWrapped::GetString() const
{
  return "wrapped";
}
