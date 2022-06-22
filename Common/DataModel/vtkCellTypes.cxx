/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellTypes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Hide VTK_DEPRECATED_IN_9_2_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkCellTypes.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkLegacy.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkCellTypes);

namespace
{
// This list should contain the cell class names in
// the same order as the enums in vtkCellType.h. Make sure
// this list is nullptr terminated.
const char* vtkCellTypesStrings[] = { "vtkEmptyCell", "vtkVertex", "vtkPolyVertex", "vtkLine",
  "vtkPolyLine", "vtkTriangle", "vtkTriangleStrip", "vtkPolygon", "vtkPixel", "vtkQuad", "vtkTetra",
  "vtkVoxel", "vtkHexahedron", "vtkWedge", "vtkPyramid", "vtkPentagonalPrism", "vtkHexagonalPrism",
  "UnknownClass", "UnknownClass", "UnknownClass", "UnknownClass", "vtkQuadraticEdge",
  "vtkQuadraticTriangle", "vtkQuadraticQuad", "vtkQuadraticTetra", "vtkQuadraticHexahedron",
  "vtkQuadraticWedge", "vtkQuadraticPyramid", "vtkBiQuadraticQuad", "vtkTriQuadraticHexahedron",
  "vtkQuadraticLinearQuad", "vtkQuadraticLinearWedge", "vtkBiQuadraticQuadraticWedge",
  "vtkBiQuadraticQuadraticHexahedron", "vtkBiQuadraticTriangle", "vtkCubicLine",
  "vtkQuadraticPolygon", "vtkTriQuadraticPyramid", "UnknownClass", "UnknownClass", "UnknownClass",
  "vtkConvexPointSet", "vtkPolyhedron", "UnknownClass", "UnknownClass", "UnknownClass",
  "UnknownClass", "UnknownClass", "UnknownClass", "UnknownClass", "UnknownClass",
  "vtkParametricCurve", "vtkParametricSurface", "vtkParametricTriSurface",
  "vtkParametricQuadSurface", "vtkParametricTetraRegion", "vtkParametricHexRegion", "UnknownClass",
  "UnknownClass", "UnknownClass", "vtkHigherOrderEdge", "vtkHigherOrderTriangle",
  "vtkHigherOrderQuad", "vtkHigherOrderPolygon", "vtkHigherOrderTetrahedron", "vtkHigherOrderWedge",
  "vtkHigherOrderPyramid", "vtkHigherOrderHexahedron", "vtkLagrangeCurve",
  "vtkLagrangeQuadrilateral", "vtkLagrangeTriangle", "vtkLagrangeTetra", "vtkLagrangeHexahedron",
  "vtkLagrangeWedge", "vtkLagrangePyramid", "vtkBezierCurve", "vtkBezierQuadrilateral",
  "vtkBezierTriangle", "vtkBezierTetra", "vtkBezierHexahedron", "vtkBezierWedge",
  "vtkBezierPyramid", nullptr };
} // anonymous namespace

//------------------------------------------------------------------------------
const char* vtkCellTypes::GetClassNameFromTypeId(int type)
{
  static int numClasses = 0;

  // find length of table
  if (numClasses == 0)
  {
    while (vtkCellTypesStrings[numClasses] != nullptr)
    {
      numClasses++;
    }
  }

  if (type < numClasses)
  {
    return vtkCellTypesStrings[type];
  }
  else
  {
    return "UnknownClass";
  }
}

//------------------------------------------------------------------------------
int vtkCellTypes::GetTypeIdFromClassName(const char* classname)
{
  if (!classname)
  {
    return -1;
  }

  for (int idx = 0; vtkCellTypesStrings[idx] != nullptr; idx++)
  {
    if (strcmp(vtkCellTypesStrings[idx], classname) == 0)
    {
      return idx;
    }
  }

  return -1;
}

//------------------------------------------------------------------------------
vtkCellTypes::vtkCellTypes()
  : TypeArray(vtkSmartPointer<vtkUnsignedCharArray>::New())
  , LocationArray(vtkSmartPointer<vtkIdTypeArray>::New())
  , MaxId(-1)
{
}

//------------------------------------------------------------------------------
int vtkCellTypes::Allocate(vtkIdType sz, vtkIdType ext)
{
  this->MaxId = -1;

  if (!this->TypeArray)
  {
    this->TypeArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
  }
  this->TypeArray->Allocate(sz, ext);

  if (!this->LocationArray)
  {
    this->LocationArray = vtkSmartPointer<vtkIdTypeArray>::New();
  }
  this->LocationArray->Allocate(sz, ext);

  return 1;
}

