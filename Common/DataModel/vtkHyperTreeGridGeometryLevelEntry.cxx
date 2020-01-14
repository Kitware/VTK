/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridGeometryLevelEntry.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridGeometryLevelEntry.h"

#include "vtkBitArray.h"

#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridScales.h"

#include <cassert>

//-----------------------------------------------------------------------------
void vtkHyperTreeGridGeometryLevelEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "--vtkHyperTreeGridGeometryLevelEntry--" << endl;
  this->Tree->PrintSelf(os, indent);
  os << indent << "Level:" << this->Level << endl;
  os << indent << "Index:" << this->Index << endl;
  os << indent << "Origin:" << this->Origin[0] << ", " << this->Origin[1] << ", " << this->Origin[2]
     << endl;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridGeometryLevelEntry::Dump(ostream& os)
{
  os << "Level:" << this->Level << endl;
  os << "Index:" << this->Index << endl;
  os << "Origin:" << this->Origin[0] << ", " << this->Origin[1] << ", " << this->Origin[2] << endl;
}

//-----------------------------------------------------------------------------
vtkHyperTree* vtkHyperTreeGridGeometryLevelEntry::Initialize(
  vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create)
{
  this->Tree = grid->GetTree(treeIndex, create);
  this->Level = 0;
  this->Index = 0;
  grid->GetLevelZeroOriginFromIndex(treeIndex, this->Origin);
  return this->Tree;
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometryLevelEntry::GetGlobalNodeIndex() const
{
  return this->Tree ? this->Tree->GetGlobalIndexFromLocal(this->Index)
                    : vtkHyperTreeGrid::InvalidIndex;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridGeometryLevelEntry::SetGlobalIndexStart(vtkIdType index)
{
  assert("pre: not_tree" && this->Tree);
  this->Tree->SetGlobalIndexStart(index);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridGeometryLevelEntry::SetGlobalIndexFromLocal(vtkIdType index)
{
  assert("pre: not_tree" && this->Tree);
  this->Tree->SetGlobalIndexFromLocal(this->Index, index);
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridGeometryLevelEntry::SetMask(const vtkHyperTreeGrid* grid, bool value)
{
  assert("pre: not_tree" && this->Tree);
  // JB Comment faire pour definir un accesseur a DepthLimiter qui est const
  const_cast<vtkHyperTreeGrid*>(grid)->GetMask()->InsertTuple1(this->GetGlobalNodeIndex(), value);
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridGeometryLevelEntry::IsMasked(const vtkHyperTreeGrid* grid) const
{
  if (this->Tree && const_cast<vtkHyperTreeGrid*>(grid)->HasMask())
  {
    return const_cast<vtkHyperTreeGrid*>(grid)->GetMask()->GetValue(this->GetGlobalNodeIndex()) !=
      0;
  }
  // JB Comment faire pour definir un accesseur a DepthLimiter qui est const
  return false;
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridGeometryLevelEntry::IsLeaf(const vtkHyperTreeGrid* grid) const
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
void vtkHyperTreeGridGeometryLevelEntry::SubdivideLeaf(const vtkHyperTreeGrid* grid)
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
bool vtkHyperTreeGridGeometryLevelEntry::IsTerminalNode(const vtkHyperTreeGrid* grid) const
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
void vtkHyperTreeGridGeometryLevelEntry::ToChild(const vtkHyperTreeGrid* grid, unsigned char ichild)
{
  assert("pre: not_tree" && this->Tree);
  assert("pre: not_leaf" && !this->IsLeaf(grid));
  assert("pre: not_valid_child" && ichild < this->Tree->GetNumberOfChildren());
  // JB Comment faire pour definir un accesseur a DepthLimiter qui est const
  assert(
    "pre: depth_limiter" && this->Level <= const_cast<vtkHyperTreeGrid*>(grid)->GetDepthLimiter());
  assert("pre: is_masked" && !this->IsMasked(grid));

  const double* sizeChild = this->Tree->GetScales()->GetScale(this->Level + 1);

  this->Index = this->Tree->GetElderChildIndex(this->Index) + ichild;

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

//-----------------------------------------------------------------------------
void vtkHyperTreeGridGeometryLevelEntry::GetBounds(double bounds[6]) const
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

//-----------------------------------------------------------------------------
void vtkHyperTreeGridGeometryLevelEntry::GetPoint(double point[3]) const
{
  assert("pre: not_tree" && this->Tree);
  const double* sizeChild = this->Tree->GetScales()->GetScale(this->Level);
  // Compute center point coordinates
  point[0] = this->Origin[0] + sizeChild[0] / 2.;
  point[1] = this->Origin[1] + sizeChild[1] / 2.;
  point[2] = this->Origin[2] + sizeChild[2] / 2.;
}
