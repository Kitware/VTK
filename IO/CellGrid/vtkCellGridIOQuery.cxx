// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridIOQuery.h"

#include "vtkObjectFactory.h"

#include <vtk_nlohmannjson.h>        // For API.
#include VTK_NLOHMANN_JSON(json.hpp) // For API.

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGridIOQuery);

void vtkCellGridIOQuery::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Data: ";
  if (this->Data)
  {
    os << this->Data->dump(2) << "\n";
  }
  else
  {
    os << "null\n";
  }
}

VTK_ABI_NAMESPACE_END
