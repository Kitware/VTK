// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkGraphEdge.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkGraphEdge);
//------------------------------------------------------------------------------
vtkGraphEdge::vtkGraphEdge()
{
  this->Source = 0;
  this->Target = 0;
  this->Id = 0;
}

//------------------------------------------------------------------------------
vtkGraphEdge::~vtkGraphEdge() = default;

//------------------------------------------------------------------------------
void vtkGraphEdge::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Source: " << this->Source << endl;
  os << indent << "Target: " << this->Target << endl;
  os << indent << "Id: " << this->Id << endl;
}
VTK_ABI_NAMESPACE_END
