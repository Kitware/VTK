// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellIterator.h"

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
    case VTK_LAGRANGE_CURVE:
    case VTK_BEZIER_CURVE:
      return 1;
    case VTK_TRIANGLE:
    case VTK_QUAD:
    case VTK_PIXEL:
    case VTK_POLYGON:
    case VTK_TRIANGLE_STRIP:
    case VTK_QUADRATIC_TRIANGLE:
    case VTK_QUADRATIC_QUAD:
    case VTK_QUADRATIC_POLYGON:
    case VTK_BIQUADRATIC_QUAD:
    case VTK_BIQUADRATIC_TRIANGLE:
    case VTK_LAGRANGE_TRIANGLE:
    case VTK_LAGRANGE_QUADRILATERAL:
    case VTK_BEZIER_TRIANGLE:
    case VTK_BEZIER_QUADRILATERAL:
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
    case VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
    case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
    case VTK_TRIQUADRATIC_HEXAHEDRON:
    case VTK_TRIQUADRATIC_PYRAMID:
    case VTK_LAGRANGE_TETRAHEDRON:
    case VTK_LAGRANGE_HEXAHEDRON:
    case VTK_LAGRANGE_WEDGE:
    case VTK_BEZIER_TETRAHEDRON:
    case VTK_BEZIER_HEXAHEDRON:
    case VTK_BEZIER_WEDGE:
      return 3;
    default:
      vtkNew<vtkGenericCell> cell;
      this->GetCell(cell);
      return cell->GetCellDimension();
  }
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
