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

#include "vtkCellTypes.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkCellTypes);

namespace
{
// This list should contain the cell class names in
// the same order as the enums in vtkCellType.h. Make sure
// this list is nullptr terminated.
static const char* vtkCellTypesStrings[] = { "vtkEmptyCell", "vtkVertex", "vtkPolyVertex",
  "vtkLine", "vtkPolyLine", "vtkTriangle", "vtkTriangleStrip", "vtkPolygon", "vtkPixel", "vtkQuad",
  "vtkTetra", "vtkVoxel", "vtkHexahedron", "vtkWedge", "vtkPyramid", "vtkPentagonalPrism",
  "vtkHexagonalPrism", "UnknownClass", "UnknownClass", "UnknownClass", "UnknownClass",
  "vtkQuadraticEdge", "vtkQuadraticTriangle", "vtkQuadraticQuad", "vtkQuadraticTetra",
  "vtkQuadraticHexahedron", "vtkQuadraticWedge", "vtkQuadraticPyramid", "vtkBiQuadraticQuad",
  "vtkTriQuadraticHexahedron", "vtkQuadraticLinearQuad", "vtkQuadraticLinearWedge",
  "vtkBiQuadraticQuadraticWedge", "vtkBiQuadraticQuadraticHexahedron", "vtkBiQuadraticTriangle",
  "vtkCubicLine", "vtkQuadraticPolygon", "vtkTriQuadraticPyramid", "UnknownClass", "UnknownClass",
  "UnknownClass", "vtkConvexPointSet", "vtkPolyhedron", "UnknownClass", "UnknownClass",
  "UnknownClass", "UnknownClass", "UnknownClass", "UnknownClass", "UnknownClass", "UnknownClass",
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
void vtkCellTypes::SetCellTypes(
  vtkIdType ncells, vtkUnsignedCharArray* cellTypes, vtkIdTypeArray* cellLocations)
{
  this->TypeArray = cellTypes;
  this->LocationArray = cellLocations;
  this->MaxId = ncells - 1;
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
