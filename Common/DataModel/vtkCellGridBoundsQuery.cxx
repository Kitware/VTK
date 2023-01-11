/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellGridBoundsQuery.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellGridBoundsQuery.h"

#include "vtkBoundingBox.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGridBoundsQuery);

void vtkCellGridBoundsQuery::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Bounds: " << this->Bounds[0] << " " << this->Bounds[1] << ", " << this->Bounds[2]
     << " " << this->Bounds[3] << ", " << this->Bounds[4] << " " << this->Bounds[5] << "\n";
}

void vtkCellGridBoundsQuery::Initialize()
{
  vtkMath::UninitializeBounds(this->Bounds.data());
}

void vtkCellGridBoundsQuery::GetBounds(double* bds)
{
  if (!bds)
  {
    return;
  }

  std::copy(this->Bounds.begin(), this->Bounds.end(), bds);
}

void vtkCellGridBoundsQuery::AddBounds(vtkBoundingBox& bbox)
{
  // Ignore invalid bounds:
  if (!bbox.IsValid())
  {
    return;
  }
  // If the current local bounds are well-defined, then add them to the bbox:
  if (this->Bounds[0] <= this->Bounds[1])
  {
    bbox.AddPoint(this->Bounds[0], this->Bounds[2], this->Bounds[4]);
    bbox.AddPoint(this->Bounds[1], this->Bounds[3], this->Bounds[5]);
  }
  // Now copy bbox into our local storage:
  bbox.GetBounds(this->Bounds.data());
}

VTK_ABI_NAMESPACE_END
