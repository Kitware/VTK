// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellIterator.h"

#include "vtkCellTypes.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkPoints.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
void vtkCellIterator::PrintSelf(ostream& os, vtkIndent indent)
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
int vtkCellIterator::GetCellDimension()
{
  return vtkCellTypes::GetDimension(this->GetCellType());
}

//------------------------------------------------------------------------------
void vtkCellIterator::GetCell(vtkGenericCell* cell)
{
  cell->SetCellType(this->GetCellType());
  cell->SetPointIds(this->GetPointIds());
  cell->SetPoints(this->GetPoints());

  if (cell->RequiresExplicitFaceRepresentation())
  {
    vtkCellArray* faces = this->GetCellFaces();

    if (faces->GetNumberOfCells() != 0)
    {
      cell->SetCellFaces(faces);
    }
  }

  if (cell->RequiresInitialization())
  {
    cell->Initialize();
  }
}

//------------------------------------------------------------------------------
// To be removed when deprecating
vtkIdList* vtkCellIterator::GetFaces()
{
  return this->GetSerializedCellFaces();
}

//------------------------------------------------------------------------------
vtkCellIterator::vtkCellIterator()
  : CellType(VTK_EMPTY_CELL)
  , CacheFlags(UninitializedFlag)
{
  this->Points = this->PointsContainer;
  this->PointIds = this->PointIdsContainer;
  this->Faces = this->FacesContainer;
}

//------------------------------------------------------------------------------
vtkCellIterator::~vtkCellIterator() = default;
VTK_ABI_NAMESPACE_END
