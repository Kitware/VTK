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
vtkHyperTreeGridLocator::vtkHyperTreeGridLocator()
  : HTG(nullptr)
{
}

//------------------------------------------------------------------------------
vtkHyperTreeGridLocator::~vtkHyperTreeGridLocator()
{
  this->SetHTG(nullptr);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridLocator::SetHTG(vtkHyperTreeGrid* cand)
{
  this->HTG = cand;
}

//------------------------------------------------------------------------------
vtkHyperTreeGrid* vtkHyperTreeGridLocator::GetHTG()
{
  return this->HTG;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridLocator::Initialize() {}

//------------------------------------------------------------------------------
void vtkHyperTreeGridLocator::Update()
{
  if (!this->HTG)
  {
    vtkErrorMacro("HyperTreeGrid not set before updating.");
    return;
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridLocator::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << this->GetObjectName() << " acting on:\n";
  if (this->HTG)
  {
    HTG->PrintSelf(os, indent);
  }
  else
  {
    os << indent << "HyperTreeGrid: none\n";
  }
}
