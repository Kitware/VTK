/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridEntry.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridEntry.h"

#include "vtkBitArray.h"

#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"

#include <cassert>

//-----------------------------------------------------------------------------
void vtkHyperTreeGridEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "--vtkHyperTreeGridEntry--" << endl;
  os << indent << "Index:" << this->Index << endl;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridEntry::Dump(ostream& os)
{
  os << "Index:" << this->Index << endl;
}

//-----------------------------------------------------------------------------
vtkHyperTree* vtkHyperTreeGridEntry::Initialize(
  vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create)
{
  assert(grid != nullptr);
  this->Index = 0;
  return grid->GetTree(treeIndex, create);
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridEntry::GetGlobalNodeIndex(const vtkHyperTree* tree) const
{
  assert("pre: not_tree" && tree);
  return tree->GetGlobalIndexFromLocal(this->Index);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridEntry::SetGlobalIndexStart(vtkHyperTree* tree, vtkIdType index)
{
  assert("pre: not_tree" && tree);
  tree->SetGlobalIndexStart(index);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridEntry::SetGlobalIndexFromLocal(vtkHyperTree* tree, vtkIdType index)
{
  assert("pre: not_tree" && tree);
  tree->SetGlobalIndexFromLocal(this->Index, index);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridEntry::SetMask(
  const vtkHyperTreeGrid* grid, const vtkHyperTree* tree, bool value)
{
  assert("pre: not_tree" && tree);
  const_cast<vtkHyperTreeGrid*>(grid)->GetMask()->InsertTuple1(
    this->GetGlobalNodeIndex(tree), value);
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridEntry::IsMasked(const vtkHyperTreeGrid* grid, const vtkHyperTree* tree) const
{
  if (tree && const_cast<vtkHyperTreeGrid*>(grid)->HasMask())
  {
    return const_cast<vtkHyperTreeGrid*>(grid)->GetMask()->GetValue(
             this->GetGlobalNodeIndex(tree)) != 0;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridEntry::IsLeaf(
  const vtkHyperTreeGrid* grid, const vtkHyperTree* tree, unsigned int level) const
{
  assert("pre: not_tree" && tree);
  if (level == const_cast<vtkHyperTreeGrid*>(grid)->GetDepthLimiter())
  {
    return true;
  }
  return tree->IsLeaf(this->Index);
}

//---------------------------------------------------------------------------
void vtkHyperTreeGridEntry::SubdivideLeaf(
  const vtkHyperTreeGrid* grid, vtkHyperTree* tree, unsigned int level)
{
  assert("pre: not_tree" && tree);
  // JB Comment faire pour definir un accesseur a DepthLimiter qui est const
  assert("pre: depth_limiter" && level <= const_cast<vtkHyperTreeGrid*>(grid)->GetDepthLimiter());
  assert("pre: is_masked" && !this->IsMasked(grid, tree));
  if (this->IsLeaf(grid, tree, level))
  {
    tree->SubdivideLeaf(this->Index, level);
  }
}

//---------------------------------------------------------------------------
bool vtkHyperTreeGridEntry::IsTerminalNode(
  const vtkHyperTreeGrid* grid, const vtkHyperTree* tree, unsigned int level) const
{
  (void)level; // remove warning for release build
  assert("pre: not_tree" && tree);
  bool result = !this->IsLeaf(grid, tree, level);
  if (result)
  {
    result = tree->IsTerminalNode(this->Index);
  }
  assert("post: compatible" && (!result || !this->IsLeaf(grid, tree, level)));
  return result;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridEntry::ToChild(
  const vtkHyperTreeGrid* grid, const vtkHyperTree* tree, unsigned int level, unsigned char ichild)
{
  (void)grid;  // only used in assert
  (void)level; // only used in assert
  assert("pre: not_tree" && tree);
  assert("pre: not_leaf" && !this->IsLeaf(grid, tree, level));
  assert("pre: not_valid_child" && ichild < tree->GetNumberOfChildren());
  // JB Comment faire pour definir un accesseur a DepthLimiter qui est const
  assert("pre: depth_limiter" && level <= const_cast<vtkHyperTreeGrid*>(grid)->GetDepthLimiter());
  assert("pre: is_masked" && !IsMasked(grid, tree));
  this->Index = tree->GetElderChildIndex(this->Index) + ichild;
}
