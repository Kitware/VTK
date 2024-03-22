// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridCrinkleQuery.h"

#include "vtkBoundingBox.h"
#include "vtkCellAttribute.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkStringToken.h"

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkCellGridCrinkleQuery);

void vtkCellGridCrinkleQuery::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Function)
  {
    os << indent << "Function:\n";
    vtkIndent i2 = indent.GetNextIndent();
    this->Function->PrintSelf(os, i2);
  }
  else
  {
    os << indent << "Function: (none)\n";
  }
  os << indent << "HalfSpace: " << std::hex << this->HalfSpace << std::dec << "\n";
}

void vtkCellGridCrinkleQuery::Initialize() {}

void vtkCellGridCrinkleQuery::Finalize() {}

VTK_ABI_NAMESPACE_END
