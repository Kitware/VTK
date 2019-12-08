/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridLevelEntry.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridLevelEntry.h"

#include "vtkBitArray.h"

#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"

#include <cassert>

//-----------------------------------------------------------------------------
vtkHyperTreeGridLevelEntry::vtkHyperTreeGridLevelEntry(
  vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create)
  : Tree(grid->GetTree(treeIndex, create))
  , Level(0)
  , Index(0)
{
}

//-----------------------------------------------------------------------------

vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor>
vtkHyperTreeGridLevelEntry::GetHyperTreeGridNonOrientedCursor(vtkHyperTreeGrid* grid)
{
  // JB assert ( "pre: level==0" && this->Level == 0 );
  vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor> cursor =
    vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor>::New();
  cursor->Initialize(grid, this->GetTree(), this->Level, this->Index);
  return cursor;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridLevelEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "--vtkHyperTreeGridLevelEntry--" << endl;
  this->Tree->PrintSelf(os, indent);
  os << indent << "Level:" << this->Level << endl;
  os << indent << "Index:" << this->Index << endl;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridLevelEntry::Dump(ostream& os)
{
  os << "Level:" << this->Level << endl;
  os << "Index:" << this->Index << endl;
}

//-----------------------------------------------------------------------------
vtkHyperTree* vtkHyperTreeGridLevelEntry::Initialize(
  vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create)
{
  this->Tree = grid->GetTree(treeIndex, create);
  this->Level = 0;
  this->Index = 0;
  return this->Tree;
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridLevelEntry::GetGlobalNodeIndex() const
{
  // JB BAD assert( "pre: not_tree" &&
  //     JB BAD     this->Tree );
  // Pourquoi ceci juste dans cette fonction entry ?
  if (this->Tree)
  {
    return this->Tree->GetGlobalIndexFromLocal(this->Index);
  }
  return -1;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridLevelEntry::SetGlobalIndexStart(vtkIdType index)
{
  assert("pre: not_tree" && this->Tree);
  this->Tree->SetGlobalIndexStart(index);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridLevelEntry::SetGlobalIndexFromLocal(vtkIdType index)
{
  assert("pre: not_tree" && this->Tree);
  this->Tree->SetGlobalIndexFromLocal(this->Index, index);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridLevelEntry::SetMask(const vtkHyperTreeGrid* grid, bool value)
{
  assert("pre: not_tree" && this->Tree);
  // JB Comment faire pour definir un accesseur a DepthLimiter qui est const
  const_cast<vtkHyperTreeGrid*>(grid)->GetMask()->InsertTuple1(this->GetGlobalNodeIndex(), value);
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridLevelEntry::IsMasked(const vtkHyperTreeGrid* grid) const
{
  // JB Comment faire pour definir un accesseur a DepthLimiter qui est const
  if (this->Tree && const_cast<vtkHyperTreeGrid*>(grid)->HasMask())
  {
    return const_cast<vtkHyperTreeGrid*>(grid)->GetMask()->GetValue(this->GetGlobalNodeIndex()) !=
      0;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridLevelEntry::IsLeaf(const vtkHyperTreeGrid* grid) const
{
  assert("pre: not_tree" && this->Tree);
  // How to set an accessor to DepthLimiter which is const?
  if (this->Level == const_cast<vtkHyperTreeGrid*>(grid)->GetDepthLimiter())
  {
    return true;
  }
  return this->Tree->IsLeaf(this->Index);
}

//---------------------------------------------------------------------------
void vtkHyperTreeGridLevelEntry::SubdivideLeaf(const vtkHyperTreeGrid* grid)
{
  assert("pre: not_tree" && this->Tree);
  // JB Comment faire pour definir un accesseur a DepthLimiter qui est const
  assert(
    "pre: depth_limiter" && this->Level <= const_cast<vtkHyperTreeGrid*>(grid)->GetDepthLimiter());
  assert("pre: is_masked" && !this->IsMasked(grid));
  if (this->IsLeaf(grid))
  {
    this->Tree->SubdivideLeaf(this->Index, this->Level);
  }
}

//---------------------------------------------------------------------------
bool vtkHyperTreeGridLevelEntry::IsTerminalNode(const vtkHyperTreeGrid* grid) const
{
  assert("pre: not_tree" && this->Tree);
  bool result = !this->IsLeaf(grid);
  if (result)
  {
    result = this->Tree->IsTerminalNode(this->Index);
  }
  assert("post: compatible" && (!result || !this->IsLeaf(grid)));
  return result;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridLevelEntry::ToChild(const vtkHyperTreeGrid* grid, unsigned char ichild)
{
  (void)grid; // Used in assert
  assert("pre: not_tree" && this->Tree);
  assert("pre: not_leaf" && !this->IsLeaf(grid));
  assert("pre: valid_child" && ichild < this->Tree->GetNumberOfChildren());
  // JB Comment faire pour definir un accesseur a DepthLimiter qui est const
  assert(
    "pre: depth_limiter" && this->Level <= const_cast<vtkHyperTreeGrid*>(grid)->GetDepthLimiter());
  assert("pre: is_masked" && !this->IsMasked(grid));
  this->Index = this->Tree->GetElderChildIndex(this->Index) + ichild;
  this->Level++;
}