//------------------------------------------------------------------------------
// Add a cell at specified id.
void vtkCellTypes::InsertCell(vtkIdType cellId, unsigned char type, vtkIdType loc)
{
  vtkDebugMacro(<< "Insert Cell id: " << cellId << " at location " << loc);
  TypeArray->InsertValue(cellId, type);

  LocationArray->InsertValue(cellId, loc);

  if (cellId > this->MaxId)
  {
    this->MaxId = cellId;
  }
}

//------------------------------------------------------------------------------
// Add a cell to the object in the next available slot.
vtkIdType vtkCellTypes::InsertNextCell(unsigned char type, vtkIdType loc)
{
  vtkDebugMacro(<< "Insert Next Cell " << type << " location " << loc);
  this->InsertCell(++this->MaxId, type, loc);
  return this->MaxId;
}

//------------------------------------------------------------------------------
// Specify a group of cell types.
void vtkCellTypes::SetCellTypes(
  vtkIdType ncells, vtkUnsignedCharArray* cellTypes, vtkIntArray* cellLocations)
{
  VTK_LEGACY_BODY(vtkCellTypes::SetCellTypes, "VTK 9.2");
  this->TypeArray = cellTypes;
  if (!this->LocationArray)
  {
    this->LocationArray = vtkSmartPointer<vtkIdTypeArray>::New();
  }
  this->LocationArray->DeepCopy(cellLocations);
  this->MaxId = ncells - 1;
}

//------------------------------------------------------------------------------
// Specify a group of cell types.
void vtkCellTypes::SetCellTypes(vtkIdType ncells, vtkUnsignedCharArray* cellTypes)
{
  this->TypeArray = cellTypes;
  this->MaxId = ncells - 1;
}

//------------------------------------------------------------------------------
// Specify a group of cell types.
void vtkCellTypes::SetCellTypes(
  vtkIdType ncells, vtkUnsignedCharArray* cellTypes, vtkIdTypeArray* cellLocations)
{
  VTK_LEGACY_BODY(vtkCellTypes::SetCellTypes, "VTK 9.2");
  this->TypeArray = cellTypes;
  this->LocationArray = cellLocations;
  this->MaxId = ncells - 1;
}

//------------------------------------------------------------------------------
int vtkCellTypes::GetDimension(unsigned char type)
{
  // For the most common cell types, this is a fast call. If the cell type is
  // more exotic, then the cell must be grabbed and queried directly, which is
  // slow.
  switch (type)
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
      cell->SetCellType(type);
      return cell->GetCellDimension();
  }
}

//------------------------------------------------------------------------------
// Reclaim any extra memory.
void vtkCellTypes::Squeeze()
{
  this->TypeArray->Squeeze();
  this->LocationArray->Squeeze();
}

//------------------------------------------------------------------------------
// Initialize object without releasing memory.
void vtkCellTypes::Reset()
{
  this->MaxId = -1;
}

//------------------------------------------------------------------------------
unsigned long vtkCellTypes::GetActualMemorySize()
{
  unsigned long size = 0;

  if (this->TypeArray)
  {
    size += this->TypeArray->GetActualMemorySize();
  }

  if (this->LocationArray)
  {
    size += this->LocationArray->GetActualMemorySize();
  }

  return static_cast<unsigned long>(ceil(size / 1024.0)); // kibibytes
}

//------------------------------------------------------------------------------
void vtkCellTypes::DeepCopy(vtkCellTypes* src)
{
  if (!this->TypeArray)
  {
    this->TypeArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
  }
  this->TypeArray->DeepCopy(src->TypeArray);

  if (!this->LocationArray)
  {
    this->LocationArray = vtkSmartPointer<vtkIdTypeArray>::New();
  }
  this->LocationArray->DeepCopy(src->LocationArray);

  this->MaxId = src->MaxId;
}

//------------------------------------------------------------------------------
void vtkCellTypes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "TypeArray:\n";
  this->TypeArray->PrintSelf(os, indent.GetNextIndent());
  os << indent << "LocationArray:\n";
  this->LocationArray->PrintSelf(os, indent.GetNextIndent());

  os << indent << "MaxId: " << this->MaxId << "\n";
}
