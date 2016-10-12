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
#include "vtkNew.h"
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
  // For the most common cell types, this is a fast call. If the cell type is
  // more exotic, then the cell must be grabbed and queried directly, which is
  // slow.

  int cellType = this->GetCellType();

  switch (cellType)
  {
    case VTK_EMPTY_CELL:
    case VTK_VERTEX:
    case VTK_POLY_VERTEX:
      return 0;
    case VTK_LINE:
    case VTK_POLY_LINE:
    case VTK_QUADRATIC_EDGE:
    case VTK_CUBIC_LINE:
      return 1;
    case VTK_TRIANGLE:
    case VTK_QUAD:
    case VTK_PIXEL:
    case VTK_POLYGON:
    case VTK_TRIANGLE_STRIP:
    case VTK_QUADRATIC_TRIANGLE:
    case VTK_QUADRATIC_QUAD:
    case VTK_QUADRATIC_POLYGON:
      return 2;
    case VTK_TETRA:
    case VTK_VOXEL:
    case VTK_HEXAHEDRON:
    case VTK_WEDGE:
    case VTK_PYRAMID:
    case VTK_PENTAGONAL_PRISM:
    case VTK_HEXAGONAL_PRISM:
    case VTK_QUADRATIC_TETRA:
    case VTK_QUADRATIC_HEXAHEDRON:
    case VTK_QUADRATIC_WEDGE:
    case VTK_QUADRATIC_PYRAMID:
      return 3;
    default:
      vtkNew<vtkGenericCell> cell;
      this->GetCell(cell.GetPointer());
      return cell->GetCellDimension();
  }
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
vtkCellIterator::~vtkCellIterator()
{
}
