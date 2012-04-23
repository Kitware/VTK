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
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCellTypes);

// This list should contain the cell class names in
// the same order as the enums in vtkCellType.h. Make sure
// this list is NULL terminated.
static const char* vtkCellTypesStrings[] = {
  "vtkEmptyCell",
  "vtkVertex",
  "vtkPolyVertex",
  "vtkLine",
  "vtkPolyLine",
  "vtkTriangle",
  "vtkTriangleStrip",
  "vtkPolygon",
  "vtkPixel",
  "vtkQuad",
  "vtkTetra",
  "vtkVoxel",
  "vtkHexahedron",
  "vtkWedge",
  "vtkPyramid",
  "vtkPentagonalPrism",
  "vtkHexagonalPrism",
  "UnknownClass",
  "UnknownClass",
  "UnknownClass",
  "UnknownClass",
  "vtkQuadraticEdge",
  "vtkQuadraticTriangle",
  "vtkQuadraticQuad",
  "vtkQuadraticTetra",
  "vtkQuadraticHexahedron",
  "vtkQuadraticWedge",
  "vtkQuadraticPyramid",
  "vtkBiQuadraticQuad",
  "vtkTriQuadraticHexahedron",
  "vtkQuadraticLinearQuad",
  "vtkQuadraticLinearWedge",
  "vtkBiQuadraticQuadraticWedge",
  "vtkBiQuadraticQuadraticHexahedron",
  "vtkBiQuadraticTriangle",
  "vtkCubicLine",
  "UnknownClass",
  "UnknownClass",
  "UnknownClass",
  "UnknownClass",
  "UnknownClass",
  "vtkConvexPointSet",
  "UnknownClass",
  "UnknownClass",
  "UnknownClass",
  "UnknownClass",
  "UnknownClass",
  "UnknownClass",
  "UnknownClass",
  "UnknownClass",
  "UnknownClass",
  "vtkParametricCurve",
  "vtkParametricSurface",
  "vtkParametricTriSurface",
  "vtkParametricQuadSurface",
  "vtkParametricTetraRegion",
  "vtkParametricHexRegion",
  "UnknownClass",
  "UnknownClass",
  "UnknownClass",
  "vtkHigherOrderEdge",
  "vtkHigherOrderTriangle",
  "vtkHigherOrderQuad",
  "vtkHigherOrderPolygon",
  "vtkHigherOrderTetrahedron",
  "vtkHigherOrderWedge",
  "vtkHigherOrderPyramid",
  "vtkHigherOrderHexahedron",
  NULL
};

