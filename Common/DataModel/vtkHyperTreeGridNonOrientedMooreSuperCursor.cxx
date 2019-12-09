/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridNonOrientedMooreSuperCursor.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright Nonice for more information.

=========================================================================*/
#include "vtkHyperTreeGridNonOrientedMooreSuperCursor.h"

#include "vtkBitArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometryLevelEntry.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkIdList.h"

#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkHyperTreeGridNonOrientedMooreSuperCursor);

#include "vtkHyperTreeGridNonOrientedMooreSuperCursorData.cxx"

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedMooreSuperCursor::Initialize(
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

  // JB Initialize caracteristique
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

  // JB Pour le niveau zero tout est defini
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
  // JB Pour le niveau zero tout est reference
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

  // JB Initialisation des cursors
  switch (grid->GetDimension())
  {
    case 1:
    {
      // dimension == 1
      if (i > 0)
      {
        // Cell has a neighbor to the left
        unsigned int r = grid->GetShiftedLevelZeroIndex(treeIndex, (unsigned int)-1, 0, 0);
        this->Entries[0].Initialize(grid, r);
      }
      else if (isOld)
      {
        this->Entries[0].Reset();
      }
      if (i + 1 < n[0])
      {
        // Cell has a neighbor to the right
        unsigned int r = grid->GetShiftedLevelZeroIndex(treeIndex, 1, 0, 0);
        this->Entries[1].Initialize(grid, r); // au lieu de 2
      }
      else if (isOld)
      {
        this->Entries[1].Reset();
      }
      break;
    }
    case 2:
    {
      // dimension == 2
      bool toW = (i > 0);
      bool toS = (j > 0);
      bool toE = (i + 1 < n[0]);
      bool toN = (j + 1 < n[1]);
      if (toS)
      {
        // Cell has a neighbor to the south
        unsigned int r = grid->GetShiftedLevelZeroIndex(treeIndex, 0, (unsigned int)-1, 0);
        this->Entries[1].Initialize(grid, r);
        if (toW)
        {
          // Cell has a neighbor to the southwest
          r = grid->GetShiftedLevelZeroIndex(treeIndex, (unsigned int)-1, (unsigned int)-1, 0);
          this->Entries[0].Initialize(grid, r);
        }
        else
        { // if (toW)
          this->Entries[0].Reset();
        }
        if (toE)
        {
          // Cell has a neighbor to the southeast
          r = grid->GetShiftedLevelZeroIndex(treeIndex, 1, (unsigned int)-1, 0);
          this->Entries[2].Initialize(grid, r);
        }
        else
        { // if (toE)
          this->Entries[2].Reset();
        }
      }
      else
      { // if (toS)
        this->Entries[0].Reset();
        this->Entries[1].Reset();
        this->Entries[2].Reset();
      }

      if (toW)
      {
        // Cell has a neighbor to the west
        unsigned int r = grid->GetShiftedLevelZeroIndex(treeIndex, (unsigned int)-1, 0, 0);
        this->Entries[3].Initialize(grid, r);
      }
      else
      { // if (toW)
        this->Entries[3].Reset();
      }
      if (toE)
      {
        // Cell has a neighbor to the east
        unsigned int r = grid->GetShiftedLevelZeroIndex(treeIndex, 1, 0, 0);
        this->Entries[4].Initialize(grid, r); // au lieu de 5
      }
      else
      { // if (toE)
        this->Entries[4].Reset();
      }
      if (toN)
      {
        // Cell has a neighbor to the north
        unsigned int r = grid->GetShiftedLevelZeroIndex(treeIndex, 0, 1, 0);
        this->Entries[6].Initialize(grid, r); // au lieu de 7
        if (toW)
        {
          // Cell has a neighbor to the northwest
          r = grid->GetShiftedLevelZeroIndex(treeIndex, (unsigned int)-1, 1, 0);
          this->Entries[5].Initialize(grid, r); // au lieu de 6
        }
        else
        { // if (toW)
          this->Entries[5].Reset();
        }
        if (toE)
        {
          // Cell has a neighbor to the northeast
          r = grid->GetShiftedLevelZeroIndex(treeIndex, 1, 1, 0);
          this->Entries[7].Initialize(grid, r); // au lieu de 8
        }
        else
        { // if (toW)
          this->Entries[7].Reset();
        }
      }
      else
      { // if (toN)
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
      int minI = (i == 0) ? 0 : -1;
      int maxI = (i + 1 < n[0]) ? 2 : 1;
      int minJ = (j == 0) ? 0 : -1;
      int maxJ = (j + 1 < n[1]) ? 2 : 1;
      int minK = (k == 0) ? 0 : -1;
      int maxK = (k + 1 < n[2]) ? 2 : 1;

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
              unsigned int r = grid->GetShiftedLevelZeroIndex(treeIndex, _i, _j, _k);
              if (c < this->IndiceCentralCursor)
              {
                this->Entries[c].Initialize(grid, r);
              }
              else
              {
                this->Entries[c - 1].Initialize(grid, r);
              }
            }
          } // _i
        }   // _j
      }     // _k
      break;
    }
  } // switch Dimension
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedMooreSuperCursor::GetCornerCursors(
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
  } // switch (N)

  // Collect the cursor index for this leaf
  leaves->SetId(l, cursorIdx);

  // Determine ownership of corner
  bool owner = true;
  if (cursorIdx != this->IndiceCentralCursor)
  {
    vtkHyperTreeGridGeometryLevelEntry& cursor = this->Entries[this->GetIndiceEntry(cursorIdx)];
    if (!cursor.GetTree() || !cursor.IsLeaf(this->Grid))
    {
      // If neighbor cell is out of bounds or has Non been
      // refined to a leaf, that leaf does Non own the corner
      owner = false;
    }
    else if (this->GetGrid()->HasMask() &&
      this->GetGrid()->GetMask()->GetTuple1(cursor.GetGlobalNodeIndex()))
    {
      // If neighbor cell is masked, that leaf does Non own the corner
      owner = false;
    }
    else if (this->IndiceCentralCursor < cursorIdx && cursor.GetLevel() == this->GetLevel())
    {
      // A level tie is broken in favor of the largest index
      owner = false;
    }
  } // if(cursorIdx! = this->IndiceCentralCursor)

  // Return ownership of corner by this node
  return owner;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedMooreSuperCursor::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "--vtkHyperTreeGridNonOrientedMooreSuperCursor--" << endl;
  vtkHyperTreeGridNonOrientedSuperCursor::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedMooreSuperCursor::~vtkHyperTreeGridNonOrientedMooreSuperCursor() {}

//-----------------------------------------------------------------------------
