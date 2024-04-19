// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridGeometryUnlimitedEntry.h"

#include "vtkBitArray.h"

#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"

#include <cassert>
#include <limits>

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkHyperTreeGridGeometryUnlimitedEntry::vtkHyperTreeGridGeometryUnlimitedEntry()
{
  for (unsigned int d = 0; d < 3; ++d)
  {
    this->Origin[d] = 0.;
  }
}

//------------------------------------------------------------------------------
vtkHyperTreeGridGeometryUnlimitedEntry::vtkHyperTreeGridGeometryUnlimitedEntry(
  vtkIdType index, const double* origin)
{
  this->Index = index;
  if (index != static_cast<vtkIdType>(std::numeric_limits<unsigned int>::max()))
  {
    this->LastRealIndex = index;
  }
  else
  {
    vtkWarningWithObjectMacro(
      nullptr, "Attempt to construct a geometry entry from an invalid index.");
    this->LastRealIndex = vtkHyperTreeGrid::InvalidIndex;
  }

  for (unsigned int d = 0; d < 3; ++d)
  {
    this->Origin[d] = origin[d];
  }
}
//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometryUnlimitedEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "--vtkHyperTreeGridGeometryLevelEntry--" << endl;
  os << indent << "Index:" << this->Index << endl;
  os << indent << "LastRealIndex:" << this->LastRealIndex << endl;
  os << indent << "Origin:" << this->Origin[0] << ", " << this->Origin[1] << ", " << this->Origin[2]
     << endl;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometryUnlimitedEntry::Dump(ostream& os)
{
  os << "Index:" << this->Index << endl;
  os << "LastRealIndex:" << this->LastRealIndex << endl;
  os << "Origin:" << this->Origin[0] << ", " << this->Origin[1] << ", " << this->Origin[2] << endl;
}

//------------------------------------------------------------------------------
vtkHyperTree* vtkHyperTreeGridGeometryUnlimitedEntry::Initialize(
  vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create)
{
  this->Index = 0;
  grid->GetLevelZeroOriginFromIndex(treeIndex, this->Origin);
  return grid->GetTree(treeIndex, create);
}

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometryUnlimitedEntry::GetGlobalNodeIndex(const vtkHyperTree* tree) const
{
  assert("pre: not_tree" && tree);
  return tree->GetGlobalIndexFromLocal(this->LastRealIndex);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometryUnlimitedEntry::SetGlobalIndexStart(
  vtkHyperTree* tree, vtkIdType index)
{
  assert("pre: not_tree" && tree);
  tree->SetGlobalIndexStart(index);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometryUnlimitedEntry::SetGlobalIndexFromLocal(
  vtkHyperTree* tree, vtkIdType index)
{
  assert("pre: not_tree" && tree);
  tree->SetGlobalIndexFromLocal(this->Index, index);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometryUnlimitedEntry::SetMask(
  const vtkHyperTreeGrid* grid, const vtkHyperTree* tree, bool value)
{
  assert("pre: not_tree" && tree);
  const_cast<vtkHyperTreeGrid*>(grid)->GetMask()->InsertTuple1(
    this->GetGlobalNodeIndex(tree), value);
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometryUnlimitedEntry::IsMasked(
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

//------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometryUnlimitedEntry::IsLeaf(
  const vtkHyperTreeGrid* grid, const vtkHyperTree* vtkNotUsed(tree), unsigned int level) const
{
  if (level >= const_cast<vtkHyperTreeGrid*>(grid)->GetDepthLimiter())
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometryUnlimitedEntry::IsRealLeaf(const vtkHyperTree* tree) const
{
  assert("pre: not_tree" && tree);
  assert("pre: not_virtual" && !this->IsVirtualLeaf(tree));
  return tree->IsLeaf(this->Index);
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometryUnlimitedEntry::IsVirtualLeaf(
  const vtkHyperTree* vtkNotUsed(tree)) const
{
  return this->LastRealIndex != this->Index;
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometryUnlimitedEntry::IsTerminalNode(
  const vtkHyperTreeGrid* grid, const vtkHyperTree* vtkNotUsed(tree), unsigned int level) const
{
  if (level + 1 == const_cast<vtkHyperTreeGrid*>(grid)->GetDepthLimiter())
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometryUnlimitedEntry::ToChild(const vtkHyperTreeGrid* grid,
  const vtkHyperTree* tree, unsigned int vtkNotUsed(level), const double* sizeChild,
  unsigned char ichild)
{
  assert("pre: not_tree" && tree);
  assert("pre: is_masked" && !IsMasked(grid, tree));

  size_t indexMax = 0;
  tree->GetElderChildIndexArray(indexMax);
  if (this->Index >= 0 && this->Index < static_cast<vtkIdType>(indexMax))
  {
    vtkIdType elder = tree->GetElderChildIndex(this->Index);
    if (elder != static_cast<vtkIdType>(std::numeric_limits<unsigned int>::max()))
    {
      this->Index = elder + ichild;
      this->LastRealIndex = this->Index;
    }
    else
    {
      // first virtual cell
      this->Index = vtkHyperTreeGrid::InvalidIndex;
    }
  }
  else
  {
    // cell is already virtual
    this->Index = vtkHyperTreeGrid::InvalidIndex;
  }

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
VTK_ABI_NAMESPACE_END
