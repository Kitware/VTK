/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridOrientedCursor.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridOrientedCursor.h"

#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridEntry.h"
#include "vtkHyperTreeGridTools.h"
#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkHyperTreeGridOrientedCursor);

//-----------------------------------------------------------------------------
vtkHyperTreeGridOrientedCursor* vtkHyperTreeGridOrientedCursor::Clone()
{
  vtkHyperTreeGridOrientedCursor* clone = this->NewInstance();
  assert("post: clone_exists" && clone != nullptr);
  // Copy
  clone->Grid = this->Grid;
  clone->Tree = this->Tree;
  clone->Level = this->Level;
  clone->Entry.Copy(&(this->Entry));
  // Return clone
  return clone;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedCursor::Initialize(
  vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create)
{
  this->Grid = grid;
  this->Level = 0;
  this->Tree = this->Entry.Initialize(grid, treeIndex, create);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedCursor::Initialize(
  vtkHyperTreeGrid* grid, vtkHyperTree* tree, unsigned int level, vtkHyperTreeGridEntry& entry)
{
  this->Grid = grid;
  this->Tree = tree;
  this->Level = level;
  this->Entry.Copy(&entry);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedCursor::Initialize(
  vtkHyperTreeGrid* grid, vtkHyperTree* tree, unsigned int level, vtkIdType index)
{
  this->Grid = grid;
  this->Tree = tree;
  this->Level = level;
  this->Entry.Initialize(index);
}

//---------------------------------------------------------------------------
vtkHyperTreeGrid* vtkHyperTreeGridOrientedCursor::GetGrid()
{
  return this->Grid;
}

//---------------------------------------------------------------------------
bool vtkHyperTreeGridOrientedCursor::HasTree() const
{
  return vtk::hypertreegrid::HasTree(*this);
}

//---------------------------------------------------------------------------
vtkHyperTree* vtkHyperTreeGridOrientedCursor::GetTree() const
{
  return this->Tree;
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridOrientedCursor::GetVertexId()
{
  return this->Entry.GetVertexId();
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridOrientedCursor::GetGlobalNodeIndex()
{
  return this->Entry.GetGlobalNodeIndex(this->Tree);
}

//-----------------------------------------------------------------------------
unsigned char vtkHyperTreeGridOrientedCursor::GetDimension()
{
  return this->Grid->GetDimension();
}

//-----------------------------------------------------------------------------
unsigned char vtkHyperTreeGridOrientedCursor::GetNumberOfChildren()
{
  return this->Tree->GetNumberOfChildren();
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedCursor::SetGlobalIndexStart(vtkIdType index)
{
  this->Entry.SetGlobalIndexStart(this->Tree, index);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedCursor::SetGlobalIndexFromLocal(vtkIdType index)
{
  this->Entry.SetGlobalIndexFromLocal(this->Tree, index);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedCursor::SetMask(bool state)
{
  this->Entry.SetMask(this->Grid, this->Tree, state);
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridOrientedCursor::IsMasked()
{
  return this->Entry.IsMasked(this->Grid, this->Tree);
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridOrientedCursor::IsLeaf()
{
  return this->Entry.IsLeaf(this->Grid, this->Tree, this->Level);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedCursor::SubdivideLeaf()
{
  this->Entry.SubdivideLeaf(this->Grid, this->Tree, this->Level);
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridOrientedCursor::IsRoot()
{
  return this->Entry.IsRoot();
}

//-----------------------------------------------------------------------------
unsigned int vtkHyperTreeGridOrientedCursor::GetLevel()
{
  return this->Level;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedCursor::ToChild(unsigned char ichild)
{
  this->Entry.ToChild(this->Grid, this->Tree, this->Level, ichild);
  this->Level++;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridOrientedCursor::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "--vtkHyperTreeGridOrientedCursor--" << endl;
  os << indent << "Level: " << this->GetLevel() << endl;
  this->Tree->PrintSelf(os, indent);
  this->Entry.PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridOrientedCursor::vtkHyperTreeGridOrientedCursor()
{
  this->Grid = nullptr;
  this->Level = 0;
  this->Tree = nullptr;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridOrientedCursor::~vtkHyperTreeGridOrientedCursor() {}

//-----------------------------------------------------------------------------
