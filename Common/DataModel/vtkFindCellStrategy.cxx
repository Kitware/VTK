/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFindCellStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFindCellStrategy.h"

#include "vtkLogger.h"
#include "vtkPointSet.h"

//----------------------------------------------------------------------------
vtkFindCellStrategy::vtkFindCellStrategy()
{
  this->PointSet = nullptr;
}

//----------------------------------------------------------------------------
vtkFindCellStrategy::~vtkFindCellStrategy()
{
  // if ( this->PointSet != nullptr )
  // {
  //   vtkPointSet *ps = this->PointSet;
  //   this->PointSet = nullptr;
  //   ps->Delete();
  // }
}

//----------------------------------------------------------------------------
int vtkFindCellStrategy::Initialize(vtkPointSet* ps)
{
  // Make sure everything is up to snuff
  if (ps == nullptr || ps->GetPoints() == nullptr || ps->GetPoints()->GetNumberOfPoints() < 1)
  {
    vtkLog(ERROR, "Initialize must be called with non-NULL instance of vtkPointSet");
    return 0;
  }
  else
  {
    this->PointSet = ps;
    this->PointSet->GetBounds(this->Bounds);
    return 1;
  }
}

//----------------------------------------------------------------------------
void vtkFindCellStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "vtkPointSet: " << this->PointSet << "\n";
}
