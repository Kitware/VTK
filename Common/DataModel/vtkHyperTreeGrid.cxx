/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGrid.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGrid.h"

#include "vtkBitArray.h"
#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkCellLinks.h"
#include "vtkCellType.h"
#include "vtkCollection.h"
#include "vtkDoubleArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkGenericCell.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeCursor.h"
#include "vtkHyperTreeGridCursor.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStructuredData.h"
#include "vtkVoxel.h"

#include <cassert>

vtkInformationKeyMacro( vtkHyperTreeGrid, LEVELS, Integer );
vtkInformationKeyMacro( vtkHyperTreeGrid, DIMENSION, Integer );
vtkInformationKeyMacro( vtkHyperTreeGrid, ORIENTATION, Integer );
vtkInformationKeyRestrictedMacro( vtkHyperTreeGrid, SIZES, DoubleVector, 3 );

vtkStandardNewMacro( vtkHyperTreeGrid );

vtkCxxSetObjectMacro( vtkHyperTreeGrid, MaterialMask, vtkBitArray );
vtkCxxSetObjectMacro( vtkHyperTreeGrid, MaterialMaskIndex, vtkIdTypeArray );
vtkCxxSetObjectMacro( vtkHyperTreeGrid, XCoordinates, vtkDataArray );
vtkCxxSetObjectMacro( vtkHyperTreeGrid, YCoordinates, vtkDataArray );
vtkCxxSetObjectMacro( vtkHyperTreeGrid, ZCoordinates, vtkDataArray );

// Helper macros to quickly fetch a HT at a given index or iterator
#define GetHyperTreeFromOtherMacro( _obj_, _index_ ) \
  ( static_cast<vtkHyperTree*>( _obj_->HyperTrees.find( _index_ ) \
                                != _obj_->HyperTrees.end() ? \
                                _obj_->HyperTrees[ _index_ ] : 0 ) )
#define GetHyperTreeFromThisMacro( _index_ ) GetHyperTreeFromOtherMacro( this, _index_ )

//=============================================================================
// Hyper tree grid depth-first cursor: a hyper tree cursor for DFS traversal
// of hyper tree grids.
// Implemented here to hide templates.
template<int N> class vtkGeometricCursor : public vtkHyperTreeGridCursor
{
public:
  //---------------------------------------------------------------------------
  vtkTemplateTypeMacro(vtkGeometricCursor<N>, vtkHyperTreeGridCursor);

  //---------------------------------------------------------------------------
  static vtkGeometricCursor<N>* New();

  //---------------------------------------------------------------------------
  void PrintSelf( ostream& os, vtkIndent indent ) override
  {
    this->Superclass::PrintSelf( os, indent );

    os << indent << "Grid: " << this->Grid << endl;

    os << indent << "TreeIndex: " << this->TreeIndex << endl;

    os << indent << "Origin: "
       << this->Origin[0] <<","
       << this->Origin[1] <<","
       << this->Origin[2] << endl;

    os << indent << "Size: "
       << this->Size[0] <<","
       << this->Size[1] <<","
       << this->Size[2] << endl;
  }

  //---------------------------------------------------------------------------
  vtkHyperTreeGridCursor* Clone() override
  {
    // Call superclass
    vtkGeometricCursor<N>* clone
      = static_cast<vtkGeometricCursor<N>*>( this->Superclass::Clone() );
    assert( "post: clone_exists" && clone != 0 );

    // Copy iVars specific to this subclass
    clone->Grid      = this->Grid;
    clone->TreeIndex = this->TreeIndex;
    memcpy( clone->Origin, this->Origin, 3 * sizeof( double ) );
    memcpy( clone->Size,   this->Size,   3 * sizeof( double ) );

    // Return clone
    return clone;
  }

  //---------------------------------------------------------------------------
  void Initialize( vtkHyperTreeGrid* grid, vtkIdType index ) override
  {
    // Call superclass
    this->Superclass::Initialize( grid, index );

    // Initialize iVars specific to this subclass
    this->Grid = grid;
    this->TreeIndex = index;

    // Retrieve Cartesian coordinates of root tree
    unsigned int i, j, k;
    grid->GetLevelZeroCoordinatesFromIndex( index, i, j, k );

    // Compute origin and size of the cursor
    vtkDataArray* xCoords = grid->GetXCoordinates();
    vtkDataArray* yCoords = grid->GetYCoordinates();
    vtkDataArray* zCoords = grid->GetZCoordinates();
    this->Origin[0] = xCoords->GetTuple1( i );
    this->Origin[1] = yCoords->GetTuple1( j );
    this->Origin[2] = zCoords->GetTuple1( k );
    this->Size[0] = xCoords->GetTuple1( i + 1 ) - this->Origin[0];
    this->Size[1] = yCoords->GetTuple1( j + 1 ) - this->Origin[1];
    this->Size[2] = zCoords->GetTuple1( k + 1 ) - this->Origin[2];
  }

  //---------------------------------------------------------------------------
  void ToRoot() override
  {
    // Call superclass
    this->Superclass::ToRoot();

    // Reset origin and extent to that of root tree
    unsigned int i, j, k;
    this->Grid->GetLevelZeroCoordinatesFromIndex( this->TreeIndex, i, j, k );
    vtkDataArray* xCoords = this->Grid->GetXCoordinates();
    vtkDataArray* yCoords = this->Grid->GetYCoordinates();
    vtkDataArray* zCoords = this->Grid->GetZCoordinates();
    this->Origin[0] = xCoords->GetTuple1( i );
    this->Origin[1] = yCoords->GetTuple1( j );
    this->Origin[2] = zCoords->GetTuple1( k );
    this->Size[0] = xCoords->GetTuple1( i + 1 ) - this->Origin[0];
    this->Size[1] = yCoords->GetTuple1( j + 1 ) - this->Origin[1];
    this->Size[2] = zCoords->GetTuple1( k + 1 ) - this->Origin[2];
  }

  //---------------------------------------------------------------------------
  void ToChild( int child ) override
  {
    // Call superclass
    this->Superclass::ToChild( child );

    // Divide cell size and translate origin per template parameter
    switch ( N )
    {
      case 2:
      {
        // dimension = 1, branch factor = 2
        unsigned int axis = this->Grid->GetOrientation();
        this->Size[axis] /= 2.;
        this->Origin[axis] += ( child & 1 ) * this->Size[axis];
        break;
      } // case 2
      case 4:
      {
        // dimension = 2, branch factor = 2
        unsigned int axis1 = 0;
        unsigned int axis2 = 1;
        switch ( this->Grid->GetOrientation() )
        {
          case 0:
            axis1 = 1;
            VTK_FALLTHROUGH;
          case 1:
            axis2 = 2;
        }
        this->Size[axis1] /= 2.;
        this->Size[axis2] /= 2.;
        this->Origin[axis1] += ( child & 1 ) * this->Size[axis1];
        this->Origin[axis2] += ( ( child & 2 ) >> 1 ) * this->Size[axis2];
        break;
      } // case 4
      case 8:
      {
        // dimension = 3, branch factor = 2
        this->Size[0] /= 2.;
        this->Size[1] /= 2.;
        this->Size[2] /= 2.;
        this->Origin[0] += ( child & 1 ) * this->Size[0];
        this->Origin[1] += ( ( child & 2 ) >> 1 ) * this->Size[1];
        this->Origin[2] += ( ( child & 4 ) >> 2 ) * this->Size[2];
        break;
      } // case 8
      case 3:
      {
        // dimension = 1, branch factor = 3
        unsigned int axis = this->Grid->GetOrientation();
        this->Size[axis] /= 3.;
        this->Origin[axis] += ( child % 3 ) * this->Size[axis];
        break;
      } // case 3
      case 9:
      {
        // dimension = 2, branch factor = 3
        unsigned int axis1 = 0;
        unsigned int axis2 = 1;
        switch ( this->Grid->GetOrientation() )
        {
          case 0:
            axis1 = 1;
            VTK_FALLTHROUGH;
          case 1:
            axis2 = 2;
        }
        this->Size[axis1] /= 3.;
        this->Size[axis2] /= 3.;
        this->Origin[axis1] += ( child % 3 ) * this->Size[axis1];
        this->Origin[axis2] += ( ( child % 9 ) / 3 ) * this->Size[axis2];
        break;
      } // case 9
      case 27:
      {
        // dimension = 3, branch factor = 3
        this->Size[0] /= 3.;
        this->Size[1] /= 3.;
        this->Size[2] /= 3.;
        this->Origin[0] += ( child % 3 ) * this->Size[0];
        this->Origin[1] += ( ( child % 9 ) / 3 ) * this->Size[1];
        this->Origin[2] += ( child / 9 ) * this->Size[2];
        break;
      } // case 27
    } // switch ( N )
  }

  //---------------------------------------------------------------------------
  double* GetOrigin() override
  {
    return this->Origin;
  }

  //---------------------------------------------------------------------------
  double* GetSize() override
  {
    return this->Size;
  }

  //---------------------------------------------------------------------------
  void GetBounds( double bnd[6] ) override
  {
    // Compute bounds
    bnd[0] = this->Origin[0];
    bnd[1] = this->Origin[0] + this->Size[0];
    bnd[2] = this->Origin[1];
    bnd[3] = this->Origin[1] + this->Size[1];
    bnd[4] = this->Origin[2];
    bnd[5] = this->Origin[2] + this->Size[2];
  }

  //---------------------------------------------------------------------------
  void GetPoint( double pt[3] ) override
  {
    // Compute center point coordinates
    pt[0] = this->Origin[0] + this->Size[0] / 2.;
    pt[1] = this->Origin[1] + this->Size[1] / 2.;
    pt[2] = this->Origin[2] + this->Size[2] / 2.;
  }

  //---------------------------------------------------------------------------
protected:
  vtkGeometricCursor<N>()
  {
    // No grid by default
    this->Grid      = 0;

    // Default origin
    this->Origin[0] = 0.;
    this->Origin[1] = 0.;
    this->Origin[2] = 0.;

    // Default size
    this->Size[0]   = 0.;
    this->Size[1]   = 0.;
    this->Size[2]   = 0.;

    // Default tree index
    this->TreeIndex = 0;
  }

  // Index of tree to which the cursor is attached in the hyper tree grid
  vtkIdType TreeIndex;

  // Origin coordinates of the root node
  double Origin[3];

  // Geometric extent of the root node
  double Size[3];

  //---------------------------------------------------------------------------
private:
  vtkGeometricCursor(const vtkGeometricCursor<N> &) = delete;
  void operator=(const vtkGeometricCursor<N> &) = delete;
}; // class vtkGeometricCursor : public vtkHyperTreeGridCursor
//-----------------------------------------------------------------------------
template<int N>
vtkStandardNewMacro(vtkGeometricCursor<N>);
//=============================================================================

//=============================================================================
// Hyper tree grid super cursor: an abstract base class for hyper tree grid
// super cursors.
// Implemented here to hide templates.
template<int N> class vtkSuperCursor : public vtkGeometricCursor<N>
{
public:
  //---------------------------------------------------------------------------
  vtkTemplateTypeMacro(vtkSuperCursor<N>,vtkGeometricCursor<N>);

  //---------------------------------------------------------------------------
  void PrintSelf( ostream& os, vtkIndent indent ) override
  {
    this->Superclass::PrintSelf( os, indent );

    os << indent << "NumberOfCursors: " << this->NumberOfCursors << endl;
    if ( this->Cursors )
    {
      os << indent << "Cursors:";
      for ( unsigned int i = 0; i < this->NumberOfCursors; ++ i )
      {
        os << " " << this->Cursors[i];
      }
    }
    else
    {
      os << indent << "Cursors: (None)\n";
    }
  }

  //---------------------------------------------------------------------------
  vtkHyperTreeGridCursor* Clone() override
  {
    // Call superclass
    vtkSuperCursor<N>* clone
      = static_cast<vtkSuperCursor<N>*>( this->Superclass::Clone() );
    assert( "post: clone_exists" && clone != 0 );

    // Return clone
    return clone;
  }

  //---------------------------------------------------------------------------
  virtual void ResetSuperCursor() = 0;

  //---------------------------------------------------------------------------
  void Initialize( vtkHyperTreeGrid* grid, vtkIdType index ) override
  {
    // Call superclass
    this->Superclass::Initialize( grid, index );

    // Initialize center cursor and its relevant neighborhood
    this->ResetSuperCursor();
  }

  //---------------------------------------------------------------------------
  unsigned int GetNumberOfCursors() override
  {
    return this->NumberOfCursors;
  }

  //---------------------------------------------------------------------------
  vtkHyperTreeGridCursor* GetCursor( unsigned int i ) override
  {
    return this->Cursors[i];
  }

  //---------------------------------------------------------------------------
  virtual unsigned int GetChildCursorToChildTable( int i )
  {
    return this->ChildCursorToChildTable[i];
  }

  //---------------------------------------------------------------------------
  void ToChild( int child ) override
  {
    // Call superclass
    this->Superclass::ToChild( child );

    // Store current cursors
    vtkHyperTreeGridCursor** parentCursors = new vtkHyperTreeGridCursor*[this->NumberOfCursors];
    for ( unsigned int i = 0; i < this->NumberOfCursors; ++ i )
    {
      parentCursors[i] = this->Cursors[i];
    }

    // Point into traversal tables at child location
    int offset = child * this->NumberOfCursors;
    const unsigned int* pTab = this->ChildCursorToParentCursorTable + offset;
    const unsigned int* cTab = this->ChildCursorToChildTable + offset;

    // Move each cursor in the supercursor down to a child
    for ( unsigned int i = 0; i < this->NumberOfCursors; ++ i )
    {
      // Make relevant cursor in parent cell point towards current child cursor
      unsigned int j = pTab[i];

      // If neighnoring cell is further subdivided, then descend into it
      this->Cursors[i] = parentCursors[j]->Clone();
      if ( parentCursors[j]->GetTree() && ! parentCursors[j]->IsLeaf() )
      {
        // Move to child
        this->Cursors[i]->ToChild( cTab[i] );
      }
    } // i

    // Clean up
    for ( unsigned int i = 0; i < this->NumberOfCursors; ++ i )
    {
      parentCursors[i]->Delete();
    }
    delete[] parentCursors;
  }

  //---------------------------------------------------------------------------
protected:
  vtkSuperCursor<N>()
  {
  }

  // Number of cursors in supercursor
  unsigned int NumberOfCursors;

  // Storage for cursors used to keep track of von Neumann neighborhoods
  vtkHyperTreeGridCursor** Cursors;

  // Super cursor traversal table to go retrieve the parent index for each cursor
  // of the child node. There are f^d * NumberOfCursors entries in the table.
  const unsigned int* ChildCursorToParentCursorTable;

  // Super cursor traversal table to go retrieve the child index for each cursor
  // of the child node. There are f^d * NumberOfCursors entries in the table.
  const unsigned int* ChildCursorToChildTable;

  //---------------------------------------------------------------------------
private:
  vtkSuperCursor(const vtkSuperCursor<N> &) = delete;
  void operator=(const vtkSuperCursor<N> &) = delete;
}; // class vtkSuperCursor : public vtkGeometricCursor
//=============================================================================

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

//=============================================================================
// Hyper tree grid von Neumann super cursor: a hyper tree grid cursor with von
// Neumann neighborhood represented with grid cursors as neighbors.
// Implemented here to hide templates.
template<int N> class vtkVonNeumannSuperCursor : public vtkSuperCursor<N>
{
public:
  //---------------------------------------------------------------------------
  vtkTemplateTypeMacro(vtkVonNeumannSuperCursor<N>,vtkSuperCursor<N>);

  //---------------------------------------------------------------------------
  static vtkVonNeumannSuperCursor<N>* New();

  //---------------------------------------------------------------------------
  ~vtkVonNeumannSuperCursor() override
  {
    if ( this->Cursors )
    {
      for ( unsigned i = 0; i < this->NumberOfCursors; ++ i )
      {
        if ( this->Cursors[i] )
        {
          this->Cursors[i]->Delete();
          this->Cursors[i] = 0;
        }
      }

      delete [] this->Cursors;
      this->Cursors = 0;
    } // if ( this->Cursors )
  }

  //---------------------------------------------------------------------------
  vtkHyperTreeGridCursor* Clone() override
  {
    // Call superclass
    vtkVonNeumannSuperCursor<N>* clone
      = static_cast<vtkVonNeumannSuperCursor<N>*>( this->Superclass::Clone() );
    assert( "post: clone_exists" && clone != 0 );

    // Copy iVars specific to this subclass
    for ( unsigned int i = 0; i < this->NumberOfCursors; ++ i )
    {
      clone->Cursors[i] = this->Cursors[i];
      this->Cursors[i]->Register( clone );
    }

    // Return clone
    return clone;
  }

  //---------------------------------------------------------------------------
  void ResetSuperCursor() override
  {
    // Create hyper tree grid cursors for von Neumann neighborhood
    for ( unsigned int i = 0; i < this->NumberOfCursors; ++ i )
    {
      this->Cursors[i] = vtkHyperTreeGridCursor::New();
    } // i

    // If dimension=d: center cursor is d
    //                 d-faces neighbor cursors are 0,...,2d except d
    unsigned int i, j, k;
    this->Grid->GetLevelZeroCoordinatesFromIndex( this->TreeIndex, i, j, k );
    unsigned int n[3];
    this->Grid->GetGridSize( n );
    switch ( N )
    {
      case 2:
      case 3:
      {
        // dimension == 1
        this->Cursors[1]->Initialize( this->Grid, this->TreeIndex );
        if( i > 0 )
        {
          // Cell has a neighbor to the left
          unsigned int r =
            this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, -1, 0, 0 );
          this->Cursors[0]->Initialize( this->Grid, r );
        }
        if( i + 1 < n[0] )
        {
          // Cell has a neighbor to the right
          unsigned int r =
            this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, 1, 0, 0 );
          this->Cursors[2]->Initialize( this->Grid, r );
        }
        return;
      }
      case 4:
      case 9:
        // dimension == 2
        this->Cursors[2]->Initialize( this->Grid, this->TreeIndex );
        if( i > 0 )
        {
          // Cell has a neighbor to the left
          unsigned int r =
            this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, -1, 0, 0 );
          this->Cursors[1]->Initialize( this->Grid, r );
        }
        if( i + 1 < n[0] )
        {
          // Cell has a neighbor to the right
          unsigned int r =
            this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, 1, 0, 0 );
          this->Cursors[3]->Initialize( this->Grid, r );
        }
        if( j > 0 )
        {
          // Cell has a neighbor before
          unsigned int r =
            this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, 0, -1, 0 );
          this->Cursors[0]->Initialize( this->Grid, r );
        }
        if( j + 1 < n[1] )
        {
          // Cell has a neighbor after
          unsigned int r =
            this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, 0, 1, 0 );
          this->Cursors[4]->Initialize( this->Grid, r );
        }
        return;
      case 8:
      case 27:
        // dimension == 3
        this->Cursors[3]->Initialize( this->Grid, this->TreeIndex );
        if( i > 0 )
        {
          // Cell has a neighbor to the left
          unsigned int r =
            this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, -1, 0, 0 );
          this->Cursors[2]->Initialize( this->Grid, r );
        }
        if( i + 1 < n[0] )
        {
          // Cell has a neighbor to the right
          unsigned int r =
            this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, 1, 0, 0 );
          this->Cursors[4]->Initialize( this->Grid, r );
        }
        if( j > 0 )
        {
          // Cell has a neighbor before
          unsigned int r =
            this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, 0, -1, 0 );
          this->Cursors[1]->Initialize( this->Grid, r );
        }
        if( j + 1 < n[1] )
        {
          // Cell has a neighbor after
          unsigned int r =
            this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, 0, 1, 0 );
          this->Cursors[5]->Initialize( this->Grid, r );
        }
        if ( k > 0 )
        {
          // Cell has a neighbor below
          unsigned int r =
            this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, 0, 0, -1 );
          this->Cursors[0]->Initialize( this->Grid, r );
        }
        if ( k + 1 < n[2] )
        {
          // Cell has a neighbor above
          unsigned int r =
            this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, 0, 0, 1 );
          this->Cursors[6]->Initialize( this->Grid, r );
        }
      } // switch ( N )
  }

  //---------------------------------------------------------------------------
