// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellData.h"

#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCellData);
vtkStandardExtendedNewMacro(vtkCellData);

//------------------------------------------------------------------------------
vtkCellData::vtkCellData()
{
  this->GhostsToSkip = vtkDataSetAttributes::HIDDENCELL | vtkDataSetAttributes::REFINEDCELL;
}

//------------------------------------------------------------------------------
void vtkCellData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
