/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridGeometryEntry.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridGeometryEntry.h"

#include "vtkBitArray.h"

#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"

#include <cassert>

//-----------------------------------------------------------------------------
vtkHyperTreeGridGeometryEntry::vtkHyperTreeGridGeometryEntry()
{
  this->Index = 0;
  for (unsigned int d = 0; d < 3; ++d)
  {
    this->Origin[d] = 0.;
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridGeometryEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "--vtkHyperTreeGridGeometryLevelEntry--" << endl;
  os << indent << "Index:" << this->Index << endl;
  os << indent << "Origin:" << this->Origin[0] << ", " << this->Origin[1] << ", " << this->Origin[2]
     << endl;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridGeometryEntry::Dump(ostream& os)
{
  os << "Index:" << this->Index << endl;
  os << "Origin:" << this->Origin[0] << ", " << this->Origin[1] << ", " << this->Origin[2] << endl;
}

//-----------------------------------------------------------------------------
vtkHyperTree* vtkHyperTreeGridGeometryEntry::Initialize(
  vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create)
{
  this->Index = 0;
  grid->GetLevelZeroOriginFromIndex(treeIndex, this->Origin);
  return grid->GetTree(treeIndex, create);
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometryEntry::GetGlobalNodeIndex(const vtkHyperTree* tree) const
{
  assert("pre: not_tree" && tree);
  return tree->GetGlobalIndexFromLocal(this->Index);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridGeometryEntry::SetGlobalIndexStart(vtkHyperTree* tree, vtkIdType index)
{
  assert("pre: not_tree" && tree);
  tree->SetGlobalIndexStart(index);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridGeometryEntry::SetGlobalIndexFromLocal(vtkHyperTree* tree, vtkIdType index)
{
  assert("pre: not_tree" && tree);
  tree->SetGlobalIndexFromLocal(this->Index, index);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridGeometryEntry::SetMask(
  const vtkHyperTreeGrid* grid, const vtkHyperTree* tree, bool value)
{
  assert("pre: not_tree" && tree);
  const_cast<vtkHyperTreeGrid*>(grid)->GetMask()->InsertTuple1(
    this->GetGlobalNodeIndex(tree), value);
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridGeometryEntry::IsMasked(
  const vtkHyperTreeGrid* grid, const vtkHyperTree* tree) const
{
  assert("pre: not_tree" && tree);
  if (tree && const_cast<vtkHyperTreeGrid*>(grid)->HasMask())
  {
    return const_cast<vtkHyperTreeGrid*>(grid)->GetMask()->GetValue(
             this->GetGlobalNodeIndex(tree)) != 0;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridGeometryEntry::IsLeaf(
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
void vtkHyperTreeGridGeometryEntry::SubdivideLeaf(
  const vtkHyperTreeGrid* grid, vtkHyperTree* tree, unsigned int level)
{
  assert("pre: not_tree" && tree);
  assert("pre: depth_limiter" && level <= const_cast<vtkHyperTreeGrid*>(grid)->GetDepthLimiter());
  assert("pre: is_masked" && !this->IsMasked(grid, tree));
  if (this->IsLeaf(grid, tree, level))
  {
    tree->SubdivideLeaf(this->Index, level);
  }
}

//---------------------------------------------------------------------------
bool vtkHyperTreeGridGeometryEntry::IsTerminalNode(
  const vtkHyperTreeGrid* grid, const vtkHyperTree* tree, unsigned int level) const
{
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
void vtkHyperTreeGridGeometryEntry::ToChild(const vtkHyperTreeGrid* grid, const vtkHyperTree* tree,
  unsigned int level, const double* sizeChild, unsigned char ichild)
{
  (void)level;
  assert("pre: not_tree" && tree);
  assert("pre: not_leaf" && !this->IsLeaf(grid, tree, level));
  assert("pre: not_valid_child" && ichild < tree->GetNumberOfChildren());
  assert("pre: depth_limiter" && level <= const_cast<vtkHyperTreeGrid*>(grid)->GetDepthLimiter());
  assert("pre: is_masked" && !IsMasked(grid, tree));

  this->Index = tree->GetElderChildIndex(this->Index) + ichild;

  // Divide cell size and translate origin per template parameter
  switch (tree->GetNumberOfChildren())
  {
    case 2: // dimension = 1, branch factor = 2
    {
      unsigned int axis = grid->GetOrientation();
      this->Origin[axis] += (ichild & 1) * sizeChild[axis];
      break;
    }
    case 3: // dimension = 1, branch factor = 3
    {
      unsigned int axis = grid->GetOrientation();
      this->Origin[axis] += (ichild % 3) * sizeChild[axis];
      break;
    }
    case 4: // dimension = 2, branch factor = 2
    {
      unsigned int axis1 = 0;
      unsigned int axis2 = 1;
      switch (grid->GetOrientation())
      {
        case 0:
          axis1 = 1;
          VTK_FALLTHROUGH;
        case 1:
          axis2 = 2;
      }
      this->Origin[axis1] += (ichild & 1) * sizeChild[axis1];
      this->Origin[axis2] += ((ichild & 2) >> 1) * sizeChild[axis2];
      break;
    }
    case 9: // dimension = 2, branch factor = 3
    {
      unsigned int axis1 = 0;
      unsigned int axis2 = 1;
      switch (grid->GetOrientation())
      {
        case 0:
          axis1 = 1;
          VTK_FALLTHROUGH;
        case 1:
          axis2 = 2;
      }
      this->Origin[axis1] += (ichild % 3) * sizeChild[axis1];
      this->Origin[axis2] += ((ichild % 9) / 3) * sizeChild[axis2];
      break;
    }
    case 8: // dimension = 3, branch factor = 2
    {
      this->Origin[0] += (ichild & 1) * sizeChild[0];
      this->Origin[1] += ((ichild & 2) >> 1) * sizeChild[1];
      this->Origin[2] += ((ichild & 4) >> 2) * sizeChild[2];
      break;
    }
    case 27: // dimension = 3, branch factor = 3
    {
      this->Origin[0] += (ichild % 3) * sizeChild[0];
      this->Origin[1] += ((ichild % 9) / 3) * sizeChild[1];
      this->Origin[2] += (ichild / 9) * sizeChild[2];
      break;
    }
  }
}