protected:
  vtkVonNeumannSuperCursor<N>()
  {
    // Compute number of cursors and assign relevant traversal tables
    switch ( N )
    {
      case 2:
        // branch factor = 2, dimension = 1
        this->NumberOfCursors = 3;
        this->ChildCursorToParentCursorTable
          = VonNeumannChildCursorToParentCursorTable[0][0];
        this->ChildCursorToChildTable
          = VonNeumannChildCursorToChildTable[0][0];
        break;
      case 3:
        // branch factor = 3, dimension = 1
        this->NumberOfCursors = 3;
        this->ChildCursorToParentCursorTable
          = VonNeumannChildCursorToParentCursorTable[0][1];
        this->ChildCursorToChildTable
          = VonNeumannChildCursorToChildTable[0][1];
        break;
      case 4:
        // branch factor = 2, dimension = 2
        this->NumberOfCursors = 5;
        this->ChildCursorToParentCursorTable
          = VonNeumannChildCursorToParentCursorTable[1][0];
        this->ChildCursorToChildTable
          = VonNeumannChildCursorToChildTable[1][0];
        break;
      case 9:
        // branch factor = 2, dimension = 3
        this->NumberOfCursors = 5;
        this->ChildCursorToParentCursorTable
          = VonNeumannChildCursorToParentCursorTable[1][1];
        this->ChildCursorToChildTable
          = VonNeumannChildCursorToChildTable[1][1];
        break;
      case 8:
        // branch factor = 3, dimension = 3
        this->NumberOfCursors = 7;
        this->ChildCursorToParentCursorTable
          = VonNeumannChildCursorToParentCursorTable[2][0];
        this->ChildCursorToChildTable
          = VonNeumannChildCursorToChildTable[2][0];
        break;
      case 27:
        // branch factor = 3, dimension = 3
        this->NumberOfCursors = 7;
        this->ChildCursorToParentCursorTable
          = VonNeumannChildCursorToParentCursorTable[2][1];
        this->ChildCursorToChildTable
          = VonNeumannChildCursorToChildTable[2][1];
        break;
      default:
        // Incorrect template parameter
        return;
    } // switch ( N )

    // Allocate storage for von Neumann neighborhood cursors
    this->Cursors = new vtkHyperTreeGridCursor*[this->NumberOfCursors];
  }

  //---------------------------------------------------------------------------
private:
  vtkVonNeumannSuperCursor(const vtkVonNeumannSuperCursor<N> &) = delete;
  void operator=(const vtkVonNeumannSuperCursor<N> &) = delete;
}; // class vtkVonNeumannSuperCursor : public vtkGeometricCursor
//-----------------------------------------------------------------------------
template<int N> vtkStandardNewMacro(vtkVonNeumannSuperCursor<N>);
//=============================================================================

//-----------------------------------------------------------------------------
// Super cursor traversal table to retrieve the child index for each cursor
// of the parent node. There are (3*f)^d entries in the table.
// d = 1 f = 2
static const unsigned int MooreChildCursorToChildTable12[6] = {
  1, 0, 1,
  0, 1, 0,
};
// d = 1 f = 3
static const unsigned int MooreChildCursorToChildTable13[9] = {
  2, 0, 1,
  0, 1, 2,
  1, 2, 0,
};
// d = 2 f = 2
static const unsigned int MooreChildCursorToChildTable22[36] = {
  3, 2, 3, 1, 0, 1, 3, 2, 3,
  2, 3, 2, 0, 1, 0, 2, 3, 2,
  1, 0, 1, 3, 2, 3, 1, 0, 1,
  0, 1, 0, 2, 3, 2, 0, 1, 0,
};
// d = 2 f = 3
static const unsigned int MooreChildCursorToChildTable23[81] = {
  8, 6, 7, 2, 0, 1, 5, 3, 4,
  6, 7, 8, 0, 1, 2, 3, 4, 5,
  7, 8, 6, 1, 2, 0, 4, 5, 3,
  2, 0, 1, 5, 3, 4, 8, 6, 7,
  0, 1, 2, 3, 4, 5, 6, 7, 8,
  1, 2, 0, 4, 5, 3, 7, 8, 6,
  5, 3, 4, 8, 6, 7, 2, 0, 1,
  3, 4, 5, 6, 7, 8, 0, 1, 2,
  4, 5, 3, 7, 8, 6, 1, 2, 0,
};

// d = 3 f = 2
static const unsigned int MooreChildCursorToChildTable32[216] = {
  7, 6, 7, 5, 4, 5, 7, 6, 7, 3, 2, 3, 1, 0, 1, 3, 2, 3, 7, 6, 7, 5, 4, 5, 7, 6, 7,
  6, 7, 6, 4, 5, 4, 6, 7, 6, 2, 3, 2, 0, 1, 0, 2, 3, 2, 6, 7, 6, 4, 5, 4, 6, 7, 6,
  5, 4, 5, 7, 6, 7, 5, 4, 5, 1, 0, 1, 3, 2, 3, 1, 0, 1, 5, 4, 5, 7, 6, 7, 5, 4, 5,
  4, 5, 4, 6, 7, 6, 4, 5, 4, 0, 1, 0, 2, 3, 2, 0, 1, 0, 4, 5, 4, 6, 7, 6, 4, 5, 4,
  3, 2, 3, 1, 0, 1, 3, 2, 3, 7, 6, 7, 5, 4, 5, 7, 6, 7, 3, 2, 3, 1, 0, 1, 3, 2, 3,
  2, 3, 2, 0, 1, 0, 2, 3, 2, 6, 7, 6, 4, 5, 4, 6, 7, 6, 2, 3, 2, 0, 1, 0, 2, 3, 2,
  1, 0, 1, 3, 2, 3, 1, 0, 1, 5, 4, 5, 7, 6, 7, 5, 4, 5, 1, 0, 1, 3, 2, 3, 1, 0, 1,
  0, 1, 0, 2, 3, 2, 0, 1, 0, 4, 5, 4, 6, 7, 6, 4, 5, 4, 0, 1, 0, 2, 3, 2, 0, 1, 0,
};

// d = 3 f = 3
static const unsigned int MooreChildCursorToChildTable33[729] = {
  26, 24, 25, 20, 18, 19, 23, 21, 22, 8, 6, 7, 2, 0, 1, 5, 3, 4, 17, 15, 16, 11, 9, 10, 14, 12, 13,
  24, 25, 26, 18, 19, 20, 21, 22, 23, 6, 7, 8, 0, 1, 2, 3, 4, 5, 15, 16, 17, 9, 10, 11, 12, 13, 14,
  25, 26, 24, 19, 20, 18, 22, 23, 21, 7, 8, 6, 1, 2, 0, 4, 5, 3, 16, 17, 15, 10, 11, 9, 13, 14, 12,
  20, 18, 19, 23, 21, 22, 26, 24, 25, 2, 0, 1, 5, 3, 4, 8, 6, 7, 11, 9, 10, 14, 12, 13, 17, 15, 16,
  18, 19, 20, 21, 22, 23, 24, 25, 26, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
  19, 20, 18, 22, 23, 21, 25, 26, 24, 1, 2, 0, 4, 5, 3, 7, 8, 6, 10, 11, 9, 13, 14, 12, 16, 17, 15,
  23, 21, 22, 26, 24, 25, 20, 18, 19, 5, 3, 4, 8, 6, 7, 2, 0, 1, 14, 12, 13, 17, 15, 16, 11, 9, 10,
  21, 22, 23, 24, 25, 26, 18, 19, 20, 3, 4, 5, 6, 7, 8, 0, 1, 2, 12, 13, 14, 15, 16, 17, 9, 10, 11,
  22, 23, 21, 25, 26, 24, 19, 20, 18, 4, 5, 3, 7, 8, 6, 1, 2, 0, 13, 14, 12, 16, 17, 15, 10, 11, 9,
  8, 6, 7, 2, 0, 1, 5, 3, 4, 17, 15, 16, 11, 9, 10, 14, 12, 13, 26, 24, 25, 20, 18, 19, 23, 21, 22,
  6, 7, 8, 0, 1, 2, 3, 4, 5, 15, 16, 17, 9, 10, 11, 12, 13, 14, 24, 25, 26, 18, 19, 20, 21, 22, 23,
  7, 8, 6, 1, 2, 0, 4, 5, 3, 16, 17, 15, 10, 11, 9, 13, 14, 12, 25, 26, 24, 19, 20, 18, 22, 23, 21,
  2, 0, 1, 5, 3, 4, 8, 6, 7, 11, 9, 10, 14, 12, 13, 17, 15, 16, 20, 18, 19, 23, 21, 22, 26, 24, 25,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
  1, 2, 0, 4, 5, 3, 7, 8, 6, 10, 11, 9, 13, 14, 12, 16, 17, 15, 19, 20, 18, 22, 23, 21, 25, 26, 24,
  5, 3, 4, 8, 6, 7, 2, 0, 1, 14, 12, 13, 17, 15, 16, 11, 9, 10, 23, 21, 22, 26, 24, 25, 20, 18, 19,
  3, 4, 5, 6, 7, 8, 0, 1, 2, 12, 13, 14, 15, 16, 17, 9, 10, 11, 21, 22, 23, 24, 25, 26, 18, 19, 20,
  4, 5, 3, 7, 8, 6, 1, 2, 0, 13, 14, 12, 16, 17, 15, 10, 11, 9, 22, 23, 21, 25, 26, 24, 19, 20, 18,
  17, 15, 16, 11, 9, 10, 14, 12, 13, 26, 24, 25, 20, 18, 19, 23, 21, 22, 8, 6, 7, 2, 0, 1, 5, 3, 4,
  15, 16, 17, 9, 10, 11, 12, 13, 14, 24, 25, 26, 18, 19, 20, 21, 22, 23, 6, 7, 8, 0, 1, 2, 3, 4, 5,
  16, 17, 15, 10, 11, 9, 13, 14, 12, 25, 26, 24, 19, 20, 18, 22, 23, 21, 7, 8, 6, 1, 2, 0, 4, 5, 3,
  11, 9, 10, 14, 12, 13, 17, 15, 16, 20, 18, 19, 23, 21, 22, 26, 24, 25, 2, 0, 1, 5, 3, 4, 8, 6, 7,
  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 0, 1, 2, 3, 4, 5, 6, 7, 8,
  10, 11, 9, 13, 14, 12, 16, 17, 15, 19, 20, 18, 22, 23, 21, 25, 26, 24, 1, 2, 0, 4, 5, 3, 7, 8, 6,
  14, 12, 13, 17, 15, 16, 11, 9, 10, 23, 21, 22, 26, 24, 25, 20, 18, 19, 5, 3, 4, 8, 6, 7, 2, 0, 1,
  12, 13, 14, 15, 16, 17, 9, 10, 11, 21, 22, 23, 24, 25, 26, 18, 19, 20, 3, 4, 5, 6, 7, 8, 0, 1, 2,
  13, 14, 12, 16, 17, 15, 10, 11, 9, 22, 23, 21, 25, 26, 24, 19, 20, 18, 4, 5, 3, 7, 8, 6, 1, 2, 0,
};
static const unsigned int* MooreChildCursorToChildTable[3][2] = {
  {MooreChildCursorToChildTable12,
   MooreChildCursorToChildTable13},
  {MooreChildCursorToChildTable22,
   MooreChildCursorToChildTable23},
  {MooreChildCursorToChildTable32,
   MooreChildCursorToChildTable33}
};
//-----------------------------------------------------------------------------
// Super cursor traversal table to go retrieve the child index for each cursor
// of the child node. There are (3*f)^d entries in the table.
//-----------------------------------------------------------------------------
// d = 1 f = 2
static const unsigned int MooreChildCursorToParentCursorTable12[6] = {
  0, 1, 1,
  1, 1, 2,
};
// d = 1 f = 3
static const unsigned int MooreChildCursorToParentCursorTable13[9] = {
  0, 1, 1,
  1, 1, 1,
  1, 1, 2,
};
// d = 2 f = 2
static const unsigned int MooreChildCursorToParentCursorTable22[36] = {
  0, 1, 1, 3, 4, 4, 3, 4, 4,
  1, 1, 2, 4, 4, 5, 4, 4, 5,
  3, 4, 4, 3, 4, 4, 6, 7, 7,
  4, 4, 5, 4, 4, 5, 7, 7, 8,
};

// d = 2 f = 3
static const unsigned int MooreChildCursorToParentCursorTable23[81] = {
  0, 1, 1, 3, 4, 4, 3, 4, 4,
  1, 1, 1, 4, 4, 4, 4, 4, 4,
  1, 1, 2, 4, 4, 5, 4, 4, 5,
  3, 4, 4, 3, 4, 4, 3, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 5, 4, 4, 5, 4, 4, 5,
  3, 4, 4, 3, 4, 4, 6, 7, 7,
  4, 4, 4, 4, 4, 4, 7, 7, 7,
  4, 4, 5, 4, 4, 5, 7, 7, 8,
};

// d = 3 f = 2
static const unsigned int MooreChildCursorToParentCursorTable32[216] = {
  0, 1, 1, 3, 4, 4, 3, 4, 4, 9, 10, 10, 12, 13, 13, 12, 13, 13, 9, 10, 10, 12, 13, 13, 12, 13, 13,
  1, 1, 2, 4, 4, 5, 4, 4, 5, 10, 10, 11, 13, 13, 14, 13, 13, 14, 10, 10, 11, 13, 13, 14, 13, 13, 14,
  3, 4, 4, 3, 4, 4, 6, 7, 7, 12, 13, 13, 12, 13, 13, 15, 16, 16, 12, 13, 13, 12, 13, 13, 15, 16, 16,
  4, 4, 5, 4, 4, 5, 7, 7, 8, 13, 13, 14, 13, 13, 14, 16, 16, 17, 13, 13, 14, 13, 13, 14, 16, 16, 17,
  9, 10, 10, 12, 13, 13, 12, 13, 13, 9, 10, 10, 12, 13, 13, 12, 13, 13, 18, 19, 19, 21, 22, 22, 21, 22, 22,
  10, 10, 11, 13, 13, 14, 13, 13, 14, 10, 10, 11, 13, 13, 14, 13, 13, 14, 19, 19, 20, 22, 22, 23, 22, 22, 23,
  12, 13, 13, 12, 13, 13, 15, 16, 16, 12, 13, 13, 12, 13, 13, 15, 16, 16, 21, 22, 22, 21, 22, 22, 24, 25, 25,
  13, 13, 14, 13, 13, 14, 16, 16, 17, 13, 13, 14, 13, 13, 14, 16, 16, 17, 22, 22, 23, 22, 22, 23, 25, 25, 26,
};

