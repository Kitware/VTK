// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor.h"

#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometryUnlimitedLevelEntry.h"
#include "vtkHyperTreeGridScales.h"

#include "vtkObjectFactory.h"

#include <cassert>

#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridOrientedGeometryCursor.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor);

//------------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor*
vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::Clone()
{
  vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor* clone = this->NewInstance();
  assert("post: clone_exists" && clone != nullptr);
  // Copy
  clone->Grid = this->Grid;
  clone->Tree = this->Tree;
  clone->Scales = this->Scales;
  clone->Level = this->Level;
  clone->LastValidEntry = this->LastValidEntry;
  clone->Entries.resize(this->Entries.size());
  auto in = this->Entries.begin();
  auto out = clone->Entries.begin();
  for (; in != this->Entries.end(); ++in, ++out)
  {
    (*out).Copy(&(*in));
  }
  // Return clone
  return clone;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::Initialize(
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

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::Initialize(vtkHyperTreeGrid* grid,
  vtkHyperTree* tree, unsigned int level, vtkHyperTreeGridGeometryUnlimitedLevelEntry& entry)
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

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::Initialize(
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
  // Initially, the index is valid
  this->Entries[0].Initialize(this->Tree, this->Level, index, origin);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::Initialize(
  vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor* cursor)
{
  this->Grid = cursor->Grid;
  this->Tree = cursor->Tree;
  this->Scales = cursor->Scales;
  this->Level = cursor->Level;
  this->LastValidEntry = cursor->LastValidEntry;
  this->Entries.resize(cursor->Entries.size());
  auto in = this->Entries.begin();
  auto out = cursor->Entries.begin();
  for (; in != this->Entries.end(); ++in, ++out)
  {
    (*out).Copy(&(*in));
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::GetVertexId()
{
  return this->Entries[this->LastValidEntry].GetVertexId();
}

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::GetGlobalNodeIndex()
{
  return this->Entries[this->LastValidEntry].GetGlobalNodeIndex();
}

//------------------------------------------------------------------------------
unsigned char vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::GetDimension()
{
  return this->Grid->GetDimension();
}

//------------------------------------------------------------------------------
unsigned char vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::GetNumberOfChildren()
{
  return this->Tree->GetNumberOfChildren();
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::SetGlobalIndexStart(vtkIdType index)
{
  this->Entries[this->LastValidEntry].SetGlobalIndexStart(index);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::SetGlobalIndexFromLocal(vtkIdType index)
{
  this->Entries[this->LastValidEntry].SetGlobalIndexFromLocal(index);
}

//------------------------------------------------------------------------------
double* vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::GetOrigin()
{
  return this->Entries[this->LastValidEntry].GetOrigin();
}

//------------------------------------------------------------------------------
double* vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::GetSize()
{
  return this->Scales->GetScale(this->Level);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::GetBounds(double bounds[6])
{
  this->Entries[this->LastValidEntry].GetBounds(bounds);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::GetPoint(double point[3])
{
  this->Entries[this->LastValidEntry].GetPoint(point);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::SetMask(bool state)
{
  this->Entries[this->LastValidEntry].SetMask(this->Grid, state);
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::IsMasked()
{
  return this->Entries[this->LastValidEntry].IsMasked(this->Grid);
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::IsLeaf()
{
  return this->Entries[this->LastValidEntry].IsLeaf(this->Grid);
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::IsRealLeaf()
{
  return this->Entries[this->LastValidEntry].IsRealLeaf(this->Grid);
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::IsVirtualLeaf()
{
  return this->Entries[this->LastValidEntry].IsVirtualLeaf(this->Grid);
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::IsRoot()
{
  return this->Entries[this->LastValidEntry].IsRoot();
}

//------------------------------------------------------------------------------
unsigned int vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::GetLevel()
{
  return this->Level;
}

//------------------------------------------------------------------------------
unsigned int vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::GetLastRealLevel()
{
  return this->Entries[this->LastValidEntry].GetLastRealLevel();
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::ToChild(unsigned char ichild)
{
  unsigned int oldLastValidEntry = this->LastValidEntry;
  this->LastValidEntry++;
  //
  if (this->Entries.size() == static_cast<size_t>(this->LastValidEntry))
  {
    this->Entries.resize(this->LastValidEntry + 1);
  }
  //
  auto& entry = this->Entries[this->LastValidEntry];
  entry.Copy(&this->Entries[oldLastValidEntry]);
  entry.ToChild(this->Grid, ichild);
  this->Level++;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::ToRoot()
{
  assert("pre: hypertree_exist" && !this->Entries.empty());
  this->Level -= this->LastValidEntry;
  this->LastValidEntry = 0;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::ToParent()
{
  assert("has: valid entry" && this->LastValidEntry > 0);
  assert("has: level" && this->Level > 0);
  this->LastValidEntry--;
  this->Level--;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "--vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor--" << endl;
  os << indent << "Level: " << this->Level << endl;
  this->Tree->PrintSelf(os, indent);
  os << indent << "LastValidEntry: " << this->LastValidEntry << endl;
  this->Entries[this->LastValidEntry].PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::Dump(ostream& os)
{
  os << "--vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor--" << endl;
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

//------------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::
  vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor()
{
  this->Grid = nullptr;
  this->Tree = nullptr;
  this->Level = 0;
  this->LastValidEntry = -1;
  // Appel au constructeur par defaut this->Entries
}

//------------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::
  ~vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor() = default;

//------------------------------------------------------------------------------
vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor>
vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::GetHyperTreeGridOrientedGeometryCursor(
  vtkHyperTreeGrid* grid)
{
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursor =
    vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor>::New();
  cursor->Initialize(grid, this->Tree, this->GetLevel(), this->GetVertexId(), this->GetOrigin());
  return cursor;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkHyperTreeGridNonOrientedGeometryCursor>
vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor::GetHyperTreeGridNonOrientedGeometryCursor(
  vtkHyperTreeGrid* grid)
{
  vtkSmartPointer<vtkHyperTreeGridNonOrientedGeometryCursor> cursor =
    vtkSmartPointer<vtkHyperTreeGridNonOrientedGeometryCursor>::New();
  cursor->Initialize(grid, this->Tree, this->GetLevel(), this->GetVertexId(), this->GetOrigin());
  return cursor;
}
VTK_ABI_NAMESPACE_END
