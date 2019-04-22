/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridNonOrientedVonNeumannSuperCursor.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright Nonice for more information.

=========================================================================*/
#include "vtkHyperTreeGridNonOrientedVonNeumannSuperCursor.h"

#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometryLevelEntry.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"

#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkHyperTreeGridNonOrientedVonNeumannSuperCursor);

//-----------------------------------------------------------------------------
// Super cursor traversal table to retrieve the child index for each cursor
// of the parent node. There are (2*d+1)*f^d entries in the table.
// d = 1 f = 2
static const unsigned int VonNeumannChildCursorToParentCursorTable12[6] = {
  0, 1, 1,
  1, 1, 2,
};
// d = 1 f = 3
static const unsigned int VonNeumannChildCursorToParentCursorTable13[9] = {
  0, 1, 1,
  1, 1, 1,
  1, 1, 2,
};
// d = 2 f = 2
static const unsigned int VonNeumannChildCursorToParentCursorTable22[20] = {
  0, 1, 2, 2, 2,
  0, 2, 2, 3, 2,
  2, 1, 2, 2, 4,
  2, 2, 2, 3, 4,
};
// d = 2 f = 3
static const unsigned int VonNeumannChildCursorToParentCursorTable23[45] = {
  0, 1, 2, 2, 2,
  0, 2, 2, 2, 2,
  0, 2, 2, 3, 2,
  2, 1, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 3, 2,
  2, 1, 2, 2, 4,
  2, 2, 2, 2, 4,
  2, 2, 2, 3, 4,
};
// d = 3 f = 2
static const unsigned int VonNeumannChildCursorToParentCursorTable32[56] = {
  0, 1, 2, 3, 3, 3, 3,
  0, 1, 3, 3, 4, 3, 3,
  0, 3, 2, 3, 3, 5, 3,
  0, 3, 3, 3, 4, 5, 3,
  3, 1, 2, 3, 3, 3, 6,
  3, 1, 3, 3, 4, 3, 6,
  3, 3, 2, 3, 3, 5, 6,
  3, 3, 3, 3, 4, 5, 6,
};
// d = 3 f = 3
static const unsigned int VonNeumannChildCursorToParentCursorTable33[189] = {
  0, 1, 2, 3, 3, 3, 3,
  0, 1, 3, 3, 3, 3, 3,
  0, 1, 3, 3, 4, 3, 3,
  0, 3, 2, 3, 3, 3, 3,
  0, 3, 3, 3, 3, 3, 3,
  0, 3, 3, 3, 4, 3, 3,
  0, 3, 2, 3, 3, 5, 3,
  0, 3, 3, 3, 3, 5, 3,
  0, 3, 3, 3, 4, 5, 3,
  3, 1, 2, 3, 3, 3, 3,
  3, 1, 3, 3, 3, 3, 3,
  3, 1, 3, 3, 4, 3, 3,
  3, 3, 2, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 4, 3, 3,
  3, 3, 2, 3, 3, 5, 3,
  3, 3, 3, 3, 3, 5, 3,
  3, 3, 3, 3, 4, 5, 3,
  3, 1, 2, 3, 3, 3, 6,
  3, 1, 3, 3, 3, 3, 6,
  3, 1, 3, 3, 4, 3, 6,
  3, 3, 2, 3, 3, 3, 6,
  3, 3, 3, 3, 3, 3, 6,
  3, 3, 3, 3, 4, 3, 6,
  3, 3, 2, 3, 3, 5, 6,
  3, 3, 3, 3, 3, 5, 6,
  3, 3, 3, 3, 4, 5, 6,
};
static const unsigned int* VonNeumannChildCursorToParentCursorTable[3][2] = {
  {VonNeumannChildCursorToParentCursorTable12,
   VonNeumannChildCursorToParentCursorTable13},
  {VonNeumannChildCursorToParentCursorTable22,
   VonNeumannChildCursorToParentCursorTable23},
  {VonNeumannChildCursorToParentCursorTable32,
   VonNeumannChildCursorToParentCursorTable33}
};
//-----------------------------------------------------------------------------
// Super cursor traversal table to go retrieve the child index for each cursor
// of the child node. There are (2*d+1)*f^d entries in the table.
// d = 1 f = 2
static const unsigned int VonNeumannChildCursorToChildTable12[6] = {
  1, 0, 1,
  0, 1, 0,
};
// d = 1 f = 3
static const unsigned int VonNeumannChildCursorToChildTable13[9] = {
  2, 0, 1,
  0, 1, 2,
  1, 2, 0,
};
// d = 2 f = 2
static const unsigned int VonNeumannChildCursorToChildTable22[20] = {
  2, 1, 0, 1, 2,
  3, 0, 1, 0, 3,
  0, 3, 2, 3, 0,
  1, 2, 3, 2, 1,
};
// d = 2 f = 3
static const unsigned int VonNeumannChildCursorToChildTable23[45] = {
  6, 2, 0, 1, 3,
  7, 0, 1, 2, 4,
  8, 1, 2, 0, 5,
  0, 5, 3, 4, 6,
  1, 3, 4, 5, 7,
  2, 4, 5, 3, 8,
  3, 8, 6, 7, 0,
  4, 6, 7, 8, 1,
  5, 7, 8, 6, 2,
};
// d = 3 f = 2
static const unsigned int VonNeumannChildCursorToChildTable32[56] = {
  4, 2, 1, 0, 1, 2, 4,
  5, 3, 0, 1, 0, 3, 5,
  6, 0, 3, 2, 3, 0, 6,
  7, 1, 2, 3, 2, 1, 7,
  0, 6, 5, 4, 5, 6, 0,
  1, 7, 4, 5, 4, 7, 1,
  2, 4, 7, 6, 7, 4, 2,
  3, 5, 6, 7, 6, 5, 3,
};
// d = 3 f = 3
static const unsigned int VonNeumannChildCursorToChildTable33[189] = {
  18, 6, 2, 0, 1, 3, 9,
  19, 7, 0, 1, 2, 4, 10,
  20, 8, 1, 2, 0, 5, 11,
  21, 0, 5, 3, 4, 6, 12,
  22, 1, 3, 4, 5, 7, 13,
  23, 2, 4, 5, 3, 8, 14,
  24, 3, 8, 6, 7, 0, 15,
  25, 4, 6, 7, 8, 1, 16,
  26, 5, 7, 8, 6, 2, 17,
  0, 15, 11, 9, 10, 12, 18,
  1, 16, 9, 10, 11, 13, 19,
  2, 17, 10, 11, 9, 14, 20,
  3, 9, 14, 12, 13, 15, 21,
  4, 10, 12, 13, 14, 16, 22,
  5, 11, 13, 14, 12, 17, 23,
  6, 12, 17, 15, 16, 9, 24,
  7, 13, 15, 16, 17, 10, 25,
  8, 14, 16, 17, 15, 11, 26,
  9, 24, 20, 18, 19, 21, 0,
  10, 25, 18, 19, 20, 22, 1,
  11, 26, 19, 20, 18, 23, 2,
  12, 18, 23, 21, 22, 24, 3,
  13, 19, 21, 22, 23, 25, 4,
  14, 20, 22, 23, 21, 26, 5,
  15, 21, 26, 24, 25, 18, 6,
  16, 22, 24, 25, 26, 19, 7,
  17, 23, 25, 26, 24, 20, 8,
};
static const unsigned int* VonNeumannChildCursorToChildTable[3][2] = {
  {VonNeumannChildCursorToChildTable12,
   VonNeumannChildCursorToChildTable13},
  {VonNeumannChildCursorToChildTable22,
   VonNeumannChildCursorToChildTable23},
  {VonNeumannChildCursorToChildTable32,
   VonNeumannChildCursorToChildTable33}
};
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedVonNeumannSuperCursor::Initialize( vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create )
{
  assert( "pre: Non_valid_dimension" &&
          grid->GetDimension() >= 1 &&
          grid->GetDimension() <= 3 );
  assert( "pre: Non_valid_branchfactor" &&
          grid->GetBranchFactor() >= 2 &&
          grid->GetBranchFactor() <= 3 );

  if ( this->Grid == nullptr )
  {
    this->Grid = grid;
  }
  assert( "pre: Non_same_grid" &&
          this->Grid == grid );

  // JB Initialize caracteristique
  switch ( grid->GetDimension() )
  {
    case 1:
    {
      switch ( grid->GetBranchFactor() )
      {
         case 2:
         {
           // dimension = 1, branch factor = 2
           this->IndiceCentralCursor = 1;
           this->NumberOfCursors = 3;
           this->ChildCursorToParentCursorTable
             = VonNeumannChildCursorToParentCursorTable[0][0];
           this->ChildCursorToChildTable
             = VonNeumannChildCursorToChildTable[0][0];
           break;
        }
        case 3:
        {
           // dimension = 1, branch factor = 3
           this->IndiceCentralCursor = 1;
           this->NumberOfCursors = 3;
           this->ChildCursorToParentCursorTable
             = VonNeumannChildCursorToParentCursorTable[0][1];
           this->ChildCursorToChildTable
             = VonNeumannChildCursorToChildTable[0][1];
           break;
        }
      } // switch BranchFactor
      break;
    }
    case 2:
    {
      switch ( grid->GetBranchFactor() )
      {
         case 2:
         {
           // dimension = 2, branch factor = 2
           this->IndiceCentralCursor = 2;
           this->NumberOfCursors = 5;
           this->ChildCursorToParentCursorTable
             = VonNeumannChildCursorToParentCursorTable[1][0];
           this->ChildCursorToChildTable
             = VonNeumannChildCursorToChildTable[1][0];
           break;
         }
         case 3:
         {
           // dimension = 2, branch factor = 3
           this->IndiceCentralCursor = 2;
           this->NumberOfCursors = 5;
           this->ChildCursorToParentCursorTable
             = VonNeumannChildCursorToParentCursorTable[1][1];
           this->ChildCursorToChildTable
             = VonNeumannChildCursorToChildTable[1][1];
           break;
         }
      } // switch BranchFactor
      break;
    }
    case 3:
    {
      switch ( grid->GetBranchFactor() )
      {
         case 2:
         {
           // dimension = 3, branch factor = 2
           this->IndiceCentralCursor = 3;
           this->NumberOfCursors = 7;
           this->ChildCursorToParentCursorTable
             = VonNeumannChildCursorToParentCursorTable[2][0];
           this->ChildCursorToChildTable
             = VonNeumannChildCursorToChildTable[2][0];
           break;
         }
         case 3:
         {
           // dimension = 3, branch factor = 3
           this->IndiceCentralCursor = 3;
           this->NumberOfCursors = 7;
           this->ChildCursorToParentCursorTable
             = VonNeumannChildCursorToParentCursorTable[2][1];
           this->ChildCursorToChildTable
             = VonNeumannChildCursorToChildTable[2][1];
           break;
         }
       } // switch BranchFactor
       break;
    }
  } // switch Dimension

  // JB Pour le niveau zero tout est defini
  this->CentralCursor->Initialize( grid, treeIndex, create );
  //
  this->CurrentFirstNonValidEntryByLevel = 0;
  if ( this->FirstNonValidEntryByLevel.size() <= this->CurrentFirstNonValidEntryByLevel + 1 )
  {
    this->FirstNonValidEntryByLevel.resize( this->CurrentFirstNonValidEntryByLevel + 1 );
  }
  this->FirstNonValidEntryByLevel[ this->CurrentFirstNonValidEntryByLevel ] = this->NumberOfCursors - 1;
  bool isOld = true;
  if ( this->Entries.size() <= this->CurrentFirstNonValidEntryByLevel + 1 )
  {
    isOld = false;
    this->Entries.resize( this->FirstNonValidEntryByLevel[ this->CurrentFirstNonValidEntryByLevel ] );
  }
  // JB Pour le niveau zero tout est reference
  this->FirstCurrentNeighboorReferenceEntry = 0;
  if ( this->ReferenceEntries.size() <= this->FirstCurrentNeighboorReferenceEntry + this->NumberOfCursors - 1 )
  {
    this->ReferenceEntries.resize( this->FirstCurrentNeighboorReferenceEntry + this->NumberOfCursors - 1 );
  }
  std::vector< unsigned int >::iterator it = this->ReferenceEntries.begin();
  for ( unsigned int icrt = this->FirstCurrentNeighboorReferenceEntry;
        it != this->ReferenceEntries.end();
        ++ it, ++ icrt )
  {
    (*it) = icrt;
  }

  // If dimension=d: center cursor is d
  //                 d-faces neighbor cursors are 0,...,2d except d
  unsigned int i, j, k;
  grid->GetLevelZeroCoordinatesFromIndex( treeIndex, i, j, k );
  unsigned int n[3];
  grid->GetCellDims( n );

  // JB Initialisation des cursors
  switch ( grid->GetDimension() )
  {
    case 1:
    {
        // dimension == 1
       if( i > 0 )
       {
         // Cell has a neighbor to the left
         unsigned int r =
           grid->GetShiftedLevelZeroIndex( treeIndex, (unsigned int)-1, 0, 0 );
         this->Entries[0].Initialize( grid, r );
       } else if ( isOld ) {
         this->Entries[0].Reset();
       }
       if( i + 1 < n[0] )
       {
         // Cell has a neighbor to the right
         unsigned int r =
           grid->GetShiftedLevelZeroIndex( treeIndex, 1, 0, 0 );
         this->Entries[1].Initialize( grid, r ); // au lieu de 2
       } else if ( isOld ) {
         this->Entries[1].Reset();
       }
       break;
    }
    case 2:
    {
       // dimension == 2
       if( i > 0 )
       {
         // Cell has a neighbor to the left
         unsigned int r =
           grid->GetShiftedLevelZeroIndex( treeIndex, (unsigned int)-1, 0, 0 );
         this->Entries[1].Initialize( grid, r );
       } else if ( isOld ) {
         this->Entries[1].Reset();
       }
       if( i + 1 < n[0] )
       {
         // Cell has a neighbor to the right
         unsigned int r =
           grid->GetShiftedLevelZeroIndex( treeIndex, 1, 0, 0 );
         this->Entries[2].Initialize( grid, r ); // au lieu de 2
       } else if ( isOld ) {
         this->Entries[2].Reset();
       }
       if( j > 0 )
       {
         // Cell has a neighbor before
         unsigned int r =
           grid->GetShiftedLevelZeroIndex( treeIndex, 0, (unsigned int)-1, 0 );
         this->Entries[0].Initialize( grid, r );
       } else if ( isOld ) {
         this->Entries[0].Reset();
       }
       if( j + 1 < n[1] )
       {
         // Cell has a neighbor after
         unsigned int r =
           grid->GetShiftedLevelZeroIndex( treeIndex, 0, 1, 0 );
         this->Entries[3].Initialize( grid, r ); // au lieu de 4
       } else if ( isOld ) {
         this->Entries[3].Reset();
       }
       break;
    }
    case 3:
    {
       // dimension == 3
       if( i > 0 )
       {
         // Cell has a neighbor to the left
         unsigned int r =
           grid->GetShiftedLevelZeroIndex( treeIndex, (unsigned int)-1, 0, 0 );
         this->Entries[2].Initialize( grid, r );
       } else if ( isOld ) {
         this->Entries[2].Reset();
       }
       if( i + 1 < n[0] )
       {
         // Cell has a neighbor to the right
         unsigned int r =
           grid->GetShiftedLevelZeroIndex( treeIndex, 1, 0, 0 );
         this->Entries[3].Initialize( grid, r ); // au lieu de 4
       } else if ( isOld ) {
         this->Entries[3].Reset();
       }
       if( j > 0 )
       {
         // Cell has a neighbor before
         unsigned int r =
           grid->GetShiftedLevelZeroIndex( treeIndex, 0, (unsigned int)-1, 0 );
         this->Entries[1].Initialize( grid, r );
       } else if ( isOld ) {
         this->Entries[1].Reset();
       }
       if( j + 1 < n[1] )
       {
         // Cell has a neighbor after
         unsigned int r =
           grid->GetShiftedLevelZeroIndex( treeIndex, 0, 1, 0 );
         this->Entries[4].Initialize( grid, r ); // au lieu de 5
       } else if ( isOld ) {
         this->Entries[4].Reset();
       }
       if ( k > 0 )
       {
         // Cell has a neighbor below
         unsigned int r =
           grid->GetShiftedLevelZeroIndex( treeIndex, 0, 0, (unsigned int)-1 );
         this->Entries[0].Initialize( grid, r );
       } else if ( isOld ) {
         this->Entries[0].Reset();
       }
       if ( k + 1 < n[2] )
       {
         // Cell has a neighbor above
         unsigned int r =
           grid->GetShiftedLevelZeroIndex( treeIndex, 0, 0, 1 );
         this->Entries[5].Initialize( grid, r ); // au lieu de 6
       } else if ( isOld ) {
         this->Entries[5].Reset();
       }
       break;
    }
  } // switch Dimension
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedVonNeumannSuperCursor::PrintSelf( ostream& os, vtkIndent indent )
{
  os << indent << "--vtkHyperTreeGridNonOrientedVonNeumannSuperCursor--" << endl;
  vtkHyperTreeGridNonOrientedSuperCursor::PrintSelf( os, indent );
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedVonNeumannSuperCursor::~vtkHyperTreeGridNonOrientedVonNeumannSuperCursor()
{
#ifndef NDEBUG
  std::cerr << "vtkHyperTreeGridNonOrientedVonNeumannSuperCursor:" << std::endl;
#endif
}

//-----------------------------------------------------------------------------
