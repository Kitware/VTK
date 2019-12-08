/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridNonOrientedGeometryCursor.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright Nonice for more information.

=========================================================================*/
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"

#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometryEntry.h"
#include "vtkHyperTreeGridScales.h"

#include "vtkObjectFactory.h"

#include <cassert>

#include "vtkHyperTreeGridOrientedGeometryCursor.h"

vtkStandardNewMacro(vtkHyperTreeGridNonOrientedGeometryCursor);

//-----------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedGeometryCursor* vtkHyperTreeGridNonOrientedGeometryCursor::Clone()
{
  vtkHyperTreeGridNonOrientedGeometryCursor* clone = this->NewInstance();
  assert("post: clone_exists" && clone != nullptr);
  // Copy
  clone->Grid = this->Grid;
  clone->Tree = this->Tree;
  clone->Scales = this->Scales;
  clone->Level = this->Level;
  clone->LastValidEntry = this->LastValidEntry;
  clone->Entries.resize(this->Entries.size());
  std::vector<vtkHyperTreeGridGeometryEntry>::iterator in = this->Entries.begin();
  std::vector<vtkHyperTreeGridGeometryEntry>::iterator out = clone->Entries.begin();
  for (; in != this->Entries.end(); ++in, ++out)
  {
    (*out).Copy(&(*in));
  }
  // Return clone
  return clone;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedGeometryCursor::Initialize(
  vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create)
{
  this->Grid = grid;
  this->LastValidEntry = 0;
  if (this->Entries.size() <= static_cast<size_t>(this->LastValidEntry))
  {
    this->Entries.resize(1);
  }
  this->Tree = this->Entries[0].Initialize(grid, treeIndex, create);
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
void vtkHyperTreeGridNonOrientedGeometryCursor::Initialize(vtkHyperTreeGrid* grid,
  vtkHyperTree* tree, unsigned int level, vtkHyperTreeGridGeometryEntry& entry)
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
  this->LastValidEntry = 0;
  this->Entries.resize(1);
  this->Entries[0].Copy(&entry);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedGeometryCursor::Initialize(
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
  this->LastValidEntry = 0;
  this->Entries.resize(1);
  this->Entries[0].Initialize(index, origin);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedGeometryCursor::Initialize(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  this->Grid = cursor->Grid;
  this->Tree = cursor->Tree;
  this->Scales = cursor->Scales;
  this->Level = cursor->Level;
  this->LastValidEntry = cursor->LastValidEntry;
  this->Entries.resize(cursor->Entries.size());
  std::vector<vtkHyperTreeGridGeometryEntry>::iterator in = this->Entries.begin();
  std::vector<vtkHyperTreeGridGeometryEntry>::iterator out = cursor->Entries.begin();
  for (; in != this->Entries.end(); ++in, ++out)
  {
    (*out).Copy(&(*in));
  }
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridNonOrientedGeometryCursor::GetVertexId()
{
  return this->Entries[this->LastValidEntry].GetVertexId();
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridNonOrientedGeometryCursor::GetGlobalNodeIndex()
{
  return this->Entries[this->LastValidEntry].GetGlobalNodeIndex(this->Tree);
}

//-----------------------------------------------------------------------------
unsigned char vtkHyperTreeGridNonOrientedGeometryCursor::GetDimension()
{
  return this->Grid->GetDimension();
}

//-----------------------------------------------------------------------------
unsigned char vtkHyperTreeGridNonOrientedGeometryCursor::GetNumberOfChildren()
{
  return this->Tree->GetNumberOfChildren();
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedGeometryCursor::SetGlobalIndexStart(vtkIdType index)
{
  this->Entries[this->LastValidEntry].SetGlobalIndexStart(this->Tree, index);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedGeometryCursor::SetGlobalIndexFromLocal(vtkIdType index)
{
  this->Entries[this->LastValidEntry].SetGlobalIndexFromLocal(this->Tree, index);
}

//-----------------------------------------------------------------------------
double* vtkHyperTreeGridNonOrientedGeometryCursor::GetOrigin()
{
  return this->Entries[this->LastValidEntry].GetOrigin();
}

//-----------------------------------------------------------------------------
double* vtkHyperTreeGridNonOrientedGeometryCursor::GetSize()
{
  return this->Scales->GetScale(this->Level);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedGeometryCursor::GetBounds(double bounds[6])
{
  this->Entries[this->LastValidEntry].GetBounds(this->GetSize(), bounds);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedGeometryCursor::GetPoint(double point[3])
{
  this->Entries[this->LastValidEntry].GetPoint(this->GetSize(), point);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedGeometryCursor::SetMask(bool state)
{
  this->Entries[this->LastValidEntry].SetMask(this->Grid, this->Tree, state);
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedGeometryCursor::IsMasked()
{
  return this->Entries[this->LastValidEntry].IsMasked(this->Grid, this->Tree);
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedGeometryCursor::IsLeaf()
{
  return this->Entries[this->LastValidEntry].IsLeaf(this->Grid, this->Tree, this->Level);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedGeometryCursor::SubdivideLeaf()
{
  this->Entries[this->LastValidEntry].SubdivideLeaf(this->Grid, this->Tree, Level);
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedGeometryCursor::IsRoot()
{
  return this->Entries[this->LastValidEntry].IsRoot();
}

//-----------------------------------------------------------------------------
unsigned int vtkHyperTreeGridNonOrientedGeometryCursor::GetLevel()
{
  return this->Level;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedGeometryCursor::ToChild(unsigned char ichild)
{
  unsigned int oldLastValidEntry = this->LastValidEntry;
  this->LastValidEntry++;
  //
  if (this->Entries.size() == static_cast<size_t>(this->LastValidEntry))
  {
    this->Entries.resize(this->LastValidEntry + 1);
  }
  //
  vtkHyperTreeGridGeometryEntry& entry = this->Entries[this->LastValidEntry];
  entry.Copy(&this->Entries[oldLastValidEntry]);
  entry.ToChild(
    this->Grid, this->Tree, this->Level, this->Scales->GetScale(this->Level + 1), ichild);
  this->Level++;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedGeometryCursor::ToRoot()
{
  assert("pre: hypertree_exist" && this->Entries.size() > 0);
  this->Level -= this->LastValidEntry;
  this->LastValidEntry = 0;
}

//---------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedGeometryCursor::ToParent()
{
  assert("pre: Non_root" && !this->IsRoot());
  this->LastValidEntry--;
  this->Level--;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedGeometryCursor::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "--vtkHyperTreeGridNonOrientedGeometryCursor--" << endl;
  os << indent << "Level: " << this->Level << endl;
  this->Tree->PrintSelf(os, indent);
  os << indent << "LastValidEntry: " << this->LastValidEntry << endl;
  this->Entries[this->LastValidEntry].PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedGeometryCursor::Dump(ostream& os)
{
  os << "--vtkHyperTreeGridNonOrientedGeometryCursor--" << endl;
  os << "Grid: " << this->Grid << endl;
  os << "Tree: " << this->Tree << endl;
  os << "Scales: " << this->Scales << endl;
  os << "Level: " << this->Level << endl;
  os << "LastValidEntry: " << this->LastValidEntry << endl;
  int ientry = 0;
  for (; ientry <= this->LastValidEntry; ++ientry)
  {
    os << "Entries: #" << ientry << endl;
    this->Entries[ientry].Dump(os);
  }
  for (; static_cast<size_t>(ientry) < this->Entries.size(); ++ientry)
  {
    os << "Entries: #" << ientry << " Non USED" << endl;
    this->Entries[ientry].Dump(os);
  }
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedGeometryCursor::vtkHyperTreeGridNonOrientedGeometryCursor()
{
  this->Grid = nullptr;
  this->Tree = nullptr;
  this->Level = 0;
  this->LastValidEntry = -1;
  // Appel au constructeur par defaut this->Entries
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedGeometryCursor::~vtkHyperTreeGridNonOrientedGeometryCursor() {}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor>
vtkHyperTreeGridNonOrientedGeometryCursor::GetHyperTreeGridOrientedGeometryCursor(
  vtkHyperTreeGrid* grid)
{
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursor =
    vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor>::New();
  cursor->Initialize(grid, this->Tree, this->GetLevel(), this->GetVertexId(), this->GetOrigin());
  return cursor;
}