// d = 3 f = 3
static const unsigned int MooreChildCursorToParentCursorTable33[729] = {
  0, 1, 1, 3, 4, 4, 3, 4, 4, 9, 10, 10, 12, 13, 13, 12, 13, 13, 9, 10, 10, 12, 13, 13, 12, 13, 13,
  1, 1, 1, 4, 4, 4, 4, 4, 4, 10, 10, 10, 13, 13, 13, 13, 13, 13, 10, 10, 10, 13, 13, 13, 13, 13, 13,
  1, 1, 2, 4, 4, 5, 4, 4, 5, 10, 10, 11, 13, 13, 14, 13, 13, 14, 10, 10, 11, 13, 13, 14, 13, 13, 14,
  3, 4, 4, 3, 4, 4, 3, 4, 4, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
  4, 4, 5, 4, 4, 5, 4, 4, 5, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14,
  3, 4, 4, 3, 4, 4, 6, 7, 7, 12, 13, 13, 12, 13, 13, 15, 16, 16, 12, 13, 13, 12, 13, 13, 15, 16, 16,
  4, 4, 4, 4, 4, 4, 7, 7, 7, 13, 13, 13, 13, 13, 13, 16, 16, 16, 13, 13, 13, 13, 13, 13, 16, 16, 16,
  4, 4, 5, 4, 4, 5, 7, 7, 8, 13, 13, 14, 13, 13, 14, 16, 16, 17, 13, 13, 14, 13, 13, 14, 16, 16, 17,
  9, 10, 10, 12, 13, 13, 12, 13, 13, 9, 10, 10, 12, 13, 13, 12, 13, 13, 9, 10, 10, 12, 13, 13, 12, 13, 13,
  10, 10, 10, 13, 13, 13, 13, 13, 13, 10, 10, 10, 13, 13, 13, 13, 13, 13, 10, 10, 10, 13, 13, 13, 13, 13, 13,
  10, 10, 11, 13, 13, 14, 13, 13, 14, 10, 10, 11, 13, 13, 14, 13, 13, 14, 10, 10, 11, 13, 13, 14, 13, 13, 14,
  12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13,
  13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
  13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14,
  12, 13, 13, 12, 13, 13, 15, 16, 16, 12, 13, 13, 12, 13, 13, 15, 16, 16, 12, 13, 13, 12, 13, 13, 15, 16, 16,
  13, 13, 13, 13, 13, 13, 16, 16, 16, 13, 13, 13, 13, 13, 13, 16, 16, 16, 13, 13, 13, 13, 13, 13, 16, 16, 16,
  13, 13, 14, 13, 13, 14, 16, 16, 17, 13, 13, 14, 13, 13, 14, 16, 16, 17, 13, 13, 14, 13, 13, 14, 16, 16, 17,
  9, 10, 10, 12, 13, 13, 12, 13, 13, 9, 10, 10, 12, 13, 13, 12, 13, 13, 18, 19, 19, 21, 22, 22, 21, 22, 22,
  10, 10, 10, 13, 13, 13, 13, 13, 13, 10, 10, 10, 13, 13, 13, 13, 13, 13, 19, 19, 19, 22, 22, 22, 22, 22, 22,
  10, 10, 11, 13, 13, 14, 13, 13, 14, 10, 10, 11, 13, 13, 14, 13, 13, 14, 19, 19, 20, 22, 22, 23, 22, 22, 23,
  12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 21, 22, 22, 21, 22, 22, 21, 22, 22,
  13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 22, 22, 22, 22, 22, 22, 22, 22, 22,
  13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 22, 22, 23, 22, 22, 23, 22, 22, 23,
  12, 13, 13, 12, 13, 13, 15, 16, 16, 12, 13, 13, 12, 13, 13, 15, 16, 16, 21, 22, 22, 21, 22, 22, 24, 25, 25,
  13, 13, 13, 13, 13, 13, 16, 16, 16, 13, 13, 13, 13, 13, 13, 16, 16, 16, 22, 22, 22, 22, 22, 22, 25, 25, 25,
  13, 13, 14, 13, 13, 14, 16, 16, 17, 13, 13, 14, 13, 13, 14, 16, 16, 17, 22, 22, 23, 22, 22, 23, 25, 25, 26,
};
static const unsigned int* MooreChildCursorToParentCursorTable[3][2] = {
  {MooreChildCursorToParentCursorTable12,
   MooreChildCursorToParentCursorTable13},
  {MooreChildCursorToParentCursorTable22,
   MooreChildCursorToParentCursorTable23},
  {MooreChildCursorToParentCursorTable32,
   MooreChildCursorToParentCursorTable33}
};
//-----------------------------------------------------------------------------
// Corner/leaf traversal tables to retrieve the parent cursor indices of all
// leaves touching a given corner of the parent node.
//-----------------------------------------------------------------------------
static const int CornerNeighborCursorsTable1D0[2] = {
   0, 1, };
static const int CornerNeighborCursorsTable1D1[2] = {
   1, 2, };
static const int* CornerNeighborCursorsTable1D[2] = {
  CornerNeighborCursorsTable1D0,
  CornerNeighborCursorsTable1D1,
};
static const int CornerNeighborCursorsTable2D0[4] = {
   0, 1, 3, 4, };
static const int CornerNeighborCursorsTable2D1[4] = {
   1, 2, 4, 5, };
static const int CornerNeighborCursorsTable2D2[4] = {
   3, 4, 6, 7, };
static const int CornerNeighborCursorsTable2D3[4] = {
   4, 5, 7, 8, };
static const int* CornerNeighborCursorsTable2D[4] = {
  CornerNeighborCursorsTable2D0,
  CornerNeighborCursorsTable2D1,
  CornerNeighborCursorsTable2D2,
  CornerNeighborCursorsTable2D3,
};
static const unsigned int CornerNeighborCursorsTable3D0[8] = {
   0, 1, 3, 4, 9, 10, 12, 13, };
static const unsigned int CornerNeighborCursorsTable3D1[8] = {
   1, 2, 4, 5, 10, 11, 13, 14, };
static const unsigned int CornerNeighborCursorsTable3D2[8] = {
   3, 4, 6, 7, 12, 13, 15, 16, };
static const unsigned int CornerNeighborCursorsTable3D3[8] = {
   4, 5, 7, 8, 13, 14, 16, 17, };
static const unsigned int CornerNeighborCursorsTable3D4[8] = {
   9, 10, 12, 13, 18, 19, 21, 22, };
static const unsigned int CornerNeighborCursorsTable3D5[8] = {
   10, 11, 13, 14, 19, 20, 22, 23, };
static const unsigned int CornerNeighborCursorsTable3D6[8] = {
   12, 13, 15, 16, 21, 22, 24, 25, };
static const unsigned int CornerNeighborCursorsTable3D7[8] = {
   13, 14, 16, 17, 22, 23, 25, 26, };
static const unsigned int* CornerNeighborCursorsTable3D[8] = {
  CornerNeighborCursorsTable3D0,
  CornerNeighborCursorsTable3D1,
  CornerNeighborCursorsTable3D2,
  CornerNeighborCursorsTable3D3,
  CornerNeighborCursorsTable3D4,
  CornerNeighborCursorsTable3D5,
  CornerNeighborCursorsTable3D6,
  CornerNeighborCursorsTable3D7,
};
//-----------------------------------------------------------------------------

//=============================================================================
// Hyper tree grid Moore super cursor: a hyper tree grid cursor with Moore
// neighborhood represented with geometric cursors as neighbors.
// Implemented here to hide templates.
template<int N> class vtkMooreSuperCursor : public vtkSuperCursor<N>
{
public:
  //---------------------------------------------------------------------------
  vtkTemplateTypeMacro(vtkMooreSuperCursor<N>,vtkSuperCursor<N>);

  //---------------------------------------------------------------------------
  static vtkMooreSuperCursor<N>* New();

  //---------------------------------------------------------------------------
  ~vtkMooreSuperCursor() override
  {
    if ( this->Cursors )
    {
      for ( unsigned i = 0; i < this->NumberOfCursors; ++ i )
      {
        if ( this->Cursors[i] )
        {
          this->Cursors[i]->Delete();
          this->Cursors[i] = 0;
        }
      }

      delete [] this->Cursors;
      this->Cursors = 0;
    } // if ( this->Cursors )
  }

  //---------------------------------------------------------------------------
  vtkHyperTreeGridCursor* Clone() override
  {
    // Call superclass
    vtkMooreSuperCursor<N>* clone
      = static_cast<vtkMooreSuperCursor<N>*>( this->Superclass::Clone() );
    assert( "post: clone_exists" && clone != 0 );

    // Copy iVars specific to this subclass
    for ( unsigned int i = 0; i < this->NumberOfCursors; ++ i )
    {
      clone->Cursors[i] = this->Cursors[i];
      this->Cursors[i]->Register( clone );
    }

    // Return clone
    return clone;
  }

  //---------------------------------------------------------------------------
  void ResetSuperCursor() override
  {
#define vtkInitializeAsGeometricCursorMacro( _N_, _c_, _g_, _r_ )  \
  {                                                                \
  vtkGeometricCursor<_N_>* cursor                                  \
    = vtkGeometricCursor<_N_>::SafeDownCast( this->Cursors[_c_] ); \
  cursor->Initialize( _g_, _r_ );                                  \
  }

    // Create hyper tree grid cursors for Moore neighborhood
    for ( unsigned int i = 0; i < this->NumberOfCursors; ++ i )
    {
      this->Cursors[i] = vtkGeometricCursor<N>::New();
    } // i

    // If dimension=d: center cursor is d
    //                 d-faces neighbor cursors are 0,...,2d except d
    unsigned int i, j, k;
    this->Grid->GetLevelZeroCoordinatesFromIndex( this->TreeIndex, i, j, k );
    unsigned int n[3];
    this->Grid->GetGridSize( n );
    switch ( N )
    {
      case 2:
      case 3:
        // dimension == 1
        this->Cursors[1]->Initialize( this->Grid, this->TreeIndex );
        if( i > 0 )
        {
          // Cell has a neighbor to the left
          unsigned int r =
            this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, -1, 0, 0 );
          this->Cursors[0]->Initialize( this->Grid, r );
        }
        if( i + 1 < n[0] )
        {
          // Cell has a neighbor to the right
          unsigned int r =
            this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, 1, 0, 0 );
          this->Cursors[2]->Initialize( this->Grid, r );
        }
        return;
      case 4:
      case 9:
      {
        // dimension == 2
        vtkInitializeAsGeometricCursorMacro( N, 4, this->Grid, this->TreeIndex );
        bool toW = ( i > 0 );
        bool toS = ( j > 0 );
        bool toE = ( i + 1 < n[0] );
        bool toN = ( j + 1 < n[1] );
        if ( toS )
        {
          // Cell has a neighbor to the south
          unsigned int r =
            this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, 0, -1, 0 );
          vtkInitializeAsGeometricCursorMacro( N, 1, this->Grid, r );
          if ( toW )
          {
            // Cell has a neighbor to the southwest
            r =
              this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, -1, -1, 0 );
            vtkInitializeAsGeometricCursorMacro( N, 0, this->Grid, r );
          } // if ( toW )
          if ( toE )
          {
            // Cell has a neighbor to the southeast
            r =
              this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, 1, -1, 0 );
            vtkInitializeAsGeometricCursorMacro( N, 2, this->Grid, r );
          } // if ( toE )
        } // if ( toS )
        if ( toW )
        {
          // Cell has a neighbor to the west
          unsigned int r =
            this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, -1, 0, 0 );
          vtkInitializeAsGeometricCursorMacro( N, 3, this->Grid, r );
        } // if ( toW )
        if ( toE )
        {
          // Cell has a neighbor to the east
          unsigned int r =
            this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, 1, 0, 0 );
          vtkInitializeAsGeometricCursorMacro( N, 5, this->Grid, r );
        } // if ( toE )
        if ( toN )
        {
          // Cell has a neighbor to the north
          unsigned int r =
            this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, 0, 1, 0 );
          vtkInitializeAsGeometricCursorMacro( N, 7, this->Grid, r );
          if ( toW )
          {
            // Cell has a neighbor to the northwest
            r =
              this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, -1, 1, 0 );
            vtkInitializeAsGeometricCursorMacro( N, 6, this->Grid, r );
          } // if ( toW )
          if ( toE )
          {
            // Cell has a neighbor to the northeast
            r =
              this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, 1, 1, 0 );
            vtkInitializeAsGeometricCursorMacro( N, 8, this->Grid, r );
          } // if ( toW )
        } // if ( toN )
        return;
      } // cases 4 and 9
      case 8:
      case 27:
      {
        // dimension == 3
        int minI = ( i == 0 ) ? 0 : -1;
        int maxI = ( i + 1 < n[0] ) ? 2 : 1;
        int minJ = ( j == 0 ) ? 0 : -1;
        int maxJ = ( j + 1 < n[1] ) ? 2 : 1;
        int minK = ( k == 0 ) ? 0 : -1;
        int maxK = ( k + 1 < n[2] ) ? 2 : 1;

        // Initialize all connectivity cursors
        for ( int _k = minK; _k < maxK; ++ _k )
        {
          for ( int _j = minJ; _j < maxJ; ++ _j )
          {
            for ( int _i = minI; _i < maxI; ++ _i )
            {
              int c = 13 + _i + 3 * _j + 9 * _k;
                unsigned int r =
                  this->Grid->GetShiftedLevelZeroIndex( this->TreeIndex, _i, _j, _k );
              vtkInitializeAsGeometricCursorMacro( N, c, this->Grid, r );
            } // _i
          } // _j
        } // _k
      } // cases 8 and 27
    } // switch ( N )

#undef vtkInitializeAsGeometricCursorMacro
  }

  //---------------------------------------------------------------------------
  bool GetCornerCursors( unsigned int c, unsigned int l, vtkIdList* leaves ) override
  {
    unsigned int cursorIdx = 0;
    unsigned int centerCursorIdx = 0;
    switch ( N )
    {
      case 2:
      case 3:
        // dimension == 1
        centerCursorIdx = 1;
        cursorIdx = CornerNeighborCursorsTable1D[c][l];
        break;
      case 4:
      case 9:
        // dimension == 2
        centerCursorIdx = 4;
        cursorIdx = CornerNeighborCursorsTable2D[c][l];
        break;
      case 8:
      case 27:
        // dimension == 3
        centerCursorIdx = 13;
        cursorIdx = CornerNeighborCursorsTable3D[c][l];
        break;
      default:
        vtkErrorMacro("unexpected neighborhood");
        return false;
    } // switch ( N )

    // Collect the cursor index for this leaf
    leaves->SetId( l, cursorIdx );

    // Determine ownership of corner
    bool owner = true;
    if( cursorIdx != centerCursorIdx )
    {
      //
      vtkHyperTreeGridCursor* cursor = this->Cursors[cursorIdx];
      if ( ! cursor->GetTree() || ! cursor->IsLeaf() )
      {
        // If neighbor cell is out of bounds or has not been
        // refined to a leaf, that leaf does not own the corner
        owner = false;
      }
      else if ( this->Grid->HasMaterialMask()
                && this->Grid->GetMaterialMask()->GetTuple1( cursor->GetGlobalNodeIndex() ) )
      {
        // If neighbor cell is masked, that leaf does not own the corner
        owner = false;
      }
      else if ( centerCursorIdx < cursorIdx &&
                cursor->GetLevel() == this->Cursors[centerCursorIdx]->GetLevel() )
      {
        // A level tie is broken in favor of the largest index
        owner = false;
      }
    } // if( cursorIdx! = centerCursorIdx )

    // Return ownership of corner by this node
    return owner;
  }

  //---------------------------------------------------------------------------
protected:
  vtkMooreSuperCursor<N>()
  {
    // Compute number of cursors and assign relevant traversal tables
    switch ( N )
    {
      case 2:
        // branch factor = 2, dimension = 1
        this->NumberOfCursors = 3;
        this->ChildCursorToParentCursorTable
          = MooreChildCursorToParentCursorTable[0][0];
        this->ChildCursorToChildTable
          = MooreChildCursorToChildTable[0][0];
        break;
      case 3:
        // branch factor = 3, dimension = 1
        this->NumberOfCursors = 3;
        this->ChildCursorToParentCursorTable
          = MooreChildCursorToParentCursorTable[0][1];
        this->ChildCursorToChildTable
          = MooreChildCursorToChildTable[0][1];
        break;
      case 4:
        // branch factor = 2, dimension = 2
        this->NumberOfCursors = 9;
        this->ChildCursorToParentCursorTable
          = MooreChildCursorToParentCursorTable[1][0];
        this->ChildCursorToChildTable
          = MooreChildCursorToChildTable[1][0];
        break;
      case 9:
        // branch factor = 2, dimension = 3
        this->NumberOfCursors = 9;
        this->ChildCursorToParentCursorTable
          = MooreChildCursorToParentCursorTable[1][1];
        this->ChildCursorToChildTable
          = MooreChildCursorToChildTable[1][1];
        break;
      case 8:
        // branch factor = 3, dimension = 3
        this->NumberOfCursors = 27;
        this->ChildCursorToParentCursorTable
          = MooreChildCursorToParentCursorTable[2][0];
        this->ChildCursorToChildTable
          = MooreChildCursorToChildTable[2][0];
        break;
      case 27:
        // branch factor = 3, dimension = 3
        this->NumberOfCursors = 27;
        this->ChildCursorToParentCursorTable
          = MooreChildCursorToParentCursorTable[2][1];
        this->ChildCursorToChildTable
          = MooreChildCursorToChildTable[2][1];
        break;
      default:
        // Incorrect template parameter
        return;
    } // switch ( N )

    // Allocate storage for Moore neighborhood cursors
    this->Cursors = new vtkHyperTreeGridCursor*[this->NumberOfCursors];
  }

  //---------------------------------------------------------------------------
private:
  vtkMooreSuperCursor(const vtkMooreSuperCursor<N> &) = delete;
  void operator=(const vtkMooreSuperCursor<N> &) = delete;
}; // class vtkMooreSuperCursor : public vtkGeometricCursor
//-----------------------------------------------------------------------------
template<int N> vtkStandardNewMacro(vtkMooreSuperCursor<N>);
//=============================================================================

//-----------------------------------------------------------------------------
vtkHyperTreeGrid::vtkHyperTreeGrid()
{
  // Dual grid corners (primal grid leaf centers)
  this->Points = nullptr;
  this->Connectivity = nullptr;

  // Internal links
  this->Links = nullptr;

  // Grid topology
  this->GridSize[0] = 0;
  this->GridSize[1] = 0;
  this->GridSize[2] = 0;
  this->TransposedRootIndexing = false;

  // Grid parameters
  this->Dimension = 1;
  this->Orientation = 0;
  this->BranchFactor = 2;
  this->NumberOfChildren = 2;

  // Masked primal leaves
  this->MaterialMask = vtkBitArray::New();
  this->MaterialMaskIndex = nullptr;
  this->PureMaterialMask = nullptr;
  this->InitPureMaterialMask = false;

  // No interface by default
  this->HasInterface = false;

  // Interface array names
  this->InterfaceNormalsName = nullptr;
  this->InterfaceInterceptsName = nullptr;

  // Primal grid geometry
  this->XCoordinates = vtkDoubleArray::New();
  this->YCoordinates = vtkDoubleArray::New();
  this->ZCoordinates = vtkDoubleArray::New();

  // For dataset API
  this->Pixel = vtkPixel::New();
  this->Line = vtkLine::New();
  this->Voxel = vtkVoxel::New();

  // Grid extent
  int extent[6];
  extent[0] = 0;
  extent[1] = this->GridSize[0] - 1;
  extent[2] = 0;
  extent[3] = this->GridSize[1] - 1;
  extent[4] = 0;
  extent[5] = this->GridSize[2] - 1;
  memcpy( this->Extent, extent, 6 * sizeof( int ) );
  this->Information->Set( vtkDataObject::DATA_EXTENT_TYPE(), VTK_3D_EXTENT ) ;
  this->Information->Set( vtkDataObject::DATA_EXTENT(), this->Extent, 6 );
}

