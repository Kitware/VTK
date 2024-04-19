// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridQuery.h"

#include <iostream>

VTK_ABI_NAMESPACE_BEGIN

void vtkCellGridQuery::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Pass: " << this->Pass << "\n";
}

void vtkCellGridQuery::Initialize()
{
  this->Pass = -1;
}

void vtkCellGridQuery::StartPass()
{
  ++this->Pass;
}

VTK_ABI_NAMESPACE_END
