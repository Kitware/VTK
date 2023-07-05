// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHyperTreeGridLocator.h"
#include "vtkHyperTreeGrid.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
void vtkHyperTreeGridLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->HTG)
  {
    os << indent << "HyperTreeGrid: ";
    this->HTG->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "HyperTreeGrid: none\n";
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridLocator::SetHTG(vtkHyperTreeGrid* htg)
{
  vtkDebugMacro(<< " setting HTG to " << htg);
  if (this->HTG != htg)
  {
    this->HTG = htg;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkHyperTreeGrid* vtkHyperTreeGridLocator::GetHTG()
{
  return this->HTG;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridLocator::Update()
{
  if (!this->HTG)
  {
    vtkErrorMacro("HyperTreeGrid is nullptr while updating.");
    return;
  }
}
VTK_ABI_NAMESPACE_END
