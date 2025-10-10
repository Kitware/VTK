// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDummy.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkDummy);

//------------------------------------------------------------------------------
void vtkDummy::PrintSelf(ostream& os, vtkIndent indent)
{
  os << "vtkDummy:\n";
  this->Superclass::PrintSelf(os, indent.GetNextIndent());
}
