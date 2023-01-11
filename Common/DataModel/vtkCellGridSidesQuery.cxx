/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellGridSidesQuery.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellGridSidesQuery.h"

#include "vtkBoundingBox.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGridSidesQuery);

void vtkCellGridSidesQuery::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Hashes: " << this->Hashes.size() << "\n";
  os << indent << "Sides: " << this->Sides.size() << "\n";
}

void vtkCellGridSidesQuery::Initialize()
{
  this->Hashes.clear();
}

void vtkCellGridSidesQuery::Finalize()
{
  this->Sides.clear();
  for (const auto& entry : this->Hashes)
  {
    if (entry.second.Sides.size() % 2 == 0)
    {
      continue; // Do not output matching pairs of sides.
    }
    for (const auto& ss : entry.second.Sides)
    {
      this->Sides[ss.CellType][ss.SideShape][ss.DOF].insert(ss.SideId);
    }
  }
}

VTK_ABI_NAMESPACE_END
