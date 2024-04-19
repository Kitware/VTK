// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellArrayIterator.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCellArrayIterator);

void vtkCellArrayIterator::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CurrentCellId: " << this->CurrentCellId << "\n";
  os << indent << "CellArray: " << this->CellArray.Get() << "\n";
}
VTK_ABI_NAMESPACE_END
