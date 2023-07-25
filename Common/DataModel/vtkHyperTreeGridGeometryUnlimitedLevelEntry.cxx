// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridGeometryUnlimitedLevelEntry.h"

#include "vtkBitArray.h"

#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridScales.h"

#include <cassert>
#include <limits>

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometryUnlimitedLevelEntry::Initialize(
  vtkHyperTree* tree, unsigned int level, vtkIdType index, const double* origin)
{
  this->Tree = tree;
  this->Level = level;
  this->Index = index;
  for (unsigned int d = 0; d < 3; ++d)
  {
    this->Origin[d] = origin[d];
  }
}

//------------------------------------------------------------------------------
vtkHyperTree* vtkHyperTreeGridGeometryUnlimitedLevelEntry::Initialize(
  vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create)
{
  this->Tree = grid->GetTree(treeIndex, create);
  grid->GetLevelZeroOriginFromIndex(treeIndex, this->Origin);
  return this->Tree;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometryUnlimitedLevelEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "--vtkHyperTreeGridGeometryUnlimitedLevelEntry--" << endl;
  this->Tree->PrintSelf(os, indent);
  os << indent << "Level:" << this->Level << endl;
  os << indent << "Index:" << this->Index << endl;
  os << indent << "LastRealIndex:" << this->LastRealIndex << endl;
  os << indent << "LastRealLevel:" << this->LastRealLevel << endl;
  os << indent << "Origin:" << this->Origin[0] << ", " << this->Origin[1] << ", " << this->Origin[2]
     << endl;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometryUnlimitedLevelEntry::Dump(ostream& os)
{
  os << "Level:" << this->Level << endl;
  os << "Index:" << this->Index << endl;
  os << "LastRealIndex:" << this->LastRealIndex << endl;
  os << "LastRealLevel:" << this->LastRealLevel << endl;
  os << "Origin:" << this->Origin[0] << ", " << this->Origin[1] << ", " << this->Origin[2] << endl;
}

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometryUnlimitedLevelEntry::GetGlobalNodeIndex() const
{
  return this->Tree ? this->Tree->GetGlobalIndexFromLocal(this->LastRealIndex)
                    : vtkHyperTreeGrid::InvalidIndex;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometryUnlimitedLevelEntry::SetGlobalIndexStart(vtkIdType index)
{
  assert("pre: not_tree" && this->Tree);
  this->Tree->SetGlobalIndexStart(index);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometryUnlimitedLevelEntry::SetGlobalIndexFromLocal(vtkIdType index)
{
  assert("pre: not_tree" && this->Tree);
  this->Tree->SetGlobalIndexFromLocal(this->Index, index);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometryUnlimitedLevelEntry::SetMask(const vtkHyperTreeGrid* grid, bool value)
{
  assert("pre: not_tree" && this->Tree);
  // JB Comment faire pour definir un accesseur a DepthLimiter qui est const
  const_cast<vtkHyperTreeGrid*>(grid)->GetMask()->InsertTuple1(this->GetGlobalNodeIndex(), value);
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometryUnlimitedLevelEntry::IsMasked(const vtkHyperTreeGrid* grid) const
{
  if (this->Tree && const_cast<vtkHyperTreeGrid*>(grid)->HasMask())
  {
    return const_cast<vtkHyperTreeGrid*>(grid)->GetMask()->GetValue(this->GetGlobalNodeIndex()) !=
      0;
  }
  // JB Comment faire pour definir un accesseur a DepthLimiter qui est const
  return false;
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometryUnlimitedLevelEntry::IsLeaf(const vtkHyperTreeGrid* grid) const
{
  if (this->Level >= const_cast<vtkHyperTreeGrid*>(grid)->GetDepthLimiter())
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometryUnlimitedLevelEntry::IsRealLeaf(const vtkHyperTreeGrid* grid) const
{
  if (this->Level == const_cast<vtkHyperTreeGrid*>(grid)->GetDepthLimiter())
  {
    return true;
  }
  return this->Index == this->LastRealIndex && this->Tree->IsLeaf(this->Index);
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometryUnlimitedLevelEntry::IsVirtualLeaf(const vtkHyperTreeGrid* grid) const
{
  if (this->Level > const_cast<vtkHyperTreeGrid*>(grid)->GetDepthLimiter())
  {
    return true;
  }
  return this->LastRealIndex != this->Index;
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometryUnlimitedLevelEntry::IsTerminalNode(const vtkHyperTreeGrid* grid) const
{
  if (this->Level + 1 == const_cast<vtkHyperTreeGrid*>(grid)->GetDepthLimiter())
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometryUnlimitedLevelEntry::ToChild(
  const vtkHyperTreeGrid* grid, unsigned char ichild)
{
  assert("pre: not_tree" && this->Tree);
  assert(
    "pre: depth_limiter" && this->Level <= const_cast<vtkHyperTreeGrid*>(grid)->GetDepthLimiter());
  assert("pre: is_masked" && !this->IsMasked(grid));

  const double* sizeChild = this->Tree->GetScales()->GetScale(this->Level + 1);

  size_t indexMax = 0;
  this->Tree->GetElderChildIndexArray(indexMax);
  if (this->Index >= 0 && this->Index < static_cast<vtkIdType>(indexMax))
  {
    const vtkIdType elder = this->Tree->GetElderChildIndex(this->Index);
    if (elder != static_cast<vtkIdType>(std::numeric_limits<unsigned int>::max()))
    {
      this->Index = elder + ichild;
      this->LastRealIndex = this->Index;
      this->LastRealLevel = this->Level + 1;
    }
    else
    {
      // first virtual cell
      this->Index = vtkHyperTreeGrid::InvalidIndex;
    }
  }
  else
  {
    // cell already virtual
    this->Index = vtkHyperTreeGrid::InvalidIndex;
  }

  // Divide cell size and translate origin per template parameter
  switch (this->Tree->GetNumberOfChildren())
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

  this->Level++;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometryUnlimitedLevelEntry::GetBounds(double bounds[6]) const
{
  assert("pre: not_tree" && this->Tree);
  const double* sizeChild = this->Tree->GetScales()->GetScale(this->Level);
  // Compute bounds
  bounds[0] = this->Origin[0];
  bounds[1] = this->Origin[0] + sizeChild[0];
  bounds[2] = this->Origin[1];
  bounds[3] = this->Origin[1] + sizeChild[1];
  bounds[4] = this->Origin[2];
  bounds[5] = this->Origin[2] + sizeChild[2];
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometryUnlimitedLevelEntry::GetPoint(double point[3]) const
{
  assert("pre: not_tree" && this->Tree);
  const double* sizeChild = this->Tree->GetScales()->GetScale(this->Level);
  // Compute center point coordinates
  point[0] = this->Origin[0] + sizeChild[0] / 2.;
  point[1] = this->Origin[1] + sizeChild[1] / 2.;
  point[2] = this->Origin[2] + sizeChild[2] / 2.;
}
VTK_ABI_NAMESPACE_END
