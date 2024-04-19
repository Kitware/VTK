// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridNonOrientedMooreSuperCursorLight.h"

#include "vtkBitArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridLevelEntry.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"

#include "vtkIdList.h"

#include "vtkObjectFactory.h"

#include <array>
#include <cassert>

#include "vtkHyperTreeGridNonOrientedMooreSuperCursorData.inl"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridNonOrientedMooreSuperCursorLight);

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedMooreSuperCursorLight::Initialize(
  vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create)
{
  assert("pre: Non_valid_dimension" && grid->GetDimension() >= 1 && grid->GetDimension() <= 3);
  assert(
    "pre: Non_valid_branchfactor" && grid->GetBranchFactor() >= 2 && grid->GetBranchFactor() <= 3);

  if (this->Grid == nullptr)
  {
    this->Grid = grid;
  }
  assert("pre: Non_same_grid" && this->Grid == grid);

  // Initialize features
  switch (grid->GetNumberOfChildren())
  {
    case 2:
    {
      // dimension = 1, branch factor = 2
      this->IndiceCentralCursor = 1;
      this->NumberOfCursors = 3;
      this->ChildCursorToParentCursorTable = MooreChildCursorToParentCursorTable[0][0];
      this->ChildCursorToChildTable = MooreChildCursorToChildTable[0][0];
      break;
    }
    case 3:
    {
      // dimension = 1, branch factor = 3
      this->IndiceCentralCursor = 1;
      this->NumberOfCursors = 3;
      this->ChildCursorToParentCursorTable = MooreChildCursorToParentCursorTable[0][1];
      this->ChildCursorToChildTable = MooreChildCursorToChildTable[0][1];
      break;
    }
    case 4:
    {
      // dimension = 2, branch factor = 2
      this->IndiceCentralCursor = 4;
      this->NumberOfCursors = 9;
      this->ChildCursorToParentCursorTable = MooreChildCursorToParentCursorTable[1][0];
      this->ChildCursorToChildTable = MooreChildCursorToChildTable[1][0];
      break;
    }
    case 9:
    {
      // dimension = 2, branch factor = 3
      this->IndiceCentralCursor = 4;
      this->NumberOfCursors = 9;
      this->ChildCursorToParentCursorTable = MooreChildCursorToParentCursorTable[1][1];
      this->ChildCursorToChildTable = MooreChildCursorToChildTable[1][1];
      break;
    }
    case 8:
    {
      // dimension = 3, branch factor = 2
      this->IndiceCentralCursor = 13;
      this->NumberOfCursors = 27;
      this->ChildCursorToParentCursorTable = MooreChildCursorToParentCursorTable[2][0];
      this->ChildCursorToChildTable = MooreChildCursorToChildTable[2][0];
      break;
    }
    case 27:
    {
      // dimension = 3, branch factor = 3
      this->IndiceCentralCursor = 13;
      this->NumberOfCursors = 27;
      this->ChildCursorToParentCursorTable = MooreChildCursorToParentCursorTable[2][1];
      this->ChildCursorToChildTable = MooreChildCursorToChildTable[2][1];
      break;
    }
  } // switch Dimension

  this->CentralCursor->Initialize(grid, treeIndex, create);
  //
  this->CurrentFirstNonValidEntryByLevel = 0;
  if (this->FirstNonValidEntryByLevel.size() <= this->CurrentFirstNonValidEntryByLevel + 1)
  {
    this->FirstNonValidEntryByLevel.resize(this->CurrentFirstNonValidEntryByLevel + 1);
  }
  this->FirstNonValidEntryByLevel[this->CurrentFirstNonValidEntryByLevel] =
    this->NumberOfCursors - 1;
  bool isOld = true;
  if (this->Entries.size() <= this->CurrentFirstNonValidEntryByLevel + 1)
  {
    isOld = false;
    this->Entries.resize(this->FirstNonValidEntryByLevel[this->CurrentFirstNonValidEntryByLevel]);
  }
  this->FirstCurrentNeighboorReferenceEntry = 0;
  if (this->ReferenceEntries.size() <=
    this->FirstCurrentNeighboorReferenceEntry + this->NumberOfCursors - 1)
  {
    this->ReferenceEntries.resize(
      this->FirstCurrentNeighboorReferenceEntry + this->NumberOfCursors - 1);
  }
  std::vector<unsigned int>::iterator it = this->ReferenceEntries.begin();
  for (unsigned int icrt = this->FirstCurrentNeighboorReferenceEntry;
       it != this->ReferenceEntries.end(); ++it, ++icrt)
  {
    (*it) = icrt;
  }

  // If dimension=d: center cursor is d
  //                 d-faces neighbor cursors are 0,...,2d except d
  unsigned int i, j, k;
  grid->GetLevelZeroCoordinatesFromIndex(treeIndex, i, j, k);
  unsigned int n[3];
  grid->GetCellDims(n);

  // Cursor initialization
  switch (grid->GetDimension())
  {
    case 1:
    {
      const std::array<unsigned int, 3> ijk{ i, j, k };
      // dimension == 1
      const bool toW = (ijk[grid->GetAxes()[0]] > 0);
      const bool toE = (ijk[grid->GetAxes()[0]] + 1 < n[grid->GetAxes()[0]]);
      if (toW)
      {
        // Cell has a neighbor to the left
        const vtkIdType shifted_lvl_zero_id = grid->GetShiftedLevelZeroIndex(treeIndex, -1, 0, 0);
        this->Entries[0].Initialize(grid, shifted_lvl_zero_id);
      }
      else if (isOld)
      {
        this->Entries[0].Reset();
      }
      if (toE)
      {
        // Cell has a neighbor to the right
        const vtkIdType shifted_lvl_zero_id = grid->GetShiftedLevelZeroIndex(treeIndex, 1, 0, 0);
        this->Entries[1].Initialize(grid, shifted_lvl_zero_id);
      }
      else if (isOld)
      {
        this->Entries[1].Reset();
      }
      break;
    }
    case 2:
    {
      const std::array<unsigned int, 3> ijk{ i, j, k };
      // dimension == 2 with context axes
      const bool toW = (ijk[grid->GetAxes()[0]] > 0);
      const bool toS = (ijk[grid->GetAxes()[1]] > 0);
      const bool toE = (ijk[grid->GetAxes()[0]] + 1 < n[grid->GetAxes()[0]]);
      const bool toN = (ijk[grid->GetAxes()[1]] + 1 < n[grid->GetAxes()[1]]);
      if (toS)
      {
        // Cell has a neighbor to the south
        const vtkIdType shifted_lvl_zero_id = grid->GetShiftedLevelZeroIndex(treeIndex, 0, -1, 0);
        this->Entries[1].Initialize(grid, shifted_lvl_zero_id);
        if (toW)
        {
          // Cell has a neighbor to the southwest
          const vtkIdType shifted_lvl_zero_idSW =
            grid->GetShiftedLevelZeroIndex(treeIndex, -1, -1, 0);
          this->Entries[0].Initialize(grid, shifted_lvl_zero_idSW);
        }
        else
        { // if ( toW )
          this->Entries[0].Reset();
        }
        if (toE)
        {
          // Cell has a neighbor to the southeast
          const vtkIdType shifted_lvl_zero_idSE =
            grid->GetShiftedLevelZeroIndex(treeIndex, 1, -1, 0);
          this->Entries[2].Initialize(grid, shifted_lvl_zero_idSE);
        }
        else
        { // if ( toE )
          this->Entries[2].Reset();
        }
      }
      else
      { // if ( toS )
        this->Entries[0].Reset();
        this->Entries[1].Reset();
        this->Entries[2].Reset();
      }

      if (toW)
      {
        // Cell has a neighbor to the west
        const vtkIdType shifted_lvl_zero_id = grid->GetShiftedLevelZeroIndex(treeIndex, -1, 0, 0);
        this->Entries[3].Initialize(grid, shifted_lvl_zero_id);
      }
      else
      { // if ( toW )
        this->Entries[3].Reset();
      }
      if (toE)
      {
        // Cell has a neighbor to the east
        const vtkIdType shifted_lvl_zero_id = grid->GetShiftedLevelZeroIndex(treeIndex, 1, 0, 0);
        this->Entries[4].Initialize(grid, shifted_lvl_zero_id);
      }
      else
      { // if ( toE )
        this->Entries[4].Reset();
      }
      if (toN)
      {
        // Cell has a neighbor to the north
        const vtkIdType shifted_lvl_zero_id = grid->GetShiftedLevelZeroIndex(treeIndex, 0, 1, 0);
        this->Entries[6].Initialize(grid, shifted_lvl_zero_id);
        if (toW)
        {
          // Cell has a neighbor to the northwest
          const vtkIdType shifted_lvl_zero_idNW =
            grid->GetShiftedLevelZeroIndex(treeIndex, -1, 1, 0);
          this->Entries[5].Initialize(grid, shifted_lvl_zero_idNW);
        }
        else
        { // if ( toW )
          this->Entries[5].Reset();
        }
        if (toE)
        {
          // Cell has a neighbor to the northeast
          const vtkIdType shifted_lvl_zero_idNE =
            grid->GetShiftedLevelZeroIndex(treeIndex, 1, 1, 0);
          this->Entries[7].Initialize(grid, shifted_lvl_zero_idNE);
        }
        else
        { // if ( toW )
          this->Entries[7].Reset();
        }
      }
      else
      { // if ( toN )
        this->Entries[5].Reset();
        this->Entries[6].Reset();
        this->Entries[7].Reset();
      }
      break;
    }
    case 3:
    {
      // Initialize all connectivity cursors
      for (unsigned int _c = 0; _c < this->NumberOfCursors - 1; ++_c)
      {
        this->Entries[_c].Reset();
      } // _c

      // dimension == 3
      const int minI = (i == 0) ? 0 : -1;
      const int maxI = (i + 1 < n[0]) ? 2 : 1;
      const int minJ = (j == 0) ? 0 : -1;
      const int maxJ = (j + 1 < n[1]) ? 2 : 1;
      const int minK = (k == 0) ? 0 : -1;
      const int maxK = (k + 1 < n[2]) ? 2 : 1;

      // Initialize all connectivity cursors
      for (int _k = minK; _k < maxK; ++_k)
      {
        for (int _j = minJ; _j < maxJ; ++_j)
        {
          for (int _i = minI; _i < maxI; ++_i)
          {
            unsigned int c = 13 + _i + 3 * _j + 9 * _k;
            if (c != this->IndiceCentralCursor)
            {
              const vtkIdType shifted_lvl_zero_id =
                grid->GetShiftedLevelZeroIndex(treeIndex, _i, _j, _k);
              if (c < this->IndiceCentralCursor)
              {
                this->Entries[c].Initialize(grid, shifted_lvl_zero_id);
              }
              else
              {
                this->Entries[c - 1].Initialize(grid, shifted_lvl_zero_id);
              }
            }
          } // _i
        }   // _j
      }     // _k
      break;
    }
  } // switch Dimension
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedMooreSuperCursorLight::GetCornerCursors(
  unsigned int c, unsigned int l, vtkIdList* leaves)
{
  unsigned int cursorIdx = 0;
  switch (this->GetDimension())
  {
    case 1:
      // dimension == 1
      cursorIdx = CornerNeighborCursorsTable1D[c][l];
      break;
    case 2:
      // dimension == 2
      cursorIdx = CornerNeighborCursorsTable2D[c][l];
      break;
    case 3:
      // dimension == 3
      cursorIdx = CornerNeighborCursorsTable3D[c][l];
      break;
    default:
      vtkErrorMacro("unexpected neighborhood");
      return false;
  } // switch ( N )

  // Collect the cursor index for this leaf
  leaves->SetId(l, cursorIdx);

  // Determine ownership of corner
  bool owner = true;
  if (cursorIdx != this->IndiceCentralCursor)
  {
    vtkHyperTreeGridLevelEntry& cursor = this->Entries[this->GetIndiceEntry(cursorIdx)];
    if (!cursor.GetTree() || !cursor.IsLeaf(this->Grid))
    {
      // If neighbor cell is out of bounds or has Non been
      // refined to a leaf, that leaf does Non own the corner
      owner = false;
    }
    else if (cursor.IsMasked(this->Grid))
    {
      // If neighbor cell is masked, that leaf does Non own the corner
      owner = false;
    }
    else if (this->IndiceCentralCursor < cursorIdx && cursor.GetLevel() == this->GetLevel())
    {
      // A level tie is broken in favor of the largest index
      owner = false;
    }
  } // if( cursorIdx! = this->IndiceCentralCursor )

  // Return ownership of corner by this node
  return owner;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedMooreSuperCursorLight::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "--vtkHyperTreeGridNonOrientedMooreSuperCursorLight--" << endl;
  vtkHyperTreeGridNonOrientedSuperCursorLight::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedMooreSuperCursorLight::
  ~vtkHyperTreeGridNonOrientedMooreSuperCursorLight() = default;

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_END
