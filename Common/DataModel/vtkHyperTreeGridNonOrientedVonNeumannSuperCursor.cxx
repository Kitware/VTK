// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridNonOrientedVonNeumannSuperCursor.h"

#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometryLevelEntry.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"

#include "vtkObjectFactory.h"

#include <array>
#include <cassert>

#include "vtkHyperTreeGridNonOrientedVonNeumannSuperCursorData.inl"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridNonOrientedVonNeumannSuperCursor);

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedVonNeumannSuperCursor::Initialize(
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
  switch (grid->GetDimension())
  {
    case 1:
    {
      switch (grid->GetBranchFactor())
      {
        case 2:
        {
          // dimension = 1, branch factor = 2
          this->IndiceCentralCursor = 1;
          this->NumberOfCursors = 3;
          this->ChildCursorToParentCursorTable = VonNeumannChildCursorToParentCursorTable[0][0];
          this->ChildCursorToChildTable = VonNeumannChildCursorToChildTable[0][0];
          break;
        }
        case 3:
        {
          // dimension = 1, branch factor = 3
          this->IndiceCentralCursor = 1;
          this->NumberOfCursors = 3;
          this->ChildCursorToParentCursorTable = VonNeumannChildCursorToParentCursorTable[0][1];
          this->ChildCursorToChildTable = VonNeumannChildCursorToChildTable[0][1];
          break;
        }
      } // switch BranchFactor
      break;
    }
    case 2:
    {
      switch (grid->GetBranchFactor())
      {
        case 2:
        {
          // dimension = 2, branch factor = 2
          this->IndiceCentralCursor = 2;
          this->NumberOfCursors = 5;
          this->ChildCursorToParentCursorTable = VonNeumannChildCursorToParentCursorTable[1][0];
          this->ChildCursorToChildTable = VonNeumannChildCursorToChildTable[1][0];
          break;
        }
        case 3:
        {
          // dimension = 2, branch factor = 3
          this->IndiceCentralCursor = 2;
          this->NumberOfCursors = 5;
          this->ChildCursorToParentCursorTable = VonNeumannChildCursorToParentCursorTable[1][1];
          this->ChildCursorToChildTable = VonNeumannChildCursorToChildTable[1][1];
          break;
        }
      } // switch BranchFactor
      break;
    }
    case 3:
    {
      switch (grid->GetBranchFactor())
      {
        case 2:
        {
          // dimension = 3, branch factor = 2
          this->IndiceCentralCursor = 3;
          this->NumberOfCursors = 7;
          this->ChildCursorToParentCursorTable = VonNeumannChildCursorToParentCursorTable[2][0];
          this->ChildCursorToChildTable = VonNeumannChildCursorToChildTable[2][0];
          break;
        }
        case 3:
        {
          // dimension = 3, branch factor = 3
          this->IndiceCentralCursor = 3;
          this->NumberOfCursors = 7;
          this->ChildCursorToParentCursorTable = VonNeumannChildCursorToParentCursorTable[2][1];
          this->ChildCursorToChildTable = VonNeumannChildCursorToChildTable[2][1];
          break;
        }
      } // switch BranchFactor
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

      if (toW)
      {
        // Cell has a neighbor to the left
        const vtkIdType shifted_lvl_zero_id = grid->GetShiftedLevelZeroIndex(treeIndex, -1, 0, 0);
        this->Entries[1].Initialize(grid, shifted_lvl_zero_id);
      }
      else if (isOld)
      {
        this->Entries[1].Reset();
      }

      if (toE)
      {
        // Cell has a neighbor to the right
        const vtkIdType shifted_lvl_zero_id = grid->GetShiftedLevelZeroIndex(treeIndex, 1, 0, 0);
        this->Entries[2].Initialize(grid, shifted_lvl_zero_id);
      }
      else if (isOld)
      {
        this->Entries[2].Reset();
      }

      if (toS)
      {
        // Cell has a neighbor before
        const vtkIdType shifted_lvl_zero_id = grid->GetShiftedLevelZeroIndex(treeIndex, 0, -1, 0);
        this->Entries[0].Initialize(grid, shifted_lvl_zero_id);
      }
      else if (isOld)
      {
        this->Entries[0].Reset();
      }

      if (toN)
      {
        // Cell has a neighbor after
        const vtkIdType shifted_lvl_zero_id = grid->GetShiftedLevelZeroIndex(treeIndex, 0, 1, 0);
        this->Entries[3].Initialize(grid, shifted_lvl_zero_id);
      }
      else if (isOld)
      {
        this->Entries[3].Reset();
      }
      break;
    }
    case 3:
    {
      // dimension == 3
      if (i > 0)
      {
        // Cell has a neighbor to the left
        const vtkIdType shifted_lvl_zero_id = grid->GetShiftedLevelZeroIndex(treeIndex, -1, 0, 0);
        this->Entries[2].Initialize(grid, shifted_lvl_zero_id);
      }
      else if (isOld)
      {
        this->Entries[2].Reset();
      }
      if (i + 1 < n[0])
      {
        // Cell has a neighbor to the right
        const vtkIdType shifted_lvl_zero_id = grid->GetShiftedLevelZeroIndex(treeIndex, 1, 0, 0);
        this->Entries[3].Initialize(grid, shifted_lvl_zero_id);
      }
      else if (isOld)
      {
        this->Entries[3].Reset();
      }
      if (j > 0)
      {
        // Cell has a neighbor before
        const vtkIdType shifted_lvl_zero_id = grid->GetShiftedLevelZeroIndex(treeIndex, 0, -1, 0);
        this->Entries[1].Initialize(grid, shifted_lvl_zero_id);
      }
      else if (isOld)
      {
        this->Entries[1].Reset();
      }
      if (j + 1 < n[1])
      {
        // Cell has a neighbor after
        const vtkIdType shifted_lvl_zero_id = grid->GetShiftedLevelZeroIndex(treeIndex, 0, 1, 0);
        this->Entries[4].Initialize(grid, shifted_lvl_zero_id);
      }
      else if (isOld)
      {
        this->Entries[4].Reset();
      }
      if (k > 0)
      {
        // Cell has a neighbor below
        const vtkIdType shifted_lvl_zero_id = grid->GetShiftedLevelZeroIndex(treeIndex, 0, 0, -1);
        this->Entries[0].Initialize(grid, shifted_lvl_zero_id);
      }
      else if (isOld)
      {
        this->Entries[0].Reset();
      }
      if (k + 1 < n[2])
      {
        // Cell has a neighbor above
        const vtkIdType shifted_lvl_zero_id = grid->GetShiftedLevelZeroIndex(treeIndex, 0, 0, 1);
        this->Entries[5].Initialize(grid, shifted_lvl_zero_id);
      }
      else if (isOld)
      {
        this->Entries[5].Reset();
      }
      break;
    }
  } // switch Dimension
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedVonNeumannSuperCursor::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "--vtkHyperTreeGridNonOrientedVonNeumannSuperCursor--" << endl;
  vtkHyperTreeGridNonOrientedSuperCursor::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedVonNeumannSuperCursor::
  ~vtkHyperTreeGridNonOrientedVonNeumannSuperCursor() = default;

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_END