//-----------------------------------------------------------------------------
vtkHyperTreeGrid::~vtkHyperTreeGrid()
{
  if ( this->Points )
  {
    this->Points->Delete();
    this->Points = 0;
  }

  if ( this->Connectivity )
  {
    this->Connectivity->Delete();
    this->Connectivity = 0;
  }

  if ( this->Links )
  {
    this->Links->Delete();
    this->Links = 0;
  }

  if ( this->MaterialMask )
  {
    this->MaterialMask->Delete();
  }

  if( this->PureMaterialMask)
  {
    this->PureMaterialMask->Delete();
  }

  if ( this->MaterialMaskIndex )
  {
    this->MaterialMaskIndex->Delete();
  }

  this->SetInterfaceNormalsName( nullptr);
  this->SetInterfaceInterceptsName( nullptr );

  if ( this->XCoordinates )
  {
    this->XCoordinates->Delete();
  }

  if ( this->YCoordinates )
  {
    this->YCoordinates->Delete();
  }

  if ( this->ZCoordinates )
  {
    this->ZCoordinates->Delete();
  }

  if ( this->Pixel )
  {
    this->Pixel->Delete();
    this->Pixel = nullptr;
  }

  if ( this->Line )
  {
    this->Line->Delete();
    this->Line = nullptr;
  }

  if ( this->Voxel )
  {
    this->Voxel->Delete();
    this->Voxel = nullptr;
  }

  this->DeleteTrees();
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "Dimension: " << this->Dimension << endl;
  os << indent << "Orientation: " << this->Orientation << endl;
  os << indent << "BranchFactor: " << this->BranchFactor << endl;

  os << indent << "GridSize: "
     << this->GridSize[0] <<","
     << this->GridSize[1] <<","
     << this->GridSize[2] << endl;

  os << indent << "MaterialMask:\n";
  if ( this->MaterialMask )
  {
    this->MaterialMask->PrintSelf( os, indent.GetNextIndent() );
  }
  if ( this->PureMaterialMask )
  {
    this->PureMaterialMask->PrintSelf( os, indent.GetNextIndent() );
  }
  os << indent << "InitPureMaterialMask: "
     << ( this->InitPureMaterialMask ? "true" : "false" ) << endl;

  os << indent << "HasInterface: " << ( this->HasInterface ? "true" : "false" ) << endl;

  os << indent << "XCoordinates:\n";
  if ( this->XCoordinates )
  {
    this->XCoordinates->PrintSelf( os, indent.GetNextIndent() );
  }
  os << indent << "YCoordinates:\n";
  if ( this->YCoordinates )
  {
    this->YCoordinates->PrintSelf( os, indent.GetNextIndent() );
  }
  os << indent << "ZCoordinates:\n";
  if ( this->ZCoordinates )
  {
    this->ZCoordinates->PrintSelf( os, indent.GetNextIndent() );
  }

  os << indent << "HyperTrees: " << this->HyperTrees.size() << endl;

  os << indent << "Points: " << this->Points << endl;
  os << indent << "Connectivity: " << this->Connectivity << endl;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGrid::GetDataObjectType()
{
  return VTK_HYPER_TREE_GRID;
}

//----------------------------------------------------------------------------
vtkHyperTreeGrid* vtkHyperTreeGrid::GetData( vtkInformation* info )
{
  return info ?
    vtkHyperTreeGrid::SafeDownCast( info->Get(DATA_OBJECT() ) ) : nullptr;
}

//----------------------------------------------------------------------------
vtkHyperTreeGrid* vtkHyperTreeGrid::GetData( vtkInformationVector* v, int i )
{
  return vtkHyperTreeGrid::GetData( v->GetInformationObject( i ) );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::DeleteTrees()
{
  if ( !this->HyperTrees.empty() )
  {
    vtkHyperTreeGridIterator it;
    it.Initialize( this );
    while ( vtkHyperTree* tree = it.GetNextTree() )
    {
      tree->Delete();
    }
    this->HyperTrees.clear();
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::CopyStructure( vtkDataSet* ds )
{
  assert( "pre: ds_exists" && ds!=nullptr );
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast( ds );
  assert( "pre: same_type" && htg!=nullptr );

  // Copy grid parameters
  this->BranchFactor = htg->BranchFactor;
  this->Dimension = htg->Dimension;
  this->Orientation = htg->Orientation;
  memcpy( this->GridSize, htg->GetGridSize(), 3 * sizeof( int ) );
  this->NumberOfChildren = htg->NumberOfChildren;
  this->TransposedRootIndexing = htg->TransposedRootIndexing;
  this->InitPureMaterialMask = htg->InitPureMaterialMask;
  this->HasInterface = htg->HasInterface;
  this->SetInterfaceNormalsName(htg->InterfaceNormalsName);
  this->SetInterfaceInterceptsName(htg->InterfaceInterceptsName);

  // Delete existing trees
  DeleteTrees();

  // Shallow copy and register new trees
  this->HyperTrees = htg->HyperTrees;

  if ( !this->HyperTrees.empty() )
  {
    vtkHyperTreeGridIterator it;
    it.Initialize( this );
    while ( vtkHyperTree* tree = it.GetNextTree() )
    {
      tree->Register( this );
    }
  }

  // Reset dual mesh
  this->ResetDual();

  // Shallow copy points if needed
  if ( this->Points != htg->Points )
  {
    this->Points = htg->Points;
    if ( this->Points )
    {
      this->Points->Register( this );
    }
  }

  // Shallow copy connectivity if needed
  if ( this->Connectivity != htg->Connectivity )
  {
    this->Connectivity = htg->Connectivity;
    if ( this->Connectivity )
    {
      this->Connectivity->Register( this );
    }
  }

  // Shallow copy links if needed
  if ( this->Links != htg->Links )
  {
    this->Links = htg->Links;
    if ( this->Links )
    {
      this->Links->Register( this );
    }
  }

  // Shallow copy masked if needed
  if ( this->MaterialMask != htg->MaterialMask )
  {
    if ( this->MaterialMask )
    {
      this->MaterialMask->Delete();
    }
    this->MaterialMask = htg->MaterialMask;
    if ( this->MaterialMask )
    {
      this->MaterialMask->Register( this );
    }
  }

  // Shallow copy masked leaf IDs if needed
  if ( this->MaterialMaskIndex != htg->MaterialMaskIndex )
  {
    if ( this->MaterialMaskIndex )
    {
      this->MaterialMaskIndex->Delete();
    }
    this->MaterialMaskIndex = htg->MaterialMaskIndex;
    if ( this->MaterialMaskIndex )
    {
      this->MaterialMaskIndex->Register( this );
    }
  }

  // Shallow copy pure material mask if needed
  if ( this->PureMaterialMask != htg->PureMaterialMask )
  {
    if ( this->PureMaterialMask )
    {
      this->PureMaterialMask->Delete();
    }
    this->PureMaterialMask = htg->PureMaterialMask;
    if ( this->PureMaterialMask )
    {
      this->PureMaterialMask->Register( this );
    }
  }

  // Shallow copy coordinates if needed
  if ( this->XCoordinates != htg->XCoordinates )
  {
    if ( this->XCoordinates )
    {
      this->XCoordinates->Delete();
    }
    this->XCoordinates = htg->XCoordinates;
    if ( this->XCoordinates )
    {
      this->XCoordinates->Register( this );
    }
  }
  if ( this->YCoordinates != htg->YCoordinates )
  {
    if ( this->YCoordinates )
    {
      this->YCoordinates->Delete();
    }
    this->YCoordinates = htg->YCoordinates;
    if ( this->YCoordinates )
    {
      this->YCoordinates->Register( this );
    }
  }
  if ( this->ZCoordinates != htg->ZCoordinates )
  {
    if ( this->ZCoordinates )
    {
      this->ZCoordinates->Delete();
    }
    this->ZCoordinates = htg->ZCoordinates;
    if ( this->ZCoordinates )
    {
      this->ZCoordinates->Register( this );
    }
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGrid::SetGridSize( unsigned int dim[3] )
{
  this->SetGridExtent( 0, dim[0]-1, 0, dim[1]-1, 0, dim[2]-1 );
}

//----------------------------------------------------------------------------
void vtkHyperTreeGrid::SetGridSize( unsigned int i, unsigned int j, unsigned int k )
{
  this->SetGridExtent( 0, i-1, 0, j-1, 0, k-1 );
}

//----------------------------------------------------------------------------
void vtkHyperTreeGrid::SetGridExtent( int extent[6] )
{
  int description;

  description = vtkStructuredData::SetExtent( extent, this->Extent );
  if ( description < 0 ) //improperly specified
  {
    vtkErrorMacro ( << "Bad extent, retaining previous values" );
    return;
  }

  if ( description == VTK_UNCHANGED )
  {
    return;
  }

  this->GridSize[0] = extent[1] - extent[0] + 1;
  this->GridSize[1] = extent[3] - extent[2] + 1;
  this->GridSize[2] = extent[5] - extent[4] + 1;

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkHyperTreeGrid::SetGridExtent( int iMin, int iMax,
                                      int jMin, int jMax,
                                      int kMin, int kMax)
{
  int extent[6];

  extent[0] = iMin; extent[1] = iMax;
  extent[2] = jMin; extent[3] = jMax;
  extent[4] = kMin; extent[5] = kMax;

  this->SetGridExtent( extent );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::SetDimension( unsigned int dim )
{
  assert( "pre: valid_dim" && dim >= 1 && dim <= 3 );
  if ( this->Dimension == dim )
  {
    return;
  }
  this->Dimension = dim;

  // Number of children is factor^dimension
  this->NumberOfChildren = this->BranchFactor;
  for ( unsigned int i = 1; i < this->Dimension; ++ i )
  {
    this->NumberOfChildren *= this->BranchFactor;
  }

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::SetBranchFactor( unsigned int factor )
{
  assert( "pre: valid_factor" && factor >= 2 && factor <= 3 );
  if ( this->BranchFactor == factor )
  {
    return;
  }
  this->BranchFactor = factor;

  // Number of children is factor^dimension
  this->NumberOfChildren = this->BranchFactor;
  for ( unsigned int i = 1; i < this->Dimension; ++ i )
  {
    this->NumberOfChildren *= this->BranchFactor;
  }

  this->Modified();
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGrid::HasMaterialMask()
{
  return ( this->MaterialMask->GetNumberOfTuples() != 0 );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GenerateTrees()
{
  // Clean up existing trees
  this->DeleteTrees();

  // Iterate over range of tree indices
  vtkIdType n = this->GetNumberOfTrees();
  for ( vtkIdType i = 0; i < n; ++ i )
  {
    // Generate concrete instance of hyper tree
    vtkHyperTree* tree = vtkHyperTree::CreateInstance( this->BranchFactor, this->Dimension );

    // Append new hyper tree to list of roots depending on mask presence
    if ( this->MaterialMaskIndex )
    {
      this->HyperTrees[this->MaterialMaskIndex->GetValue( i )] = tree;
    }
    else
    {
      this->HyperTrees[ i ] = tree;
    }
  } // r

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkHyperTreeGrid::ComputeBounds()
{
  // Retrieve coordinate arrays
  vtkDataArray* coords[3] =
    {
      this->XCoordinates,
      this->YCoordinates,
      this->ZCoordinates
    };
  for ( unsigned int i = 0; i < 3; ++ i )
  {
    if ( ! coords[i] || ! coords[i]->GetNumberOfTuples() )
    {
      return;
    }
  }

  // Get bounds from coordinate arrays
  vtkMath::UninitializeBounds( this->Bounds );
  for ( unsigned int i = 0; i < 3; ++ i )
  {
    unsigned int di = 2 * i;
    unsigned int dip = di + 1;
    this->Bounds[di] = coords[i]->GetComponent( 0, 0 );
    this->Bounds[dip] = coords[i]->GetComponent( coords[i]->GetNumberOfTuples() - 1, 0 );

    // Ensure that the bounds are increasing
    if ( this->Bounds[di] > this->Bounds[dip] )
    {
      std::swap( this->Bounds[di], this->Bounds[dip] );
    }
  }
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::GetNumberOfTrees()
{
  return this->MaterialMaskIndex ?
    this->MaterialMaskIndex->GetNumberOfTuples() :
    this->GridSize[0] * this->GridSize[1] * this->GridSize[2];
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::GetNumberOfLevels( vtkIdType index )
{
  vtkHyperTree* tree = GetHyperTreeFromThisMacro( index );
  return tree ? tree->GetNumberOfLevels() : 0;
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::GetNumberOfLevels()
{
  vtkIdType nLevels = 0;

  // Iterate over all individual trees
  vtkIdType index;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  this->InitializeTreeIterator( it );
  while ( it.GetNextTree( index ) )
  {
    vtkIdType nl = this->GetNumberOfLevels( index );
    if ( nl > nLevels )
    {
      nLevels = nl;
    }
  } // while ( it.GetNextTree( inIndex ) )

  return nLevels;
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::GetNumberOfVertices()
{
  vtkIdType nVertices = 0;

  // Iterate over all trees in grid
  vtkHyperTreeGridIterator it;
  it.Initialize( this );
  while ( vtkHyperTree* tree = it.GetNextTree() )
  {
    nVertices += tree->GetNumberOfVertices();
  }

  return nVertices;
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::GetNumberOfLeaves()
{
  vtkIdType nLeaves = 0;

  // Iterate over all trees in grid
  vtkHyperTreeGridIterator it;
  it.Initialize( this );
  while ( vtkHyperTree* tree = it.GetNextTree() )
  {
    nLeaves += tree->GetNumberOfLeaves();
  }

  return nLeaves;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::InitializeTreeIterator( vtkHyperTreeGridIterator& it )
{
  it.Initialize( this );
}

//-----------------------------------------------------------------------------
vtkHyperTreeCursor* vtkHyperTreeGrid::NewCursor( vtkIdType index,
                                                 bool create )
{
  // Try to retrieve hyper tree at given location
  vtkHyperTree* tree = GetHyperTreeFromThisMacro( index );

  // Create a new cursor if only required to do so
  if( create && ! tree )
  {
    tree = vtkHyperTree::CreateInstance( this->BranchFactor, this->Dimension );
    this->HyperTrees[index] = tree;
  }

  // Return either found or created tree, or a null pointer
  return tree ? tree->NewCursor() : nullptr;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridCursor* vtkHyperTreeGrid::NewGridCursor( vtkIdType index,
                                                         bool create )
{
  // Try to retrieve hyper tree at given location
  vtkHyperTree* tree = GetHyperTreeFromThisMacro( index );

  // Create a new cursor if only required to do so
  if( create && ! tree )
  {
    tree = vtkHyperTree::CreateInstance( this->BranchFactor, this->Dimension );
    this->HyperTrees[index] = tree;
  }

  // Return either found or created tree, or a null pointer
  vtkHyperTreeGridCursor* result = vtkHyperTreeGridCursor::New();
  result->Initialize( this, index );
  return result;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridCursor* vtkHyperTreeGrid::NewGeometricCursor( vtkIdType index,
                                                              bool create )
{
#define vtkHyperTreeGridNewGeometricCursorMacro( _N_ )             \
  {                                                                \
  vtkGeometricCursor<_N_>* result = vtkGeometricCursor<_N_>::New();\
  result->Initialize( this, index );                               \
  return result;                                                   \
  }

  // Try to retrieve hyper tree at given location
  vtkHyperTree* tree = GetHyperTreeFromThisMacro( index );

  // Create a new cursor if only required to do so
  if( create && ! tree )
  {
    tree = vtkHyperTree::CreateInstance( this->BranchFactor, this->Dimension );
    this->HyperTrees[index] = tree;
  }

  // Instantiate new geometric cursor if tree with given index exists
  if ( tree )
  {
    switch ( this->GetDimension() )
    {
      case 1:
        switch ( this->GetBranchFactor() )
        {
          case 2: vtkHyperTreeGridNewGeometricCursorMacro( 2 );
          case 3: vtkHyperTreeGridNewGeometricCursorMacro( 3 );
        } // case this->Dimension = 1
        break;
      case 2:
        switch ( this->GetBranchFactor() )
        {
          case 2: vtkHyperTreeGridNewGeometricCursorMacro( 4 );
          case 3: vtkHyperTreeGridNewGeometricCursorMacro( 9 );
        } // case this->Dimension = 2
      break;
      case 3:
        switch ( this->GetBranchFactor() )
        {
          case 2: vtkHyperTreeGridNewGeometricCursorMacro( 8 );
          case 3: vtkHyperTreeGridNewGeometricCursorMacro( 27 );
        } // case this->Dimension = 3
        break;
    } // switch ( this->GetDimension() )
  } // if ( tree )

#undef vtkHyperTreeGridNewGeometricCursorMacro

  // Return a null pointer if tree neither found nor created
  return nullptr;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridCursor* vtkHyperTreeGrid::NewVonNeumannSuperCursor( vtkIdType index,
                                                                    bool create )
{
#define vtkHyperTreeGridNewVonNeumannSuperCursorMacro( _N_ )                   \
  {                                                                            \
  vtkVonNeumannSuperCursor<_N_>* result = vtkVonNeumannSuperCursor<_N_>::New();\
  result->Initialize( this, index );                                           \
  return result;                                                               \
  }

  // Try to retrieve hyper tree at given location
  vtkHyperTree* tree = GetHyperTreeFromThisMacro( index );

  // Create a new cursor if only required to do so
  if( create && ! tree )
  {
    tree = vtkHyperTree::CreateInstance( this->BranchFactor, this->Dimension );
    this->HyperTrees[index] = tree;
  }

  // Instantiate new supercursor if tree with given index exists
  if ( tree )
  {
    switch ( this->GetDimension() )
    {
      case 1:
        switch ( this->GetBranchFactor() )
        {
          case 2: vtkHyperTreeGridNewVonNeumannSuperCursorMacro( 2 );
          case 3: vtkHyperTreeGridNewVonNeumannSuperCursorMacro( 3 );
        } // case this->Dimension = 1
        break;
      case 2:
        switch ( this->GetBranchFactor() )
        {
          case 2: vtkHyperTreeGridNewVonNeumannSuperCursorMacro( 4 );
          case 3: vtkHyperTreeGridNewVonNeumannSuperCursorMacro( 9 );
        } // case this->Dimension = 2
      break;
      case 3:
        switch ( this->GetBranchFactor() )
        {
          case 2: vtkHyperTreeGridNewVonNeumannSuperCursorMacro( 8 );
          case 3: vtkHyperTreeGridNewVonNeumannSuperCursorMacro( 27 );
        } // case this->Dimension = 3
        break;
    } // switch ( this->GetDimension() )
  } // if ( tree )

#undef vtkHyperTreeGridNewVonNeumannSuperCursorMacro

  // Return a null pointer if tree neither found nor created
  return nullptr;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridCursor* vtkHyperTreeGrid::NewMooreSuperCursor( vtkIdType index,
                                                               bool create )
{
#define vtkHyperTreeGridNewMooreSuperCursorMacro( _N_ )              \
  {                                                                  \
  vtkMooreSuperCursor<_N_>* result = vtkMooreSuperCursor<_N_>::New();\
  result->Initialize( this, index );                                 \
  return result;                                                     \
  }

  // Try to retrieve hyper tree at given location
  vtkHyperTree* tree = GetHyperTreeFromThisMacro( index );

  // Create a new cursor if only required to do so
  if( create && ! tree )
  {
    tree = vtkHyperTree::CreateInstance( this->BranchFactor, this->Dimension );
    this->HyperTrees[index] = tree;
  }

  // Instantiate new supercursor if tree with given index exists
  if ( tree )
  {
    switch ( this->GetDimension() )
    {
      case 1:
        switch ( this->GetBranchFactor() )
        {
          case 2: vtkHyperTreeGridNewMooreSuperCursorMacro( 2 );
          case 3: vtkHyperTreeGridNewMooreSuperCursorMacro( 3 );
        } // case this->Dimension = 1
        break;
      case 2:
        switch ( this->GetBranchFactor() )
        {
          case 2: vtkHyperTreeGridNewMooreSuperCursorMacro( 4 );
          case 3: vtkHyperTreeGridNewMooreSuperCursorMacro( 9 );
        } // case this->Dimension = 2
      break;
      case 3:
        switch ( this->GetBranchFactor() )
        {
          case 2: vtkHyperTreeGridNewMooreSuperCursorMacro( 8 );
          case 3: vtkHyperTreeGridNewMooreSuperCursorMacro( 27 );
        } // case this->Dimension = 3
        break;
    } // switch ( this->GetDimension() )
  } // if ( tree )

#undef vtkHyperTreeGridNewMooreSuperCursorMacro

  // Return a null pointer if tree neither found nor created
  return nullptr;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::SubdivideLeaf( vtkHyperTreeCursor* leaf, vtkIdType id )
{
  assert( "pre: leaf_exists" && leaf );
  assert( "pre: is_a_leaf" && leaf->IsLeaf() );
  vtkHyperTree* tree = GetHyperTreeFromThisMacro( id );
  if ( tree )
  {
    tree->SubdivideLeaf( leaf );
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::Initialize()
{
  // Delete existing trees
  this->DeleteTrees();

  // Reset dual mesh
  this->ResetDual();
}

//-----------------------------------------------------------------------------
vtkHyperTree* vtkHyperTreeGrid::GetTree( vtkIdType id )
{
  // Wrap convenience macro for outside use
  return GetHyperTreeFromThisMacro( id );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::SetTree( vtkIdType id, vtkHyperTree* tree )
{
  // Search for hyper tree with given index
  std::map<vtkIdType, vtkHyperTree*>::iterator it = this->HyperTrees.find( id );

  // If found, perform replacement
  if( it != this->HyperTrees.end() )
  {
    if( it->second == tree )
    {
      return;
    }
    // Unregister tree to avoid double reference
    tree->Delete();
  }

  // Assign given tree at given index of hyper tree grid
  this->HyperTrees[id] = tree;

  // Register tree
  tree->Register( this );
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGrid::GetMaxCellSize()
{
  switch ( this->Dimension )
  {
    case 3:
      // Hexahedron, 8 vertices
      return 8;
    case 2:
      // Quadrangle, 4 vertices
      return 4;
    case 1:
      // Line segment, 2 vertices
      return 2;
    default:
      // This is useless, just to avoid a warning
      assert( "check: bad grid dimension" && 0 );
      return 0;
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::ShallowCopy( vtkDataObject* src )
{
  assert( "src_same_type" && vtkHyperTreeGrid::SafeDownCast( src ) );
  this->CopyStructure(vtkHyperTreeGrid::SafeDownCast( src ) );

  // Call superclass
  this->Superclass::ShallowCopy( src );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::DeepCopyCursors( vtkHyperTreeCursor* ic,
                                        vtkHyperTreeCursor* oc )
{
  // Retrieve trees
  vtkHyperTree* it = ic->GetTree();
  vtkHyperTree* ot = oc->GetTree();

  // Get and set global indices
  vtkIdType ii =it-> GetGlobalIndexFromLocal( ic->GetVertexId() );
  ot->SetGlobalIndexFromLocal( oc->GetVertexId(), ii );

  // If input cursor not at leaf, subdivide output tree
  if( ! ic->IsLeaf() )
  {
    ot->SubdivideLeaf( oc );

    int nbChild = 1;
    int bf = this->GetBranchFactor();
    switch ( this->GetDimension() )
    {
      case 3:
        nbChild *= bf;
        VTK_FALLTHROUGH;
      case 2:
        nbChild *= bf;
        VTK_FALLTHROUGH;
      case 1:
        nbChild *= bf;
    } // switch ( this->GetDimension() )

    // Iterate over children
    for ( int newChildIdx = 0; newChildIdx < nbChild ; ++ newChildIdx )
    {
      // Move down to children
      ic->ToChild( newChildIdx );
      oc->ToChild( newChildIdx );

      // Recurse
      this->DeepCopyCursors( ic, oc );

      // Move up to parents
      ic->ToParent();
      oc->ToParent();
    } // for newChildIdx
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::DeepCopy( vtkDataObject* src )
{
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast( src );
  assert( "src_same_type" && htg );
  //this->CopyStructure( htg );

  // FIXME: some iVars are missing here
  this->Dimension = htg->Dimension;
  this->Orientation = htg->Orientation;
  this->BranchFactor = htg->BranchFactor;
  this->NumberOfChildren = htg->NumberOfChildren;
  this->TransposedRootIndexing = htg->TransposedRootIndexing;
  memcpy( this->GridSize, htg->GetGridSize(), 3 * sizeof( int ) );

  // Initialize iterators
  vtkHyperTreeGrid::vtkHyperTreeGridIterator iit;
  htg->InitializeTreeIterator( iit );
  vtkHyperTreeGrid::vtkHyperTreeGridIterator oit;
  this->InitializeTreeIterator( oit );

  // Reset dual mesh
  this->ResetDual();

  if( htg->Points )
  {
    this->Points = vtkPoints::New();
    this->Points->Register( this );
    this->Points->DeepCopy( htg->Points);
    this->Points->Delete();
  }

  if( htg->Connectivity )
  {
    this->Connectivity = vtkIdTypeArray::New();
    this->Connectivity->Register( this );
    this->Connectivity->DeepCopy( htg->Connectivity);
    this->Connectivity->Delete();
  }

  if( htg->Links )
  {
    this->Links = vtkCellLinks::New();
    this->Links->Register( this );
    this->Links->DeepCopy( htg->Links );
    this->Links->Delete();
  }

  this->MaterialMask = vtkBitArray::New();
  this->MaterialMask->Register( this );
  this->MaterialMask->DeepCopy( htg->MaterialMask );
  this->MaterialMask->Delete();

  if( htg->PureMaterialMask )
  {
    this->PureMaterialMask = vtkBitArray::New();
    this->PureMaterialMask->Register( this );
    this->PureMaterialMask->DeepCopy( htg->PureMaterialMask );
    this->PureMaterialMask->Delete();
  }

  this->MaterialMaskIndex = vtkIdTypeArray::New();
  this->MaterialMaskIndex->Register( this );
  this->MaterialMaskIndex->DeepCopy( htg->MaterialMask );
  this->MaterialMaskIndex->Delete();

  this->XCoordinates = htg->XCoordinates->NewInstance();
  this->XCoordinates->Register( this );
  this->XCoordinates->DeepCopy( htg->XCoordinates );
  this->XCoordinates->Delete();
  this->YCoordinates = htg->YCoordinates->NewInstance();
  this->YCoordinates->Register( this );
  this->YCoordinates->DeepCopy( htg->YCoordinates );
  this->YCoordinates->Delete();
  this->ZCoordinates = htg->ZCoordinates->NewInstance();
  this->ZCoordinates->Register( this );
  this->ZCoordinates->DeepCopy( htg->ZCoordinates );
  this->ZCoordinates->Delete();

  // Call superclass
  this->Superclass::DeepCopy( src );
}

//=============================================================================
// DataSet API that returns dual grid.
//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::GetNumberOfCells()
{
  this->ComputeDualGrid();
  return this->GetConnectivity()->GetNumberOfTuples();
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::GetNumberOfPoints()
{
  // TODO: This could be dramatically improved by using only leaf count
  return this->GetNumberOfVertices();
}

//-----------------------------------------------------------------------------
double* vtkHyperTreeGrid::GetPoint( vtkIdType ptId )
{
  this->ComputeDualGrid();
  vtkPoints* leafCenters = this->GetPoints();
  assert( "Index out of bounds." && ptId >= 0
          && ptId < leafCenters->GetNumberOfPoints() );
  return leafCenters->GetPoint( ptId );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetPoint( vtkIdType ptId, double x[3] )
{
  this->ComputeDualGrid();
  vtkPoints* leafCenters = this->GetPoints();
  assert( "Index out of bounds." && ptId >= 0
          && ptId < leafCenters->GetNumberOfPoints() );
  leafCenters->GetPoint( ptId, x );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetCellImplementation( vtkIdType cellId, vtkCell* cell )
{
  assert( "Null cell ptr." && cell != nullptr );

  int numPts = 1 << this->Dimension;
  double x[3];

  this->ComputeDualGrid();
  vtkIdTypeArray* cornerLeafIds = this->GetConnectivity();
  assert( "Index out of bounds." &&
          cellId >= 0 && cellId < cornerLeafIds->GetNumberOfTuples() );
  vtkPoints* leafCenters = this->GetPoints();
  vtkIdType* ptr = cornerLeafIds->GetPointer( 0 ) + cellId*numPts;
  for ( int ptIdx = 0; ptIdx < numPts; ++ptIdx, ++ptr )
  {
    cell->PointIds->SetId( ptIdx, *ptr );
    leafCenters->GetPoint( *ptr, x );
    cell->Points->SetPoint( ptIdx, x );
  }
}

//-----------------------------------------------------------------------------
vtkCell* vtkHyperTreeGrid::GetCell( vtkIdType cellId )
{
  vtkCell* cell = nullptr;
  switch ( this->Dimension )
  {
    case 1:
      cell = this->Line;
      break;
    case 2:
      cell = this->Pixel;
      break;
    case 3:
      cell = this->Voxel;
      break;
    default:
      assert( "post: bad grid dimension" && false );
      return nullptr;          // impossible case
  }

  this->GetCellImplementation( cellId, cell );
  return cell;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetCell( vtkIdType cellId, vtkGenericCell* cell )
{
  assert( "GetCell on null cell." && cell != nullptr );
  switch ( this->Dimension )
  {
    case 1:
      cell->SetCellTypeToLine();
      break;
    case 2:
      cell->SetCellTypeToPixel();
      break;
    case 3:
      cell->SetCellTypeToVoxel();
      break;
    default:
      assert( "post: bad grid dimension" && false );
      return;            // impossible case
  }

  this->GetCellImplementation( cellId, static_cast<vtkCell*>( cell ) );
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGrid::GetCellType( vtkIdType vtkNotUsed(cellId) )
{
  switch ( this->Dimension )
  {
    case 1:
      return VTK_LINE;  // line = 2 points
    case 2:
      return VTK_PIXEL; // quad = 4 points
    case 3:
      return VTK_VOXEL; // hexahedron = 8 points
    default:
      assert( "post: bad grid dimension" && false );
      return 0;         // impossible case
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetCellPoints( vtkIdType cellId, vtkIdList* ptIds )
{
  int numPts = 1 << this->Dimension;
  ptIds->Initialize();
  ptIds->SetNumberOfIds( numPts );

  this->ComputeDualGrid();
  vtkIdTypeArray* cornerLeafIds = this->GetConnectivity();
  assert( "Index out of bounds." &&
          cellId >= 0 && cellId < cornerLeafIds->GetNumberOfTuples() );
  vtkIdType* ptr = cornerLeafIds->GetPointer( 0 ) + cellId * numPts;
  memcpy( ptIds->GetPointer(0 ), ptr, numPts * sizeof( vtkIdType ) );
}

//----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetCellPoints( vtkIdType cellId,
                                      vtkIdType& npts,
                                      vtkIdType* &pts )
{
  this->ComputeDualGrid();
  vtkIdTypeArray* cornerLeafIds = this->GetConnectivity();
  assert( "Index out of bounds." &&
          cellId >= 0 && cellId < cornerLeafIds->GetNumberOfTuples() );
  unsigned int i_npts = 1 << this->Dimension; //avoid a 32/64 shift warning, dims is always < 4, so safe
  npts = static_cast<vtkIdType>( i_npts );
  pts = cornerLeafIds->GetPointer( 0 ) + cellId * npts;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetPointCells( vtkIdType ptId, vtkIdList* cellIds )
{
  if ( ! this->Links )
  {
    this->BuildLinks();
  }
  cellIds->Reset();

  int numCells = this->Links->GetNcells( ptId );
  cellIds->SetNumberOfIds( numCells );

  vtkIdType* cells = this->Links->GetCells( ptId );
  for ( int i = 0; i < numCells; ++ i )
  {
    cellIds->SetId( i, cells[i] );
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::BuildLinks()
{
  this->Links = vtkCellLinks::New();
  this->Links->Allocate( this->GetNumberOfPoints() );
  this->Links->Register( this );
  this->Links->BuildLinks( this );
  this->Links->Delete();
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetCellNeighbors( vtkIdType cellId,
                                         vtkIdList* ptIds,
                                         vtkIdList* cellIds )
{
  if ( ! this->Links )
  {
    this->BuildLinks();
  }

  cellIds->Reset();

  vtkIdType numPts = ptIds->GetNumberOfIds();
  if (numPts <= 0 )
  {
    vtkErrorMacro("input point ids empty.");
    return;
  }

  int minNumCells = VTK_INT_MAX;
  vtkIdType* pts = ptIds->GetPointer( 0 );
  vtkIdType* minCells = nullptr;
  vtkIdType minPtId = 0;
  // Find the point used by the fewest number of cells
  for ( vtkIdType i = 0; i < numPts; i++ )
  {
    vtkIdType ptId = pts[i];
    int numCells = this->Links->GetNcells( ptId );
    if ( numCells < minNumCells )
    {
      minNumCells = numCells;
      minCells = this->Links->GetCells( ptId );
      minPtId = ptId;
    }
  }

  // Allocate storage for cell IDs
  cellIds->Allocate( minNumCells );

  // For all cells that contNow for each cell, see if it contains all the points
  // in the ptIds list.
  for ( int i = 0; i < minNumCells; i++ )
  {
    // Do not include current cell
    if ( minCells[i] != cellId )
    {
      vtkIdType* cellPts;
      vtkIdType npts;
      this->GetCellPoints( minCells[i], npts, cellPts );
      // Iterate over all points in input cell
      bool match = true;
      for ( vtkIdType j = 0; j < numPts && match; j++ )
      {
        // Skip point with index minPtId which is contained by current cell
        if ( pts[j] != minPtId )
        {
          // Iterate over all points in current cell
          match = false;
          for ( vtkIdType k = 0; k < npts; ++ k )
          {
            if ( pts[j] == cellPts[k] )
            {
              // A match was found
              match = true;
              break;
            }
          } // For all points in current cell
        } // If not guaranteed match
      } // For all points in input cell
      if ( match )
      {
        cellIds->InsertNextId( minCells[i] );
      }
    } // If not the reference cell
  } // For all candidate cells attached to point
}

//----------------------------------------------------------------------------
bool vtkHyperTreeGrid::RecursivelyInitializePureMaterialMask( vtkHyperTreeGridCursor* cursor )
{
  // Retrieve mask value at cursor
  int id = cursor->GetGlobalNodeIndex();
  bool mask = (this->MaterialMask->GetTuple1( id  )!=0);

  //  Dot recurse if node is masked or is a leaf
  if( ! mask && ! cursor->IsLeaf() )
  {
    // Iterate over all chidren
    int numChildren = this->GetNumberOfChildren();
    for ( int child = 0; child < numChildren; ++ child )
    {
      // Create child cursor from parent
      vtkHyperTreeGridCursor* childCursor = cursor->Clone();
      childCursor->ToChild( child );

      // Recurse
      mask |= this->RecursivelyInitializePureMaterialMask( childCursor );

      // Clean up
      childCursor->Delete();
    }
  }

  // Set and return pure material mask with recursively computed value
  this->PureMaterialMask->SetTuple1( id, mask );
  return mask;
}

//----------------------------------------------------------------------------
vtkBitArray* vtkHyperTreeGrid::GetPureMaterialMask()
{
  // Check whether a pure material mask was initialized
  if( ! this->InitPureMaterialMask )
  {
    // If not, then create one
    this->PureMaterialMask = vtkBitArray::New();
    this->PureMaterialMask->SetNumberOfTuples( this->MaterialMask->GetNumberOfTuples() );

    // Iterate over hyper tree grid
    vtkIdType index;
    vtkHyperTreeGridIterator it;
    it.Initialize( this );
    while ( it.GetNextTree( index ) )
    {
      // Create cursor instance over current hyper tree
      vtkHyperTreeGridCursor* cursor = this->NewGridCursor( index );

      // Recursively initialize pure material mask
      this->RecursivelyInitializePureMaterialMask( cursor );

      // Clean up
      cursor->Delete();
    }

    // Keep track of the fact that a pure material mask now exists
    this->InitPureMaterialMask = true;
  }

  // Return existing or created pure material mask
  return this->PureMaterialMask;
}

//----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::FindPoint( double x[3] )
{
  vtkIdType ix = 0;
  vtkIdType nx = this->XCoordinates->GetNumberOfTuples();
  while ( ix < nx && x[0] > this->XCoordinates->GetTuple1( ix ) )
  {
    ++ ix;
  }
  if ( ix )
  {
    -- ix;
  }

  vtkIdType iy = 0;
  vtkIdType ny = this->YCoordinates->GetNumberOfTuples();
  while ( iy < ny && x[1] > this->YCoordinates->GetTuple1( iy ) )
  {
    ++ iy;
  }
  if ( iy )
  {
    -- iy;
  }

  vtkIdType iz = 0;
  vtkIdType nz = this->ZCoordinates->GetNumberOfTuples();
  while ( iz < nz && x[2] > this->ZCoordinates->GetTuple1( iz ) )
  {
    ++ iz;
  }
  if ( iz )
  {
    -- iz;
  }

  int index = ( this->TransposedRootIndexing ) ?
    ( ix * this->GridSize[1] + iy ) * this->GridSize[2] + iz :
    ( iz * this->GridSize[1] + iy ) * this->GridSize[0] + ix;

  vtkHyperTreeGridCursor* cursor = this->NewGeometricCursor( index );

  // Geometry of the cell
  double origin[3] =
    {
      this->XCoordinates->GetTuple1( ix ),
      this->YCoordinates->GetTuple1( iy ),
      this->ZCoordinates->GetTuple1( iz )
    };

  double extreme[3] =
    {
      this->XCoordinates->GetTuple1( ix + 1 ),
      this->YCoordinates->GetTuple1( iy + 1 ),
      this->ZCoordinates->GetTuple1( iz + 1 )
    };

  double size[3] =
    {
      extreme[0] - origin[0],
      extreme[1] - origin[1],
      extreme[2] - origin[2]
    };

  vtkIdType id = this->RecursivelyFindPoint( x, cursor, origin, size );

  cursor->Delete();

  return id;
}

//----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::RecursivelyFindPoint( double x[3],
                                                 vtkHyperTreeGridCursor* cursor,
                                                 double* origin,
                                                 double* size )
{
  if ( cursor->IsLeaf() )
  {
    return cursor->GetGlobalNodeIndex();
  }

  double newSize[3];
  double newOrigin[3];
  int child = 0;
  for ( int i = 0; i < 3; ++ i )
  {
    newSize[i] = size[i] * 0.5;
    newOrigin[i] = origin[i];
    if ( x[i] >= origin[i] + newSize[i] )
    {
      child = child | ( 1 << i );
      newOrigin[i] += newSize[i];
    }
  }
  cursor->ToChild( child );

  return this->RecursivelyFindPoint( x, cursor, newOrigin, newSize );
}

//----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::FindCell( double x[3],
                                      vtkCell* cell,
                                      vtkGenericCell* gencell,
                                      vtkIdType cellId,
                                      double tol2,
                                      int& subId,
                                      double pcoords[3],
                                      double* weights )
{
  vtkIdType ptId = this->FindPoint( x );
  if ( ptId < 0 )
  {
    // Return invalid Id if point is completely outside of data set
    return -1;
  }

  vtkNew<vtkIdList> cellIds;
  cellIds->Allocate( 8, 100 );
  this->GetPointCells( ptId, cellIds );
  if ( cellIds->GetNumberOfIds() <= 0 )
  {
    return -1;
  }

  double closestPoint[3];
  double dist2;
  vtkIdType num = cellIds->GetNumberOfIds();
  for ( vtkIdType i = 0; i < num; ++ i )
  {
    cellId = cellIds->GetId( i );
    if ( gencell )
    {
      this->GetCell( cellId, gencell );
    }
    else
    {
      cell = this->GetCell( cellId );
    }

    // See whether this cell contains the point
    if ( ( gencell &&
           gencell->EvaluatePosition( x, closestPoint, subId,
                                      pcoords, dist2, weights ) == 1
           && dist2 <= tol2 )  ||
         ( !gencell &&
           cell->EvaluatePosition( x, closestPoint, subId,
                                   pcoords, dist2, weights ) == 1
           && dist2 <= tol2 ) )
    {
      return cellId;
    }
  }

  // This should never happen.
  vtkErrorMacro( "Could not find cell." );
  return -1;
}

//----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::FindCell( double x[3],
                                      vtkCell* cell,
                                      vtkIdType cellId,
                                      double tol2,
                                      int& subId,
                                      double pcoords[3],
                                      double* weights )
{
  return this->FindCell( x, cell, 0, cellId, tol2, subId, pcoords, weights );
}

//----------------------------------------------------------------------------
unsigned long vtkHyperTreeGrid::GetActualMemorySize()
{
  unsigned long size = this->vtkDataSet::GetActualMemorySize();

  // Iterate over all trees in grid
  vtkHyperTreeGridIterator it;
  it.Initialize( this );
  while ( vtkHyperTree* tree = it.GetNextTree() )
  {
    size += tree->GetActualMemorySize();
  }

  // Approximate map memory size
  size += static_cast<unsigned long>( ( this->HyperTrees.size() * sizeof(vtkIdType) * 3 ) / 1024 );

  if ( this->XCoordinates )
  {
    size += this->XCoordinates->GetActualMemorySize();
  }

  if ( this->YCoordinates )
  {
    size += this->YCoordinates->GetActualMemorySize();
  }

  if ( this->ZCoordinates )
  {
    size += this->ZCoordinates->GetActualMemorySize();
  }

  if ( this->Points )
  {
    size += this->Points->GetActualMemorySize();
  }

  if ( this->Connectivity )
  {
    size += this->Connectivity->GetActualMemorySize();
  }

  if ( this->MaterialMask )
  {
    size += this->MaterialMask->GetActualMemorySize();
  }

  if ( this->MaterialMaskIndex )
  {
    size += this->MaterialMaskIndex->GetActualMemorySize();
  }

  return size;
}

//=============================================================================
// Internal arrays used to generate dual grid.  Random access to cells
// requires the cell leaves connectively array which costs memory.
//-----------------------------------------------------------------------------
vtkPoints* vtkHyperTreeGrid::GetPoints()
{
  this->ComputeDualGrid();
  return this->Points;
}

//-----------------------------------------------------------------------------
vtkIdTypeArray* vtkHyperTreeGrid::GetConnectivity()
{
  this->ComputeDualGrid();
  return this->Connectivity;
}

//----------------------------------------------------------------------------
unsigned int vtkHyperTreeGrid::GetShiftedLevelZeroIndex( vtkIdType index,
                                                         int i,
                                                         int j,
                                                         int k )
{
  // Distinguish between two cases depending on indexing order
  return this->TransposedRootIndexing ?
    index + (k +
             j * static_cast<int>(this->GridSize[2]) +
             i * static_cast<int>(this->GridSize[2] * this->GridSize[1]))
    :
    index + (i +
             j * static_cast<int>(this->GridSize[0]) +
             k * static_cast<int>(this->GridSize[0] * this->GridSize[1]));
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetLevelZeroCoordinatesFromIndex( vtkIdType index,
                                                         unsigned int &i,
                                                         unsigned int &j,
                                                         unsigned int &k )
{
  // Distinguish between two cases depending on indexing order
  if ( ! this->TransposedRootIndexing )
  {
    k = index / ( this->GridSize[0] * this->GridSize[1] );
    vtkIdType rk = k * ( this->GridSize[0] * this->GridSize[1] );
    j = ( index - rk ) / this->GridSize[0];
    i = index - ( j * this->GridSize[0] ) - rk;
  }
  else
  {
    i = index / ( this->GridSize[2] * this->GridSize[1] );
    vtkIdType rk = i * ( this->GridSize[2] * this->GridSize[1] );
    j = ( index - rk ) / this->GridSize[2];
    k = index - ( j * this->GridSize[2] ) - rk;
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetIndexFromLevelZeroCoordinates( vtkIdType& index,
                                                         unsigned int i,
                                                         unsigned int j,
                                                         unsigned int k )
{
  // Distinguish between two cases depending on indexing order
  if ( ! this->TransposedRootIndexing )
  {
    index = i + j * this->GridSize[0]
      + k * ( this->GridSize[0] * this->GridSize[1] );
  }
  else
  {
    index = k + j * this->GridSize[2]
      + i * ( this->GridSize[2] * this->GridSize[1] );
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::ComputeDualGrid()
{
  // Check if we can break out early
  if ( this->Points )
  {
    return;
  }

  // Create arrays needed by dual mesh
  this->Points = vtkPoints::New();
  this->Connectivity = vtkIdTypeArray::New();

  // Primal cell centers are dual points
  this->Points->SetNumberOfPoints( this->GetNumberOfVertices() );

  int numVerts = 1 << this->Dimension;
  this->Connectivity->SetNumberOfComponents( numVerts );

  // Initialize grid depth
  vtkIdType gridDepth = 0;

  // Compute and assign scales of all tree roots
  double scale[] = { 1., 1., 1. };

  // Check whether coordinate arrays match grid size
  // If coordinates array are complete, compute all tree scales
  if ( static_cast<int>( this->GridSize[0] ) + 1 == this->XCoordinates->GetNumberOfTuples()
       && static_cast<int>( this->GridSize[1] ) + 1 == this->YCoordinates->GetNumberOfTuples()
       && static_cast<int>( this->GridSize[2] ) + 1 == this->ZCoordinates->GetNumberOfTuples() )
  {
    // Iterate over all hyper trees depending on indexing mode
    std::map<vtkIdType, vtkHyperTree*>::iterator it = this->HyperTrees.begin();
    std::map<vtkIdType, vtkHyperTree*>::iterator endit = this->HyperTrees.end();

    if ( this->TransposedRootIndexing )
    {
      // I-J-K indexing
      for ( unsigned int i = 0; i < this->GridSize[0] && it != endit; ++ i )
      {
        // Compute scale along x-axis
        scale[0] = this->XCoordinates->GetTuple1( i + 1 ) -
          this->XCoordinates->GetTuple1( i );

        for ( unsigned int j = 0; j < this->GridSize[1] && it != endit; ++ j )
        {
          // Compute scale along y-axis
          scale[1] = this->YCoordinates->GetTuple1( j + 1 ) -
            this->YCoordinates->GetTuple1( j );

          for ( unsigned int k = 0; k < this->GridSize[2] && it != endit; ++ k )
          {
            // Retrieve hyper tree
            vtkHyperTree* tree = it->second;
            ++ it;
            if ( ! tree )
            {
              // No tree at this location, skip to next
              continue;
            }

            // Compute scale along z-axis
            scale[2] = this->ZCoordinates->GetTuple1( k + 1 ) -
              this->ZCoordinates->GetTuple1( k );

            // Set tree scale
            tree->SetScale( scale );

            // Update hyper tree grid depth
            vtkIdType treeDepth = tree->GetNumberOfLevels();
            if ( treeDepth > gridDepth )
            {
              gridDepth = treeDepth;
            }
          } // i
        } // j
      } // k
    } // if ( this->TransposedRootIndexing )
    else
    {
      // K-J-I indexing
      for ( unsigned int k = 0; k < this->GridSize[2] && it != endit; ++ k )
      {
        // Compute scale along z-axis
        scale[2] = this->ZCoordinates->GetTuple1( k + 1 ) -
          this->ZCoordinates->GetTuple1( k );

        for ( unsigned int j = 0; j < this->GridSize[1] && it != endit; ++ j )
        {
          // Compute scale along y-axis
          scale[1] = this->YCoordinates->GetTuple1( j + 1 ) -
            this->YCoordinates->GetTuple1( j );

          for ( unsigned int i = 0; i < this->GridSize[0] && it != endit; ++ i )
          {
            // Retrieve hyper tree
            vtkHyperTree* tree = it->second;
            ++ it;
            if ( ! tree )
            {
              // No tree at this location, skip to next
              continue;
            }

            // Compute scale along x-axis
            scale[0] = this->XCoordinates->GetTuple1( i + 1 ) -
              this->XCoordinates->GetTuple1( i );

            // Set tree scale
            tree->SetScale( scale );

            // Update hyper tree grid depth
            vtkIdType treeDepth = tree->GetNumberOfLevels();
            if ( treeDepth > gridDepth )
            {
              gridDepth = treeDepth;
            }
          } // i
        } // j
      } // k
    } // else indexing mode
  } // if coordinate cardinalities match

  // Compute and store reduction factors for speed
  double factor = 1.;
  for ( unsigned short p = 0; p < gridDepth; ++ p )
  {
    this->ReductionFactors[p] = .5 * factor;
    factor /= this->BranchFactor;
  } // p

  // Retrieve material mask
  vtkBitArray* mask = this->HasMaterialMask() ? this->GetMaterialMask() : 0;

  // Iterate over all hyper trees
  vtkIdType index;
  vtkHyperTreeGridIterator it;
  this->InitializeTreeIterator( it );
  while ( it.GetNextTree( index ) )
  {
    // Initialize new Moore cursor at root of current tree
    vtkHyperTreeGridCursor* cursor = this->NewMooreSuperCursor( index );

    // Convert hyper tree into unstructured mesh recursively
    if ( mask )
    {
      this->TraverseDualRecursively( cursor, mask );
    }
    else
    {
      this->TraverseDualRecursively( cursor );
    }

    // Clean up
    cursor->Delete();
  } // it

  // Adjust dual points as needed to fit the primal boundary
  for ( unsigned int d = 0; d < this->Dimension; ++ d )
  {
    // Iterate over all adjustments for current dimension
    for ( std::map<vtkIdType, double>::const_iterator _it
            = this->PointShifts[d].begin();
          _it != this->PointShifts[d].end(); ++ _it )
    {
      double pt[3];
      this->Points->GetPoint( _it->first, pt );
      pt[d] += _it->second;
      this->Points->SetPoint( _it->first, pt );
    } // it
    this->PointShifts[d].clear();
  } // d
  this->PointShifted.clear();
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::TraverseDualRecursively( vtkHyperTreeGridCursor* cursor )
{
  // Create cell corner if cursor is at leaf
  if ( cursor->IsLeaf() )
  {
    // Center is a leaf, create dual items depending on dimension
    switch ( this->Dimension )
    {
      case 1:
        this->GenerateDualCornerFromLeaf1D( cursor );
        break;
      case 2:
        this->GenerateDualCornerFromLeaf2D( cursor );
        break;
      case 3:
        this->GenerateDualCornerFromLeaf3D( cursor );
        break;
    } // switch ( this->Dimension )
  } // if ( cursor->IsLeaf() )
  else
  {
     // Cursor is not at leaf, recurse to all children
    int numChildren = this->NumberOfChildren;
    for ( int child = 0; child < numChildren; ++ child )
    {
      // Create child cursor from parent
      vtkHyperTreeGridCursor* childCursor = cursor->Clone();
      childCursor->ToChild( child );

      // Recurse
      this->TraverseDualRecursively( childCursor );

      // Clean up
      childCursor->Delete();
      childCursor = 0;
    } // child
  } // else
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::TraverseDualRecursively( vtkHyperTreeGridCursor* cursor,
                                                vtkBitArray* mask )
{
  // Create cell corner if cursor is at leaf
  if ( cursor->IsLeaf() )
  {
    // Cursor is at leaf, retrieve its global index
    vtkIdType id = cursor->GetGlobalNodeIndex();

    // Center is a leaf, create dual items depending on dimension
    if ( mask->GetValue( id ) )
    {
      switch ( this->Dimension )
      {
        case 2:
          this->ShiftDualCornerFromMaskedLeaf2D( cursor, mask );
          break;
        case 3:
          this->ShiftDualCornerFromMaskedLeaf3D( cursor, mask );
      } // switch ( this->Dimension )
    }
    else
    {
      switch ( this->Dimension )
      {
        case 1:
          this->GenerateDualCornerFromLeaf1D( cursor );
          break;
        case 2:
          this->GenerateDualCornerFromLeaf2D( cursor, mask );
          break;
        case 3:
          this->GenerateDualCornerFromLeaf3D( cursor, mask );
      } // switch ( this->Dimension )
    } // else
  } // if ( cursor->IsLeaf() )
  else
  {
     // Cursor is not at leaf, recurse to all children
    int numChildren = this->NumberOfChildren;
    for ( int child = 0; child < numChildren; ++ child )
    {
      // Create child cursor from parent
      vtkHyperTreeGridCursor* childCursor = cursor->Clone();
      childCursor->ToChild( child );

      // Recurse
      this->TraverseDualRecursively( childCursor, mask );

      // Clean up
      childCursor->Delete();
      childCursor = 0;
    } // child
  } // else
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GenerateDualCornerFromLeaf1D( vtkHyperTreeGridCursor* cursor )
{
  // With d=1:
  //   (d-0)-faces are corners, neighbor cursors are 0 and 2
  //   (d-1)-faces do not exist
  //   (d-2)-faces do not exist

  // Retrieve neighbor (left/right) cursors
  vtkHyperTreeGridCursor* cursorL = cursor->GetCursor( 0 );
  vtkHyperTreeGridCursor* cursorR = cursor->GetCursor( 2 );

  // Retrieve cursor center coordinates
  double pt[3];
  cursor->GetPoint( pt );

  // Check across d-face neighbors whether point must be adjusted
  if ( ! cursorL->GetTree() )
  {
    // Move to left corner
    pt[this->Orientation] -= .5 * cursor->GetSize()[this->Orientation];;
  }
  if ( ! cursorR->GetTree() )
  {
    // Move to right corner
    pt[this->Orientation] += .5 * cursor->GetSize()[this->Orientation];;
  }

  // Retrieve global index of center cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();

  // Insert dual point at center of leaf cell
  this->Points->SetPoint( id, pt );

  // Storage for edge vertex IDs: dual cell ownership to cursor with higher index
  vtkIdType ids[2];
  ids[0] = id;

  // Check whether a dual edge to left neighbor exists
  if ( cursorL->GetTree() && cursorL->IsLeaf() )
  {
    // If left neighbor is a leaf, always create an edge
    ids[1] = cursorL->GetGlobalNodeIndex();
    this->Connectivity->InsertNextTypedTuple( ids );
  }

  // Check whether a dual edge to right neighbor exists
  if ( ( cursorR->GetTree() && cursorR->IsLeaf() )
       && cursorR->GetLevel() != cursor->GetLevel() )
  {
    // If right neighbor is a leaf, create an edge only if right cell at higher level
    ids[1] = cursorR->GetGlobalNodeIndex();
    this->Connectivity->InsertNextTypedTuple( ids );
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GenerateDualCornerFromLeaf2D( vtkHyperTreeGridCursor* cursor )
{
  // With d=2:
  //   (d-0)-faces are edges, neighbor cursors are 1, 3, 5, 7
  //   (d-1)-faces are corners, neighbor cursors are 0, 2, 6, 8
  //   (d-2)-faces do not exist

  // Retrieve (d-0)-neighbor (south/east/west/north) cursors
  vtkHyperTreeGridCursor* cursorS = cursor->GetCursor( 1 );
  vtkHyperTreeGridCursor* cursorW = cursor->GetCursor( 3 );
  vtkHyperTreeGridCursor* cursorE = cursor->GetCursor( 5 );
  vtkHyperTreeGridCursor* cursorN = cursor->GetCursor( 7 );

  // Retrieve (d-1)-neighbor (southwest/southeast/northwest/northeast) cursors
  vtkHyperTreeGridCursor* cursorSW = cursor->GetCursor( 0 );
  vtkHyperTreeGridCursor* cursorSE = cursor->GetCursor( 2 );
  vtkHyperTreeGridCursor* cursorNW = cursor->GetCursor( 6 );
  vtkHyperTreeGridCursor* cursorNE = cursor->GetCursor( 8 );

  // Retrieve 2D axes (east-west/south-north)
  unsigned int axisWE = this->Orientation ? 0 : 1;
  unsigned int axisSN = this->Orientation == 2 ? 1 : 2;

  // Retrieve cursor center coordinates
  double pt[3];
  cursor->GetPoint( pt );

  // Compute potential shifts
  double shift[2];
  shift[0] = .5 * cursor->GetSize()[axisWE];
  shift[1] = .5 * cursor->GetSize()[axisSN];

  // Check across edge neighbors whether point must be adjusted
  if ( ! cursorS->GetTree() )
  {
    // Move to south edge
    pt[axisSN] -= shift[1];
  }
  if ( ! cursorW->GetTree() )
  {
    // Move to west edge
    pt[axisWE] -= shift[0];
  }
  if ( ! cursorE->GetTree() )
  {
    // Move to east edge
    pt[axisWE] += shift[0];
  }
  if ( ! cursorN->GetTree() )
  {
    // Move to north edge
    pt[axisSN] += shift[1];
  }

  // Retrieve global index of center cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();

  // Insert dual point at center of leaf cell
  this->Points->SetPoint( id, pt );

  // Storage for edge vertex IDs: dual cell ownership to cursor with higher index
  vtkIdType ids[4];
  ids[0] = id;

  // Retrieve current level to break corner ownership ties
  unsigned int level = cursor->GetLevel();

  // Check whether a dual cell around SW corner exists
  if ( cursorSW->GetTree() && cursorSW->IsLeaf()
       && cursorS->GetTree() && cursorS->IsLeaf()
       && cursorW->GetTree() && cursorW->IsLeaf() )
  {
    // If SW, S, and W neighbors are leaves, always create a face
    ids[1] = cursorW->GetGlobalNodeIndex();
    ids[2] = cursorS->GetGlobalNodeIndex();
    ids[3] = cursorSW->GetGlobalNodeIndex();
    this->Connectivity->InsertNextTypedTuple( ids );
  }

  // Check whether a dual cell around SE corner exists
  if ( cursorS->GetTree() && cursorS->IsLeaf()
       && cursorSE->GetTree() && cursorSE->IsLeaf()
       && cursorE->GetTree() && cursorE->IsLeaf()
       && level != cursorE->GetLevel() )
  {
    // If S, SE, and E neighbors are leaves, create a face if E at higher level
    ids[1] = cursorE->GetGlobalNodeIndex();
    ids[2] = cursorS->GetGlobalNodeIndex();
    ids[3] = cursorSE->GetGlobalNodeIndex();
    this->Connectivity->InsertNextTypedTuple( ids );
  }

  // Check whether a dual cell around NE corner exists
  if ( cursorE->GetTree() && cursorE->IsLeaf()
       && cursorNE->GetTree() && cursorNE->IsLeaf()
       && cursorN->GetTree() && cursorN->IsLeaf()
       && level != cursorE->GetLevel()
       && level != cursorNE->GetLevel()
       && level != cursorN->GetLevel() )
  {
    // If E, NE, and N neighbors are leaves, create a face if E, NE, N at higher level
    ids[1] = cursorE->GetGlobalNodeIndex();
    ids[2] = cursorN->GetGlobalNodeIndex();
    ids[3] = cursorNE->GetGlobalNodeIndex();;
    this->Connectivity->InsertNextTypedTuple( ids );
  }

  // Check whether a dual cell around NW corner exists
  if ( cursorW->GetTree() && cursorW->IsLeaf()
       && cursorN->GetTree() && cursorN->IsLeaf()
       && cursorNW->GetTree() && cursorNW->IsLeaf()
       && level != cursorNW->GetLevel()
       && level != cursorN->GetLevel() )
  {
    // If W, N, and NW neighbors are leaves, create a face if NW and N at higher level
    ids[1] = cursorW->GetGlobalNodeIndex();
    ids[2] = cursorN->GetGlobalNodeIndex();
    ids[3] = cursorNW->GetGlobalNodeIndex();
    this->Connectivity->InsertNextTypedTuple( ids );
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GenerateDualCornerFromLeaf2D( vtkHyperTreeGridCursor* cursor,
                                                     vtkBitArray* mask )
{
  // With d=2:
  //   (d-0)-faces are edges, neighbor cursors are 1, 3, 5, 7
  //   (d-1)-faces are corners, neighbor cursors are 0, 2, 6, 8
  //   (d-2)-faces do not exist

  // Retrieve (d-0)-neighbor (south/east/west/north) cursors
  vtkHyperTreeGridCursor* cursorS = cursor->GetCursor( 1 );
  vtkHyperTreeGridCursor* cursorW = cursor->GetCursor( 3 );
  vtkHyperTreeGridCursor* cursorE = cursor->GetCursor( 5 );
  vtkHyperTreeGridCursor* cursorN = cursor->GetCursor( 7 );

  // Retrieve (d-1)-neighbor (southwest/southeast/northwest/northeast) cursors
  vtkHyperTreeGridCursor* cursorSW = cursor->GetCursor( 0 );
  vtkHyperTreeGridCursor* cursorSE = cursor->GetCursor( 2 );
  vtkHyperTreeGridCursor* cursorNW = cursor->GetCursor( 6 );
  vtkHyperTreeGridCursor* cursorNE = cursor->GetCursor( 8 );

  // Retrieve global indices of non-center cursors
  vtkIdType idS = cursorS->GetGlobalNodeIndex();
  vtkIdType idW = cursorW->GetGlobalNodeIndex();
  vtkIdType idE = cursorE->GetGlobalNodeIndex();
  vtkIdType idN = cursorN->GetGlobalNodeIndex();
  vtkIdType idSW = cursorSW->GetGlobalNodeIndex();
  vtkIdType idSE = cursorSE->GetGlobalNodeIndex();
  vtkIdType idNW = cursorNW->GetGlobalNodeIndex();
  vtkIdType idNE = cursorNE->GetGlobalNodeIndex();

  // Retrieve corresponding maskings
  bool maskedS = mask->GetValue( idS ) ? true : false;
  bool maskedW = mask->GetValue( idW ) ? true : false;
  bool maskedE = mask->GetValue( idE ) ? true : false;
  bool maskedN = mask->GetValue( idN ) ? true : false;
  bool maskedSW = mask->GetValue( idSW ) ? true : false;
  bool maskedSE = mask->GetValue( idSE ) ? true : false;
  bool maskedNW = mask->GetValue( idNW ) ? true : false;
  bool maskedNE = mask->GetValue( idNE ) ? true : false;

  // Retrieve 2D axes (east-west/south-north)
  unsigned int axisWE = this->Orientation ? 0 : 1;
  unsigned int axisSN = this->Orientation == 2 ? 1 : 2;

  // Retrieve cursor center coordinates
  double pt[3];
  cursor->GetPoint( pt );

  // Compute potential shifts
  double shift[2];
  shift[0] = .5 * cursor->GetSize()[axisWE];
  shift[1] = .5 * cursor->GetSize()[axisSN];

  // When a mask is present, edge as well as face shifts are possible
  bool shifted = false;

  // Check across edge neighbors whether point must be adjusted
  if ( ! cursorS->GetTree() || ( cursorS->IsLeaf() && maskedS ) )

  {
    // Move to south edge
    pt[axisSN] -= shift[1];
    shifted = true;
  }
  if ( ! cursorW->GetTree() || ( cursorW->IsLeaf() && maskedW ) )
  {
    // Move to west edge
    pt[axisWE] -= shift[0];
    shifted = true;
  }
  if ( ! cursorE->GetTree() || ( cursorE->IsLeaf() && maskedE ) )
  {
    // Move to east edge
    pt[axisWE] += shift[0];
    shifted = true;
  }
  if ( ! cursorN->GetTree() || ( cursorN->IsLeaf() && maskedN ) )
  {
    // Move to north edge
    pt[axisSN] += shift[1];
    shifted = true;
  }

  // Only when point was not moved to edge, check corner neighbors
  if ( ! shifted )
  {
     if ( ! cursorSW->GetTree() || ( cursorSW->IsLeaf() && maskedSW ) )
     {
       // Move to southwest corner
       pt[axisWE] -= shift[0];
       pt[axisSN] -= shift[1];
     }
     if ( ! cursorSE->GetTree() || ( cursorSE->IsLeaf() && maskedSE ) )
     {
       // Move to southeast corner
       pt[axisWE] += shift[0];
       pt[axisSN] -= shift[1];
     }
     if ( ! cursorNW->GetTree() || ( cursorNW->IsLeaf() && maskedNW ) )
     {
       // Move to northwest corner
       pt[axisWE] -= shift[0];
       pt[axisSN] += shift[1];
     }
     if ( ! cursorNE->GetTree() || ( cursorNE->IsLeaf() && maskedNE ) )
     {
       // Move to northeast corner
       pt[axisWE] += shift[0];
       pt[axisSN] += shift[1];
     }
  } // if ( ! shifted )

  // Retrieve global index of center cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();

  // Insert dual point at center of leaf cell
  this->Points->SetPoint( id, pt );

  // If cell is masked, terminate recursion, no dual cell will be generated
  if ( mask->GetValue( id ) )
  {
    return;
  }

   // Storage for edge vertex IDs: dual cell ownership to cursor with higher index
  vtkIdType ids[4];
  ids[0] = id;

  // Retrieve current level to break corner ownership ties
  unsigned int level = cursor->GetLevel();

  // Check whether a dual cell around SW corner exists
  if ( cursorSW->GetTree() && cursorSW->IsLeaf()
       && cursorS->GetTree() && cursorS->IsLeaf()
       && cursorW->GetTree() && cursorW->IsLeaf()
       && ! maskedSW && ! maskedS && ! maskedW )
  {
    // If SW, S, and W neighbors are leaves, always create a face
    ids[1] = idW;
    ids[2] = idS;
    ids[3] = idSW;
    this->Connectivity->InsertNextTypedTuple( ids );
  }

  // Check whether a dual cell around SE corner exists
  if ( cursorS->GetTree() && cursorS->IsLeaf()
       && cursorSE->GetTree() && cursorSE->IsLeaf()
       && cursorE->GetTree() && cursorE->IsLeaf()
       && ! maskedS && ! maskedSE && ! maskedE
       && level != cursorE->GetLevel() )
  {
    // If S, SE, and E neighbors are leaves, create a face if E at higher level
    ids[1] = idE;
    ids[2] = idS;
    ids[3] = idSE;
    this->Connectivity->InsertNextTypedTuple( ids );
  }

  // Check whether a dual cell around NE corner exists
  if ( cursorE->GetTree() && cursorE->IsLeaf()
       && cursorNE->GetTree() && cursorNE->IsLeaf()
       && cursorN->GetTree() && cursorN->IsLeaf()
       && ! maskedE && ! maskedNE && ! maskedN
       && level != cursorE->GetLevel()
       && level != cursorNE->GetLevel()
       && level != cursorN->GetLevel() )
  {
    // If E, NE, and N neighbors are leaves, create a face if E, NE, N at higher level
    ids[1] = idE;
    ids[2] = idN;
    ids[3] = idNE;
    this->Connectivity->InsertNextTypedTuple( ids );
  }

  // Check whether a dual cell around NW corner exists
  if ( cursorW->GetTree() && cursorW->IsLeaf()
       && cursorN->GetTree() && cursorN->IsLeaf()
       && cursorNW->GetTree() && cursorNW->IsLeaf()
       && ! maskedW && ! maskedN && ! maskedNW
       && level != cursorNW->GetLevel()
       && level != cursorN->GetLevel() )
  {
    // If W, N, and NW neighbors are leaves, create a face if NW and N at higher level
    ids[1] = idW;
    ids[2] = idN;
    ids[3] = idNW;
    this->Connectivity->InsertNextTypedTuple( ids );
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GenerateDualCornerFromLeaf3D( vtkHyperTreeGridCursor* cursor )
{
  // With d=3:
  //   (d-0)-faces are faces, neighbor cursors are 4, 10, 12, 14, 16, 22
  //   (d-1)-faces are edges, neighbor cursors are 1, 3, 5, 7, 9, 11, 15, 17, 19, 21, 23, 25
  //   (d-2)-faces are corners, neighbor cursors are 0, 2, 6, 8, 18, 20, 24, 26

  // Retrieve cursors
  vtkHyperTreeGridCursor* cursors[27];
  for ( unsigned int c = 0; c < 27; ++ c )
  {
    cursors[c] = cursor->GetCursor( c );
  }

  // Retrieve cursor center coordinates
  double pt[3];
  cursor->GetPoint( pt );

  // Compute potential shifts
  double shift[3];
  shift[0] = .5 * cursor->GetSize()[0];
  shift[1] = .5 * cursor->GetSize()[1];
  shift[2] = .5 * cursor->GetSize()[2];

  // Index offset relative to center cursor (13)
  unsigned int offset = 1;

  // Check across face neighbors whether point must be adjusted
  for ( unsigned int axis = 0; axis < 3; ++ axis, offset *= 3 )
  {
    if ( ! cursors[13 - offset]->GetTree() )
    {
      // Move to negative side along axis
      pt[axis] -= shift[axis];
    }
    if ( ! cursors[13 + offset]->GetTree() )
    {
      // Move to positive side along axis
      pt[axis] += shift[axis];
    }
  } // axis

  // Retrieve global index of center cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();

  // Insert dual point at center of leaf cell
  this->Points->SetPoint( id, pt );

  // Storage for edge vertex IDs: dual cell ownership to cursor with higher index
  vtkIdType ids[8];

  // Retrieve current level to break corner ownership ties
  unsigned int level = cursor->GetLevel();

  // Iterate over leaf corners
  for ( unsigned int c = 0; c < 8; ++ c )
  {
    // Assume center cursor leaf owns the corner
    bool owner = true;

    // Iterate over every leaf touching the corner
    for ( unsigned int l = 0; l < 8 && owner; ++ l )
    {
      // Retrieve cursor index of touching leaf
      unsigned int index = CornerNeighborCursorsTable3D[c][l];

      // Collect the leaf indices for the dual cell
      ids[l] = cursors[index]->GetGlobalNodeIndex();

      // Compute whether corner is owned by another leaf
      if ( index != 13 )
      {
        vtkHyperTreeGridCursor* cursorL = cursors[index];
        if ( ! cursorL->GetTree() || ! cursorL->IsLeaf()
             ||
             ( cursorL->GetLevel() == level && index > 13 ) )
        {
          // If neighbor leaf is out of bounds or has not been
          // refined to a leaf, this leaf does not own the corner
          // A level tie is broken in favor of the largest index
          owner = false;
        }
      } // if ( index != 13 )
    } // l

    // If leaf owns the corner, create dual cell
    if ( owner )
    {
      this->Connectivity->InsertNextTypedTuple( ids );
    } // if ( owner )
  } // c
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GenerateDualCornerFromLeaf3D( vtkHyperTreeGridCursor* cursor,
                                                     vtkBitArray* mask )
{
  // With d=3:
  //   (d-0)-faces are faces, neighbor cursors are 4, 10, 12, 14, 16, 22
  //   (d-1)-faces are edges, neighbor cursors are 1, 3, 5, 7, 9, 11, 15, 17, 19, 21, 23, 25
  //   (d-2)-faces are corners, neighbor cursors are 0, 2, 6, 8, 18, 20, 24, 26

  // Retrieve cursor center coordinates
  double pt[3];
  cursor->GetPoint( pt );

  // Compute potential shifts
  double shift[3];
  shift[0] = .5 * cursor->GetSize()[0];
  shift[1] = .5 * cursor->GetSize()[1];
  shift[2] = .5 * cursor->GetSize()[2];

  // Retrieve global indices of cursors in super cursor
  vtkIdType globalIDs[27];
  for ( unsigned int c = 0; c < 27; ++ c )
  {
    globalIDs[c] = cursor->GetCursor( c )->GetGlobalNodeIndex();
  }

  // When a mask is present, corner, edge, and face shifts are possible
  bool shifted = false;

  // Check across face neighbors whether point must be adjusted
  unsigned int offset = 1;
  for ( unsigned int axis = 0; axis < 3; ++ axis, offset *= 3 )
  {
    vtkHyperTreeGridCursor* cursorM = cursor->GetCursor( 13 - offset );
    vtkIdType idM = cursorM->GetGlobalNodeIndex();
    if ( ! cursorM->GetTree() || ( cursorM->IsLeaf() && mask->GetValue( idM ) ) )
    {
      // Move to negative side along axis
      pt[axis] -= shift[axis];
      shifted = true;
    }
    vtkHyperTreeGridCursor* cursorP = cursor->GetCursor( 13 + offset );
    vtkIdType idP = cursorP->GetGlobalNodeIndex();
    if ( ! cursorP->GetTree() || ( cursorP->IsLeaf() && mask->GetValue( idP ) ) )
    {
      // Move to positive side along axis
      pt[axis] += shift[axis];
      shifted = true;
    }
  } // axis

  // Only when point was not moved to face, check edge neighbors
  if ( ! shifted )
  {
     int i = 1;
     for ( int axis1 = 0; axis1 < 2; ++ axis1, i *= 3 )
     {
       int j = 3 * i;
       for ( int axis2 = axis1 + 1; axis2 < 3; ++ axis2, j *= 3 )
       {
         for ( int o2 = -1; o2 < 2; o2 += 2 )
         {
           for ( int o1 = -1; o1 < 2; o1 += 2 )
           {
             int index = 13 + o1 * ( i * o2 + j );
             vtkHyperTreeGridCursor* cursorE = cursor->GetCursor(static_cast<unsigned int>(index) );
             vtkIdType idE = cursorE->GetGlobalNodeIndex();
             if ( ! cursorE->GetTree() || ( cursorE->IsLeaf() && mask->GetValue( idE ) ) )
             {
               // Move to corresponding edge
               pt[axis1] += o1 * o2 * shift[axis1];
               pt[axis2] += o1 * shift[axis2];
               shifted = true;
             }
           } // o1
         } // o2
       } // axis2
     } // axis1
  } // if ( ! shifted )

  // Only when point was neither moved to face nor to edge, check corners neighbors
  if ( ! shifted )
  {
    // Iterate over all 8 corners
    for ( int o3 = -1; o3 < 2; o3 += 2  )
    {
      for ( int o2 = -1; o2 < 2; o2 += 2 )
      {
        offset = o2 * ( o3 + 3 ) + 9;
        for ( int o1 = -1; o1 < 2; o1 += 2 )
        {
          int index = 13 + o1 * static_cast<int>( offset );
          vtkHyperTreeGridCursor* cursorC = cursor->GetCursor( static_cast<unsigned int>(index) );
          vtkIdType idC = cursorC->GetGlobalNodeIndex();
          if ( ! cursorC->GetTree() || ( cursorC->IsLeaf() && mask->GetValue( idC ) ) )
          {
            // Move to corresponding corner
            pt[0] += o1 * o2 * o3 * shift[0];
            pt[1] += o1 * o2 * shift[1];
            pt[2] += o1 * shift[2];
          } // if cursor
        } // o1
      } // o2
    } // o3
  } // if ( ! shifted )

  // Retrieve global index of center cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();

  // Insert dual point at center of leaf cell
  this->Points->SetPoint( id, pt );

  // Storage for edge vertex IDs: dual cell ownership to cursor with higher index
  vtkIdType ids[8];

  // Retrieve current level to break corner ownership ties
  unsigned int level = cursor->GetLevel();

  // Iterate over leaf corners
  for ( unsigned int c = 0; c < 8; ++ c )
  {
    // Assume center cursor leaf owns the corner
    bool owner = true;

    // Iterate over every leaf touching the corner
    for ( unsigned int l = 0; l < 8 && owner; ++ l )
    {
      // Retrieve cursor index of touching leaf
      unsigned int index = CornerNeighborCursorsTable3D[c][l];

      // Collect the leaf indices for the dual cell
      ids[l] = globalIDs[index];

      // Compute whether corner is owned by another leaf
      if ( index != 13 )
      {
        vtkHyperTreeGridCursor* cursorL = cursor->GetCursor( index );
        if ( ! cursorL->GetTree() || ! cursorL->IsLeaf()
             ||
             ( cursorL->GetLevel() == level && index > 13 )
             ||
             mask->GetValue( cursorL->GetGlobalNodeIndex() ) )
        {
          // If neighbor leaf is out of bounds or has not been
          // refined to a leaf, this leaf does not own the corner
          // A level tie is broken in favor of the largest index
          owner = false;
        }
      } // if ( index != 13 )
    } // l

    // If leaf owns the corner, create dual cell
    if ( owner )
    {
      this->Connectivity->InsertNextTypedTuple( ids );
    } // if ( owner )
  } // c
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::ShiftDualCornerFromMaskedLeaf2D( vtkHyperTreeGridCursor* cursor,
                                                        vtkBitArray* mask )
{
  // With d=2:
  //   (d-0)-faces are edges, neighbor cursors are 1, 3, 5, 7
  //   (d-1)-faces are corners, neighbor cursors are 0, 2, 6, 8
  //   (d-2)-faces do not exist

  // Retrieve (d-0)-neighbor (south/east/west/north) cursors
  vtkHyperTreeGridCursor* cursorS = cursor->GetCursor( 1 );
  vtkHyperTreeGridCursor* cursorW = cursor->GetCursor( 3 );
  vtkHyperTreeGridCursor* cursorE = cursor->GetCursor( 5 );
  vtkHyperTreeGridCursor* cursorN = cursor->GetCursor( 7 );

  // Retrieve (d-1)-neighbor (southwest/southeast/northwest/northeast) cursors
  vtkHyperTreeGridCursor* cursorSW = cursor->GetCursor( 0 );
  vtkHyperTreeGridCursor* cursorSE = cursor->GetCursor( 2 );
  vtkHyperTreeGridCursor* cursorNW = cursor->GetCursor( 6 );
  vtkHyperTreeGridCursor* cursorNE = cursor->GetCursor( 8 );

  // Retrieve global indices of non-center cursors
  vtkIdType idS = cursorS->GetGlobalNodeIndex();
  vtkIdType idW = cursorW->GetGlobalNodeIndex();
  vtkIdType idE = cursorE->GetGlobalNodeIndex();
  vtkIdType idN = cursorN->GetGlobalNodeIndex();
  vtkIdType idSW = cursorSW->GetGlobalNodeIndex();
  vtkIdType idSE = cursorSE->GetGlobalNodeIndex();
  vtkIdType idNW = cursorNW->GetGlobalNodeIndex();
  vtkIdType idNE = cursorNE->GetGlobalNodeIndex();

  // Retrieve 2D axes (east-west/south-north)
  unsigned int axisWE = this->Orientation ? 0 : 1;
  unsigned int axisSN = this->Orientation == 2 ? 1 : 2;

  // Retrieve current level to break corner ownership ties
  unsigned int level = cursor->GetLevel();

  // Check whether dual point across S edge must be adjusted
  if ( cursorS->GetTree() && cursorS->IsLeaf()
       && cursorS->GetLevel() < level
       && ! mask->GetValue( idS ) )
  {
    // Dual point must be adusted
    this->PointShifted[idS] = true;
    this->PointShifts[axisSN][idS] = cursorS->GetTree()->GetScale( axisSN )
      * this->ReductionFactors[cursorS->GetLevel()];
  }

  // Check whether dual point across W edge must be adjusted
  if ( cursorW->GetTree() && cursorW->IsLeaf()
       && cursorW->GetLevel() < level
       && ! mask->GetValue( idW ) )
  {
    // Dual point must be adusted
    this->PointShifted[idW] = true;
    this->PointShifts[axisWE][idW] = cursorW->GetTree()->GetScale( axisWE )
      * this->ReductionFactors[cursorW->GetLevel()];
  }

  // Check whether dual point across E face must be adjusted
  if ( cursorE->GetTree() && cursorE->IsLeaf()
       && cursorE->GetLevel() < level
       && ! mask->GetValue( idE ) )
  {
    // Dual point must be adusted
    this->PointShifted[idE] = true;
    this->PointShifts[axisWE][idE] = - cursorE->GetTree()->GetScale( axisWE )
      * this->ReductionFactors[cursorE->GetLevel()];
  }

  // Check whether dual point across N edge must be adjusted
  if ( cursorN->GetTree() && cursorN->IsLeaf()
       && cursorN->GetLevel() < level
       && ! mask->GetValue( idN ) )
  {
    // Dual point must be adusted
    this->PointShifted[idN] = true;
    this->PointShifts[axisSN][idN] = - cursorN->GetTree()->GetScale( axisSN )
      * this->ReductionFactors[cursorN->GetLevel()];
  }

  // Check whether dual point across SE corner must be adjusted
  if ( cursorSE->GetTree() && cursorSE->IsLeaf()
       && cursorSE->GetLevel() < level
       && ! mask->GetValue( idSE )
       && ! this->PointShifted[idSE] )
  {
    // Dual point must be adusted
    double shift[3];
    cursorSE->GetTree()->GetScale( shift );
    double factor = this->ReductionFactors[cursorSE->GetLevel()];
    this->PointShifts[axisWE][idSE] = factor * shift[axisWE];
    this->PointShifts[axisSN][idSE] = factor * shift[axisSN];
  }

  // Check whether dual point across SW corner must be adjusted
  if ( cursorSW->GetTree() && cursorSW->IsLeaf()
       && cursorSW->GetLevel() < level
       && ! mask->GetValue( idSW )
       && ! this->PointShifted[idSW] )
  {
    // Dual point must be adusted
    double shift[3];
    cursorSW->GetTree()->GetScale( shift );
    double factor = this->ReductionFactors[cursorSW->GetLevel()];
    this->PointShifts[axisWE][idSW] = - factor * shift[axisWE];
    this->PointShifts[axisSN][idSW] = factor * shift[axisSN];
  }

  // Check whether dual point across NW corner must be adjusted
  if ( cursorNW->GetTree() && cursorNW->IsLeaf()
       && cursorNW->GetLevel() < level
       && ! mask->GetValue( idNW )
       && ! this->PointShifted[idNW] )
  {
    // Dual point must be adusted
    double shift[3];
    cursorNW->GetTree()->GetScale( shift );
    double factor = this->ReductionFactors[cursorNW->GetLevel()];
    this->PointShifts[axisWE][idNW] = factor * shift[axisWE];
    this->PointShifts[axisSN][idNW] = - factor * shift[axisSN];
  }

  // Check whether dual point across NE corner must be adjusted
  if ( cursorNE->GetTree() && cursorNE->IsLeaf()
       && cursorNE->GetLevel() < level
       && ! mask->GetValue( idNE )
       && ! this->PointShifted[idNE] )
  {
    // Dual point must be adusted
    double shift[3];
    cursorNE->GetTree()->GetScale( shift );
    double factor = this->ReductionFactors[cursorNE->GetLevel()];
    this->PointShifts[axisWE][idNE] = - factor * shift[axisWE];
    this->PointShifts[axisSN][idNE] = - factor * shift[axisSN];
  }

}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::ShiftDualCornerFromMaskedLeaf3D( vtkHyperTreeGridCursor* cursor,
                                                        vtkBitArray* mask )
{
  // With d=3:
  //   (d-0)-faces are faces, neighbor cursors are 4, 10, 12, 14, 16, 22
  //   (d-1)-faces are edges, neighbor cursors are 1, 3, 5, 7, 9, 11, 15, 17, 19, 21, 23, 25
  //   (d-2)-faces are corners, neighbor cursors are 0, 2, 6, 8, 18, 20, 24, 26

  // Retrieve current level to break corner ownership ties
  unsigned int level = cursor->GetLevel();

  // Check whether dual points across face neighbors must be adjusted
  int offset = 1;
  for ( unsigned int axis = 0; axis < 3; ++ axis, offset *= 3 )
  {
    vtkHyperTreeGridCursor* cursorM = cursor->GetCursor( 13 - offset );
    vtkIdType idM = cursorM->GetGlobalNodeIndex();
    if ( cursorM->GetTree() && cursorM->IsLeaf()
         && cursorM->GetLevel() < level && ! mask->GetValue( idM ) )
    {
      // Dual point must be adjusted
      this->PointShifted[idM] = true;
      this->PointShifts[axis][idM] = cursorM->GetTree()->GetScale( axis )
        * this->ReductionFactors[cursorM->GetLevel()];
    }
    vtkHyperTreeGridCursor* cursorP = cursor->GetCursor( 13 + offset );
    vtkIdType idP = cursorP->GetGlobalNodeIndex();
    if ( cursorP->GetTree() && cursorP->IsLeaf()
         && cursorP->GetLevel() < level && ! mask->GetValue( idP ) )
    {
      // Dual point must be adjusted
      this->PointShifted[idP] = true;
      this->PointShifts[axis][idP] = - cursorP->GetTree()->GetScale( axis )
        * this->ReductionFactors[cursorP->GetLevel()];
    }
  } // axis

  // Check whether dual points across edge neighbors must be adjusted
  int i = 1;
  for ( int axis1 = 0; axis1 < 2; ++ axis1, i *= 3 )
  {
    int j = 3 * i;
    for ( int axis2 = axis1 + 1; axis2 < 3; ++ axis2, j *= 3 )
    {
      for ( int o2 = -1; o2 < 2; o2 += 2 )
      {
        for ( int o1 = -1; o1 < 2; o1 += 2 )
        {
          int index = 13 + o1 * ( i * o2 + j );
          vtkHyperTreeGridCursor* cursorE = cursor->GetCursor( static_cast<unsigned int>(index) );
          vtkIdType idE = cursorE->GetGlobalNodeIndex();
          if ( cursorE->GetTree() && cursorE->IsLeaf()
               && cursorE->GetLevel() < level && ! mask->GetValue( idE )
               && ! this->PointShifted[idE] )
          {
            // Dual point must be adusted
            this->PointShifted[idE] = true;
            double shift[3];
            cursorE->GetTree()->GetScale( shift );
            double factor = this->ReductionFactors[cursorE->GetLevel()];
            this->PointShifts[axis1][idE] = - o1 * o2 * factor * shift[axis1];
            this->PointShifts[axis2][idE] = - o1 * factor * shift[axis2];
          }
        } // o1
      } // o2
    } // axis2
  } // axis1

  // Check whether dual points across corner neighbors must be adjusted
  for ( int o3 = -1; o3 < 2; o3 += 2  )
  {
    for ( int o2 = -1; o2 < 2; o2 += 2 )
    {
      offset = o2 * ( o3 + 3 ) + 9;
      for ( int o1 = -1; o1 < 2; o1 += 2 )
      {
        int index = 13 + o1 * offset;
        vtkHyperTreeGridCursor* cursorC = cursor->GetCursor( index );
        vtkIdType idC = cursorC->GetGlobalNodeIndex();
        if ( cursorC->GetTree() && cursorC->IsLeaf()
             && cursorC->GetLevel() < level && ! mask->GetValue( idC )
             && ! this->PointShifted[idC] )
        {
          // Dual point must be adusted
          this->PointShifted[idC] = true;
          double shift[3];
          cursorC->GetTree()->GetScale( shift );
          double factor = this->ReductionFactors[cursorC->GetLevel()];
          this->PointShifts[0][idC] = - o1 * o2 * o3 * factor * shift[0];
          this->PointShifts[1][idC] = - o1 * o2 * factor * shift[1];
          this->PointShifts[2][idC] = - o1 * factor * shift[2];
        }
      } // o1
    } // o2
  } // o3
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::ResetDual()
{
  if ( this->Points )
  {
    this->Points->Delete();
    this->Points = nullptr;
  }
  if ( this->Connectivity )
  {
    this->Connectivity->Delete();
    this->Connectivity = nullptr;
  }
  if ( this->Links )
  {
    this->Links->Delete();
    this->Links = nullptr;
  }
}

//=============================================================================
// Hyper tree grid iterator
// Implemented here because it needs access to the internal classes.
//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::vtkHyperTreeGridIterator::Initialize( vtkHyperTreeGrid* tree )
{
  this->Tree = tree;
  this->Iterator = tree->HyperTrees.begin();
}

//-----------------------------------------------------------------------------
vtkHyperTree* vtkHyperTreeGrid::vtkHyperTreeGridIterator::GetNextTree( vtkIdType &index )
{
  if ( this->Iterator == this->Tree->HyperTrees.end() )
  {
    return nullptr;
  }
  vtkHyperTree* t = this->Iterator->second;
  index = this->Iterator->first;
  ++ this->Iterator;
  return t;
}

//-----------------------------------------------------------------------------
vtkHyperTree* vtkHyperTreeGrid::vtkHyperTreeGridIterator::GetNextTree()
{
  vtkIdType index;
  return GetNextTree( index );
}

//=============================================================================
// Hard-coded child mask bitcodes
static const unsigned int HyperTreeGridMask_1_2[2] = { 0x80000000, 0x20000000 };

static const unsigned int HyperTreeGridMask_1_3[3] = { 0x80000000, 0x40000000, 0x20000000 };

static const unsigned int HyperTreeGridMask_2_2[4] = { 0xd0000000, 0x64000000,
                                                       0x13000000, 0x05800000 };

static const unsigned int HyperTreeGridMask_2_3[9] = { 0xd0000000, 0x40000000, 0x64000000,
                                                       0x10000000, 0x08000000, 0x04000000,
                                                       0x13000000, 0x01000000, 0x05800000 };

static const unsigned int HyperTreeGridMask_3_2[8] = { 0xd8680000, 0x6c320000,
                                                       0x1b098000, 0x0d82c000,
                                                       0x00683600, 0x00321b00,
                                                       0x000986c0, 0x0002c360 };

static const unsigned int HyperTreeGridMask_3_3[27] = { 0xd8680000, 0x48200000, 0x6c320000,
                                                        0x18080000, 0x08000000, 0x0c020000,
                                                        0x1b098000, 0x09008000, 0x0d82c000,
                                                        0x00680000, 0x00200000, 0x00320000,
                                                        0x00080000, 0x00040000, 0x00020000,
                                                        0x00098000, 0x00008000, 0x0002c000,
                                                        0x00683600, 0x00201200, 0x00321b00,
                                                        0x00080600, 0x00000200, 0x00020300,
                                                        0x000986c0, 0x00008240, 0x0002c360 };

static const unsigned int* HyperTreeGridMask[3][2]={ {HyperTreeGridMask_1_2,
                                                      HyperTreeGridMask_1_3},
                                                     {HyperTreeGridMask_2_2,
                                                      HyperTreeGridMask_2_3},
                                                     {HyperTreeGridMask_3_2,
                                                      HyperTreeGridMask_3_3} };

//-----------------------------------------------------------------------------
unsigned int vtkHyperTreeGrid::GetChildMask( unsigned int child )
{
  int i = this->GetDimension() - 1;
  int j = this->GetBranchFactor() - 2;
  return HyperTreeGridMask[i][j][child];
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::SetOrientation( unsigned int i )
{
  if (this->Orientation != (i>2?2:i))
  {
    this->Orientation = (i>2?2:i);
    this->Modified();
  }
}
//=============================================================================
