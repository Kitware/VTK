/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkHyperTreeGridLocator.h"
#include "vtkHyperTreeGrid.h"

//------------------------------------------------------------------------------
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
