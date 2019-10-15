/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright Nonice for more information.

=========================================================================*/
#include "vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight.h"

#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridLevelEntry.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"

#include "vtkObjectFactory.h"

#include <cassert>
#include <vector>

vtkStandardNewMacro(vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight);

#include "vtkHyperTreeGridNonOrientedVonNeumannSuperCursorData.cxx"

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight::Initialize(
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
      if (i > 0)
      {
        // Cell has a neighbor to the left
        unsigned int r = grid->GetShiftedLevelZeroIndex(treeIndex, (unsigned int)-1, 0, 0);
        this->Entries[1].Initialize(grid, r);
      }
      else if (isOld)
      {
        this->Entries[1].Reset();
      }
      if (i + 1 < n[0])
      {
        // Cell has a neighbor to the right
        unsigned int r = grid->GetShiftedLevelZeroIndex(treeIndex, 1, 0, 0);
        this->Entries[2].Initialize(grid, r); // au lieu de 2
      }
      else if (isOld)
      {
        this->Entries[2].Reset();
      }
      if (j > 0)
      {
        // Cell has a neighbor before
        unsigned int r = grid->GetShiftedLevelZeroIndex(treeIndex, 0, (unsigned int)-1, 0);
        this->Entries[0].Initialize(grid, r);
      }
      else if (isOld)
      {
        this->Entries[0].Reset();
      }
      if (j + 1 < n[1])
      {
        // Cell has a neighbor after
        unsigned int r = grid->GetShiftedLevelZeroIndex(treeIndex, 0, 1, 0);
        this->Entries[3].Initialize(grid, r); // au lieu de 4
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
        unsigned int r = grid->GetShiftedLevelZeroIndex(treeIndex, (unsigned int)-1, 0, 0);
        this->Entries[2].Initialize(grid, r);
      }
      else if (isOld)
      {
        this->Entries[2].Reset();
      }
      if (i + 1 < n[0])
      {
        // Cell has a neighbor to the right
        unsigned int r = grid->GetShiftedLevelZeroIndex(treeIndex, 1, 0, 0);
        this->Entries[3].Initialize(grid, r); // au lieu de 4
      }
      else if (isOld)
      {
        this->Entries[3].Reset();
      }
      if (j > 0)
      {
        // Cell has a neighbor before
        unsigned int r = grid->GetShiftedLevelZeroIndex(treeIndex, 0, (unsigned int)-1, 0);
        this->Entries[1].Initialize(grid, r);
      }
      else if (isOld)
      {
        this->Entries[1].Reset();
      }
      if (j + 1 < n[1])
      {
        // Cell has a neighbor after
        unsigned int r = grid->GetShiftedLevelZeroIndex(treeIndex, 0, 1, 0);
        this->Entries[4].Initialize(grid, r); // au lieu de 5
      }
      else if (isOld)
      {
        this->Entries[4].Reset();
      }
      if (k > 0)
      {
        // Cell has a neighbor below
        unsigned int r = grid->GetShiftedLevelZeroIndex(treeIndex, 0, 0, (unsigned int)-1);
        this->Entries[0].Initialize(grid, r);
      }
      else if (isOld)
      {
        this->Entries[0].Reset();
      }
      if (k + 1 < n[2])
      {
        // Cell has a neighbor above
        unsigned int r = grid->GetShiftedLevelZeroIndex(treeIndex, 0, 0, 1);
        this->Entries[5].Initialize(grid, r); // au lieu de 6
      }
      else if (isOld)
      {
        this->Entries[5].Reset();
      }
      break;
    }
  } // switch Dimension
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "--vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight--" << endl;
  vtkHyperTreeGridNonOrientedSuperCursorLight::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight::
  ~vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight()
{
}

//-----------------------------------------------------------------------------
