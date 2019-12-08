/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridNonOrientedCursor.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridNonOrientedCursor.h"

#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridEntry.h"
#include "vtkHyperTreeGridTools.h"
#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkHyperTreeGridNonOrientedCursor);

//-----------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedCursor* vtkHyperTreeGridNonOrientedCursor::Clone()
{
  vtkHyperTreeGridNonOrientedCursor* clone = this->NewInstance();
  assert("post: clone_exists" && clone != nullptr);
  // Copy
  clone->Grid = this->Grid;
  clone->Tree = this->Tree;
  clone->Level = this->Level;
  clone->LastValidEntry = this->LastValidEntry;
  clone->Entries.resize(this->Entries.size());
  std::vector<vtkHyperTreeGridEntry>::iterator in = this->Entries.begin();
  std::vector<vtkHyperTreeGridEntry>::iterator out = clone->Entries.begin();
  for (; in != this->Entries.end(); ++in, ++out)
  {
    (*out).Copy(&(*in));
  }
  // Return clone
  return clone;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedCursor::Initialize(
  vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create)
{
  this->Grid = grid;
  this->Level = 0;
  this->LastValidEntry = 0;
  this->Entries.resize(1);
  this->Tree = this->Entries[0].Initialize(grid, treeIndex, create);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedCursor::Initialize(
  vtkHyperTreeGrid* grid, vtkHyperTree* tree, unsigned int level, vtkHyperTreeGridEntry& entry)
{
  this->Grid = grid;
  this->Tree = tree;
  this->Level = level;
  this->LastValidEntry = 0;
  this->Entries.resize(1);
  this->Entries[0].Copy(&entry);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedCursor::Initialize(
  vtkHyperTreeGrid* grid, vtkHyperTree* tree, unsigned int level, vtkIdType index)
{
  assert(this->Entries.size() && "this->Entries empty");
  this->Grid = grid;
  this->Tree = tree;
  this->Level = level;
  this->LastValidEntry = 0;
  this->Entries.resize(1);
  this->Entries[0].Initialize(index);
}

//---------------------------------------------------------------------------
vtkHyperTreeGrid* vtkHyperTreeGridNonOrientedCursor::GetGrid()
{
  return this->Grid;
}

//---------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedCursor::HasTree() const
{
  return vtk::hypertreegrid::HasTree(*this);
}

//---------------------------------------------------------------------------
vtkHyperTree* vtkHyperTreeGridNonOrientedCursor::GetTree() const
{
  return this->Tree;
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridNonOrientedCursor::GetVertexId()
{
  return this->Entries[this->LastValidEntry].GetVertexId();
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridNonOrientedCursor::GetGlobalNodeIndex()
{
  return this->Entries[this->LastValidEntry].GetGlobalNodeIndex(this->Tree);
}

//-----------------------------------------------------------------------------
unsigned char vtkHyperTreeGridNonOrientedCursor::GetDimension()
{
  return this->Grid->GetDimension();
}

//-----------------------------------------------------------------------------
unsigned char vtkHyperTreeGridNonOrientedCursor::GetNumberOfChildren()
{
  return this->Tree->GetNumberOfChildren();
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedCursor::SetGlobalIndexStart(vtkIdType index)
{
  this->Entries[this->LastValidEntry].SetGlobalIndexStart(this->Tree, index);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedCursor::SetGlobalIndexFromLocal(vtkIdType index)
{
  this->Entries[this->LastValidEntry].SetGlobalIndexFromLocal(this->Tree, index);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedCursor::SetMask(bool state)
{
  this->Entries[this->LastValidEntry].SetMask(this->Grid, this->Tree, state);
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedCursor::IsMasked()
{
  return this->Entries[this->LastValidEntry].IsMasked(this->Grid, this->Tree);
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedCursor::IsLeaf()
{
  return this->Entries[this->LastValidEntry].IsLeaf(this->Grid, this->Tree, this->Level);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedCursor::SubdivideLeaf()
{
  this->Entries[this->LastValidEntry].SubdivideLeaf(this->Grid, this->Tree, this->Level);
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedCursor::IsRoot()
{
  return this->Entries[this->LastValidEntry].IsRoot();
}

//-----------------------------------------------------------------------------
unsigned int vtkHyperTreeGridNonOrientedCursor::GetLevel()
{
  return this->Level;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedCursor::ToChild(unsigned char ichild)
{
  unsigned int oldLastValidEntry = this->LastValidEntry;
  this->LastValidEntry++;
  //
  if (this->Entries.size() == static_cast<size_t>(this->LastValidEntry))
  {
    this->Entries.resize(this->LastValidEntry + 1);
  }
  //
  vtkHyperTreeGridEntry& entry = this->Entries[this->LastValidEntry];
  entry.Copy(&this->Entries[oldLastValidEntry]);
  entry.ToChild(this->Grid, this->Tree, this->Level, ichild);
  this->Level++;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedCursor::ToRoot()
{
  assert("pre: hypertree_exist" && this->Entries.size() > 0);
  this->LastValidEntry = 0;
  this->Level = 0;
}

//---------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedCursor::ToParent()
{
  assert("pre: not_root" && !this->IsRoot());
  this->LastValidEntry--;
  this->Level--;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedCursor::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "--vtkHyperTreeGridNonOrientedCursor--" << endl;
  os << indent << "Level: " << this->GetLevel() << endl;
  this->Tree->PrintSelf(os, indent);
  if (this->Entries.size())
  {
    os << indent << "LastValidEntry: " << this->LastValidEntry << endl;
    this->Entries[this->LastValidEntry].PrintSelf(os, indent);
  }
  else
  {
    os << indent << "No valid entry " << std::endl;
  }
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedCursor::vtkHyperTreeGridNonOrientedCursor()
{
  this->Grid = nullptr;
  this->Tree = nullptr;
  this->Level = 0;
  this->LastValidEntry = -1;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedCursor::~vtkHyperTreeGridNonOrientedCursor() {}

//-----------------------------------------------------------------------------
