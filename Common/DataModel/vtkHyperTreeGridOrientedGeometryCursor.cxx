/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridOrientedGeometryCursor.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright Nonice for more information.

=========================================================================*/
#include "vtkHyperTreeGridOrientedGeometryCursor.h"

#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridScales.h"

#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkHyperTreeGridOrientedGeometryCursor);

//-----------------------------------------------------------------------------
vtkHyperTreeGridOrientedGeometryCursor* vtkHyperTreeGridOrientedGeometryCursor::Clone()
{
  vtkHyperTreeGridOrientedGeometryCursor* clone = this->NewInstance();
  assert("post: clone_exists" && clone != nullptr);
  // Copy
  clone->Grid = this->Grid;
  clone->Tree = this->Tree;
  clone->Scales = this->Scales;
  clone->Level = this->Level;
  clone->Entry.Copy(&(this->Entry));
  // Return clone
  return clone;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedGeometryCursor::Initialize(
  vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create)
{
  this->Grid = grid;
  this->Level = 0;
  this->Tree = this->Entry.Initialize(grid, treeIndex, create);
  if (this->Tree)
  {
    this->Scales = this->Tree->GetScales();
    assert(this->Scales);
  }
  else
  {
    this->Scales = nullptr;
  }
  this->Level = 0;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedGeometryCursor::Initialize(vtkHyperTreeGrid* grid, vtkHyperTree* tree,
  unsigned int level, vtkHyperTreeGridGeometryEntry& entry)
{
  this->Grid = grid;
  this->Tree = tree;
  if (this->Tree)
  {
    this->Scales = this->Tree->GetScales();
    assert(this->Scales);
  }
  else
  {
    this->Scales = nullptr;
  }
  this->Level = level;
  this->Entry.Copy(&entry);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedGeometryCursor::Initialize(
  vtkHyperTreeGrid* grid, vtkHyperTree* tree, unsigned int level, vtkIdType index, double* origin)
{
  this->Grid = grid;
  this->Tree = tree;
  if (this->Tree)
  {
    this->Scales = this->Tree->GetScales();
    assert(this->Scales);
  }
  else
  {
    this->Scales = nullptr;
  }
  this->Level = level;
  this->Entry.Initialize(index, origin);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedGeometryCursor::Initialize(
  vtkHyperTreeGridOrientedGeometryCursor* cursor)
{
  this->Grid = cursor->Grid;
  this->Tree = cursor->Tree;
  this->Scales = cursor->Scales;
  this->Level = cursor->Level;
  this->Entry.Copy(&(cursor->Entry));
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridOrientedGeometryCursor::GetVertexId()
{
  return this->Entry.GetVertexId();
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridOrientedGeometryCursor::GetGlobalNodeIndex()
{
  return this->Entry.GetGlobalNodeIndex(this->Tree);
}

//-----------------------------------------------------------------------------
unsigned char vtkHyperTreeGridOrientedGeometryCursor::GetDimension()
{
  return this->Grid->GetDimension();
}

//-----------------------------------------------------------------------------
unsigned char vtkHyperTreeGridOrientedGeometryCursor::GetNumberOfChildren()
{
  return this->Tree->GetNumberOfChildren();
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedGeometryCursor::SetGlobalIndexStart(vtkIdType index)
{
  this->Entry.SetGlobalIndexStart(this->Tree, index);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedGeometryCursor::SetGlobalIndexFromLocal(vtkIdType index)
{
  this->Entry.SetGlobalIndexFromLocal(this->Tree, index);
}

//-----------------------------------------------------------------------------
double* vtkHyperTreeGridOrientedGeometryCursor::GetOrigin()
{
  return this->Entry.GetOrigin();
}

//-----------------------------------------------------------------------------
double* vtkHyperTreeGridOrientedGeometryCursor::GetSize()
{
  return (double*)(this->Scales->GetScale(this->GetLevel()));
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedGeometryCursor::GetBounds(double bounds[6])
{
  this->Entry.GetBounds(this->GetSize(), bounds);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedGeometryCursor::GetPoint(double point[3])
{
  this->Entry.GetPoint(this->GetSize(), point);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedGeometryCursor::SetMask(bool state)
{
  this->Entry.SetMask(this->Grid, this->Tree, state);
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridOrientedGeometryCursor::IsMasked()
{
  return this->Entry.IsMasked(this->Grid, this->Tree);
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridOrientedGeometryCursor::IsLeaf()
{
  return this->Entry.IsLeaf(this->Grid, this->Tree, this->Level);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedGeometryCursor::SubdivideLeaf()
{
  this->Entry.SubdivideLeaf(this->Grid, this->Tree, this->Level);
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridOrientedGeometryCursor::IsRoot()
{
  return this->Entry.IsRoot();
}

//-----------------------------------------------------------------------------
unsigned int vtkHyperTreeGridOrientedGeometryCursor::GetLevel()
{
  return this->Level;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedGeometryCursor::ToChild(unsigned char ichild)
{
  this->Entry.ToChild(
    this->Grid, this->Tree, this->Level, this->Scales->GetScale(this->Level + 1), ichild);
  this->Level++;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedGeometryCursor::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "--vtkHyperTreeGridOrientedGeometryCursor--" << endl;
  os << indent << "Level: " << this->GetLevel() << endl;
  this->Tree->PrintSelf(os, indent);
  this->Entry.PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedGeometryCursor::Dump(ostream& os)
{
  os << "--vtkHyperTreeGridOrientedGeometryCursor--" << endl;
  os << "Grid: " << this->Grid << endl;
  os << "Tree: " << this->Tree << endl;
  os << "Scales: " << this->Scales << endl;
  os << "Level: " << this->Level << endl;
  os << "Entry: " << endl;
  this->Entry.Dump(os);
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridOrientedGeometryCursor::vtkHyperTreeGridOrientedGeometryCursor()
{
  this->Grid = nullptr;
  this->Tree = nullptr;
  this->Level = 0;
  // Appel au constructeur par defaut this->Entry
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridOrientedGeometryCursor::~vtkHyperTreeGridOrientedGeometryCursor() {}

//-----------------------------------------------------------------------------