//----------------------------------------------------------------------------
const char* vtkCellTypes::GetClassNameFromTypeId(int type)
{
  static int numClasses = 0;

  // find length of table
  if (numClasses == 0)
    {
    while (vtkCellTypesStrings[numClasses] != NULL)
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

//----------------------------------------------------------------------------
int vtkCellTypes::GetTypeIdFromClassName(const char* classname)
{
  if (!classname)
    {
    return -1;
    }

  for(int idx=0; vtkCellTypesStrings[idx] != NULL; idx++)
    {
    if (strcmp(vtkCellTypesStrings[idx], classname) == 0)
      {
      return idx;
      }
    }

  return -1;
}

//----------------------------------------------------------------------------
vtkCellTypes::vtkCellTypes ()
{

  this->TypeArray = NULL;
  this->LocationArray = NULL;
  this->Size = 0;
  this->MaxId = -1;
  this->Extend = 1000;
  this->Allocate(1000,this->Extend);

}

//----------------------------------------------------------------------------
vtkCellTypes::~vtkCellTypes()
{

  if ( this->TypeArray )
    {
    this->TypeArray->UnRegister(this);
    this->TypeArray = NULL;
    }

  if ( this->LocationArray )
    {
    this->LocationArray->UnRegister(this);
    this->LocationArray = NULL;
    }

}

//----------------------------------------------------------------------------
// Allocate memory for this array. Delete old storage only if necessary.
int vtkCellTypes::Allocate(int sz, int ext)
{

  this->Size = ( sz > 0 ? sz : 1);
  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;

  if ( this->TypeArray )
    {
    this->TypeArray->UnRegister(this);
    }
  this->TypeArray = vtkUnsignedCharArray::New();
  this->TypeArray->Allocate(sz,ext);
  this->TypeArray->Register(this);
  this->TypeArray->Delete();

  if ( this->LocationArray )
    {
    this->LocationArray->UnRegister(this);
    }
  this->LocationArray = vtkIntArray::New();
  this->LocationArray->Allocate(sz,ext);
  this->LocationArray->Register(this);
  this->LocationArray->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// Add a cell at specified id.
void vtkCellTypes::InsertCell(int cellId, unsigned char type, int loc)
{
  vtkDebugMacro(<<"Insert Cell id: " << cellId << " at location " << loc);
  TypeArray->InsertValue(cellId, type);

  LocationArray->InsertValue(cellId, loc);

  if ( cellId > this->MaxId )
    {
    this->MaxId = cellId;
    }
  return;
}

//----------------------------------------------------------------------------
// Add a cell to the object in the next available slot.
int vtkCellTypes::InsertNextCell(unsigned char type, int loc)
{
  vtkDebugMacro(<<"Insert Next Cell " << type << " location " << loc);
  this->InsertCell (++this->MaxId,type,loc);
  return this->MaxId;
}

//----------------------------------------------------------------------------
// Specify a group of cell types.
void vtkCellTypes::SetCellTypes(int ncells, vtkUnsignedCharArray *cellTypes, vtkIntArray *cellLocations)
{
  this->Size = ncells;

  if (this->TypeArray)
    {
    this->TypeArray->Delete();
    }

  this->TypeArray = cellTypes;
  cellTypes->Register(this);

  if (this->LocationArray)
    {
    this->LocationArray->Delete();
    }
  this->LocationArray = cellLocations;
  cellLocations->Register(this);
  this->Extend = 1;
  this->MaxId = -1;

}

//----------------------------------------------------------------------------
// Reclaim any extra memory.
void vtkCellTypes::Squeeze()
{
  this->TypeArray->Squeeze();
  this->LocationArray->Squeeze();
}

//----------------------------------------------------------------------------
// Initialize object without releasing memory.
void vtkCellTypes::Reset()
{
  this->MaxId = -1;
}

//----------------------------------------------------------------------------
unsigned long vtkCellTypes::GetActualMemorySize()
{
  unsigned long size=0;

  if ( this->TypeArray )
    {
    size += this->TypeArray->GetActualMemorySize();
    }

  if ( this->LocationArray )
    {
    size += this->LocationArray->GetActualMemorySize();
    }

  return static_cast<unsigned long>(ceil(size/1024.0)); //kilobytes
}


//----------------------------------------------------------------------------
void vtkCellTypes::DeepCopy(vtkCellTypes *src)
{
  if (this->TypeArray)
    {
      this->TypeArray->UnRegister(this);
      this->TypeArray = NULL;
    }
  if (src->TypeArray)
    {
      this->TypeArray = vtkUnsignedCharArray::New();
      this->TypeArray->DeepCopy(src->TypeArray);
      this->TypeArray->Register(this);
      this->TypeArray->Delete();
    }

  if (this->LocationArray)
    {
      this->LocationArray->UnRegister(this);
      this->LocationArray = NULL;
    }
  if (src->LocationArray)
    {
      this->LocationArray = vtkIntArray::New();
      this->LocationArray->DeepCopy(src->LocationArray);
      this->LocationArray->Register(this);
      this->LocationArray->Delete();
    }

  this->Allocate(src->Size, src->Extend);
  this->MaxId = src->MaxId;
}

//----------------------------------------------------------------------------
void vtkCellTypes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "TypeArray:\n";
  this->TypeArray->PrintSelf(os,indent.GetNextIndent());
  os << indent << "LocationArray:\n";
  this->LocationArray->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Size: " << this->Size << "\n";
  os << indent << "MaxId: " << this->MaxId << "\n";
  os << indent << "Extend: " << this->Extend << "\n";
}
