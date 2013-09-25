/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCellIterator.h"

#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkPoints.h"

//------------------------------------------------------------------------------
void vtkCellIterator::PrintSelf(ostream &os, vtkIndent indent)
{
  os << indent << "CacheFlags: ";
  switch (this->CacheFlags)
    {
    case UninitializedFlag:
      os << "UninitializedFlag" << endl;
      break;
    default:
      {
      bool addSplit = false;

      if (this->CheckCache(CellTypeFlag))
        {
        os << "CellTypeFlag";
        addSplit = true;
        }

      if (this->CheckCache(PointIdsFlag))
        {
        os << (addSplit ? " | " : "") << "PointIdsFlag";
        addSplit = true;
        }

      if (this->CheckCache(PointsFlag))
        {
        os << (addSplit ? " | " : "") << "PointsFlag";
        addSplit = true;
        }

      if (this->CheckCache(FacesFlag))
        {
        os << (addSplit ? " | " : "") << "FacesFlag";
        addSplit = true;
        }
      os << endl;
      }
    }

  os << indent << "CellType: " << this->CellType << endl;
  os << indent << "Points:" << endl;
  this->Points->PrintSelf(os, indent.GetNextIndent());
  os << indent << "PointIds:" << endl;
  this->PointIds->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Faces:" << endl;
  this->Faces->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
void vtkCellIterator::GetCell(vtkGenericCell *cell)
{
  cell->SetCellType(this->GetCellType());
  cell->SetPointIds(this->GetPointIds());
  cell->SetPoints(this->GetPoints());

  if (cell->RequiresExplicitFaceRepresentation())
    {
    vtkIdList *faces = this->GetFaces();
    if (faces->GetNumberOfIds() != 0)
      {
      cell->SetFaces(faces->GetPointer(0));
      }
    }

  if (cell->RequiresInitialization())
    {
    cell->Initialize();
    }
}

//------------------------------------------------------------------------------
vtkCellIterator::vtkCellIterator()
  : CellType(VTK_EMPTY_CELL),
    CacheFlags(UninitializedFlag)
{
  this->Points = this->PointsContainer.GetPointer();
  this->PointIds = this->PointIdsContainer.GetPointer();
  this->Faces = this->FacesContainer.GetPointer();
}

//------------------------------------------------------------------------------
void vtkCellIterator::ResetContainers()
{
  this->Points->Reset();
  this->PointIds->Reset();
  this->Faces->Reset();
  this->CellType = VTK_EMPTY_CELL;
}

//------------------------------------------------------------------------------
vtkCellIterator::~vtkCellIterator()
{
}
