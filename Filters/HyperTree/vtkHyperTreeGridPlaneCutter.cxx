/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridPlaneCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridPlaneCutter.h"

#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCleanPolyData.h"
#include "vtkCutter.h"
#include "vtkDataObject.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridCursor.h"
#include "vtkInformation.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

vtkIdType First8Integers[] = {
  0, 1, 2, 3, 4, 5, 6, 7 };

static const unsigned int MooreCursors3D[26] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26 };

vtkStandardNewMacro(vtkHyperTreeGridPlaneCutter);

//-----------------------------------------------------------------------------
vtkHyperTreeGridPlaneCutter::vtkHyperTreeGridPlaneCutter()
{
  // Create storage for intersections of the cut
  this->Points = vtkPoints::New();

  // Create storage for untructured cells
  this->Cells = vtkCellArray::New();

  // Initialize plane parameters
  std::fill( this->Plane, this->Plane + 4, 0. );

  // By default a non-conforming output mesh is produced for better rendering
  this->Dual = 0;

  // By default member variables for dual-based computation are not used
  this->SelectedCells = nullptr;
  this->Centers = nullptr;
  this->Cutter = nullptr;
  this->Leaves = nullptr;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridPlaneCutter::~vtkHyperTreeGridPlaneCutter()
{
  if ( this->Points )
  {
    this->Points->Delete();
    this->Points = nullptr;
  }

  if ( this->Cells )
  {
    this->Cells->Delete();
    this->Cells = nullptr;
  }

  if ( this->Leaves )
  {
    this->Leaves->Delete();
    this->Leaves = nullptr;
  }

  if ( this->Centers )
  {
    this->Centers->Delete();
    this->Centers = nullptr;
  }

  if ( this->Cutter )
  {
    this->Cutter->Delete();
    this->Cutter = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridPlaneCutter::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "Plane: ( "
     << this->Plane[0] << " ) * X + ( "
     << this->Plane[1] << " ) * Y + ( "
     << this->Plane[2] << " ) * Z = "
     << this->Plane[3] << "\n";

  if ( this->Dual )
  {
    os << indent << "Dual: Yes\n";
  }
  else
  {
    os << indent << "Dual: No\n";
  }

  if ( this->Points )
  {
    os << indent << "Points:\n";
    this->Points->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "Points: ( none )\n";
  }

  if ( this->Cells )
  {
    os << indent << "Cells:\n";
    this->Cells->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "Cells: ( none )\n";
  }

  if ( this->Leaves )
  {
    os << indent << "Leaves:\n";
    this->Leaves->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "Leaves: ( none )\n";
  }

  if ( this->Centers )
  {
    os << indent << "Centers:\n";
    this->Centers->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "Centers: ( none )\n";
  }

  if ( this->Cutter )
  {
    os << indent << "Cutter:\n";
    this->Cutter->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "Cutter: ( none )\n";
  }
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridPlaneCutter::FillOutputPortInformation( int,
                                                            vtkInformation* info )
{
  info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData" );
  return 1;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridPlaneCutter::ProcessTrees( vtkHyperTreeGrid* input,
                                               vtkDataObject* outputDO )
{
  // Downcast output data object to polygonal data set
  vtkPolyData* output = vtkPolyData::SafeDownCast( outputDO );
  if ( ! output )
  {
    vtkErrorMacro( "Incorrect type of output: " << outputDO->GetClassName() );
    return 0;
  }

  // This filter works only with 3D grids
  if ( input->GetDimension() != 3 )
  {
    vtkErrorMacro (<< "Bad input dimension:"
                   << input->GetDimension());
    return 0;
  }

  // Retrieve input point data
  this->InData  = input->GetPointData();

  // Retrieve material mask
  vtkBitArray* mask = input->HasMaterialMask() ? input->GetMaterialMask() : 0;

  // Compute cut on dual or primal input depending on specification
  if ( this->Dual )
  {
    // Initialize output point data
    this->OutData = output->GetPointData();
    this->OutData->CopyAllocate( this->InData );

    // Storage for leaf indices
    this->Leaves = vtkIdList::New();
    this->Leaves->SetNumberOfIds( 8 );

    // Initialize storage for dual geometry
    this->Centers = vtkPoints::New();
    this->Centers->SetNumberOfPoints( 8 );

    // Convert plane parameters into normal/origin specification
    unsigned int maxId = 0;
    if ( fabs( this->Plane[1] ) > fabs( this->Plane[0] ) )
    {
      maxId = 1;
    }
    if ( fabs( this->Plane[2] ) > fabs( this->Plane[maxId] ) )
    {
      maxId = 2;
    }
    double origin[] = { 0., 0., 0. };
    origin[maxId] = this->Plane[3] / this->Plane[maxId];
    vtkPlane* plane = vtkPlane::New();
    plane->SetOrigin( origin );
    plane->SetNormal( this->Plane[0], this->Plane[1], this->Plane[2] );

    // Initialize plane cutter
    this->Cutter = vtkCutter::New();
    this->Cutter->GenerateTrianglesOff();
    this->Cutter->SetCutFunction( plane );

    // Clean up
    plane->Delete();

    // Create storage to keep track of selected cells
    this->SelectedCells = vtkBitArray::New();
    vtkIdType numCells = input->GetNumberOfPoints();
    this->SelectedCells->SetNumberOfTuples( numCells );
    for ( vtkIdType i = 0; i < numCells; ++ i )
    {
      // Initialization is needed because not all cells are pre-processed
      this->SelectedCells->SetValue( i, 0 );
    }

    // First pass across tree roots to evince cells intersected by contours
    vtkIdType index;
    vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
    input->InitializeTreeIterator( it );
    while ( it.GetNextTree( index ) )
    {
      // Initialize new geometric cursor at root of current input tree
      vtkHyperTreeGridCursor* cursor = input->NewGeometricCursor( index );

      // Pre-process tree recursively
      this->RecursivelyPreProcessTree( cursor, mask );
      // Clean up
      cursor->Delete();
    } // it

    // Second pass across tree roots: now compute isocontours recursively
    input->InitializeTreeIterator( it );
    while ( it.GetNextTree( index ) )
    {
      // Initialize new Moore cursor at root of current tree
      vtkHyperTreeGridCursor* cursor = input->NewMooreSuperCursor( index );

      // Generate leaf cell centers recursively
      this->RecursivelyProcessTreeDual( cursor, mask );

      // Clean up
      cursor->Delete();
    } // it

    // Clean up
    this->SelectedCells->Delete();
  } // if ( this->Dual )
  else
  {
    // Initialize output cell data
    this->OutData = output->GetCellData();
    this->OutData->CopyAllocate( this->InData );

    // Iterate over all hyper trees
    vtkIdType index;
    vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
    input->InitializeTreeIterator( it );
    while ( it.GetNextTree( index ) )
    {
      // Initialize new geometric cursor at root of current tree
      vtkHyperTreeGridCursor* cursor = input->NewGeometricCursor( index );

      // Generate leaf cell centers recursively
      this->RecursivelyProcessTreePrimal( cursor, mask );

      // Clean up
      cursor->Delete();
    } // it
  } // else

  // Set output geometry and topology
  output->SetPoints( this->Points );
  output->SetPolys( this->Cells );

  // Clean and squeeze output
  vtkCleanPolyData* cleaner = vtkCleanPolyData::New();
  cleaner->ConvertPolysToLinesOff();
  cleaner->SetInputData( output );
  cleaner->Update();
  output->ShallowCopy( cleaner->GetOutput() );
  output->Squeeze();

  // Clean up
  cleaner->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridPlaneCutter::RecursivelyProcessTreePrimal( vtkHyperTreeGridCursor* cursor,
                                                                vtkBitArray* mask )
{
  // If cursor is at a masked cell stop recursion
  vtkIdType inId = cursor->GetGlobalNodeIndex();
  if ( mask && mask->GetValue( inId ) )
  {
    return;
  }

  // Retrieve input grid
  vtkHyperTreeGrid* input = cursor->GetGrid();

  // Retrieve cursor geometry
  double* origin = cursor->GetOrigin();
  double* size = cursor->GetSize();

  // Initialize cell coordinates
  double cellCoords[8][3];
  for ( int i = 0; i < 8; ++ i )
  {
    cellCoords[i][0] = ( i & 1 ) ? origin[0] + size[0] : origin[0];
    cellCoords[i][1] = ( i & 2 ) ? origin[1] + size[1] : origin[1];
    cellCoords[i][2] = ( i & 4 ) ? origin[2] + size[2] : origin[2];
  }

  // Check cell-plane intersection
  double functEval[8];
  if ( this->CheckIntersection( cellCoords, functEval ) )
  {
    // Create plane cut if cursor is at leaf
    if ( cursor->IsLeaf() )
    {
      // Keep track of the number of intersection points
      int n = 0;

      // Storage for intersection points
      double  points[6][3];

      // Iterate over cell vertices
      for ( int i = 0; i < 8; ++ i )
      {
        // Check all cell edges
        if ( functEval[i] == 0.0 )
        {
          // If current vertex is intersected then save it
          memcpy( points[n], cellCoords[i], 3 * sizeof( double ) );
          ++ n;
        }
        else
        {
          // Check every edge of the current vertex.
          if ( ! ( i & 1 ) && functEval[i] * functEval[i + 1] < 0 )
          {
            // Edge in X
            this->PlaneCut( i, i + 1, cellCoords, n, points );
          }
          if ( ! ( i & 2 ) && functEval[i] * functEval[i + 2] < 0 )
          {
            // Edge in Y
            this->PlaneCut( i, i + 2, cellCoords, n, points );
          }
          if ( ! ( i & 4 ) && functEval[i] * functEval[i + 4] < 0 )
          {
            // Edge in Z
            this->PlaneCut( i, i + 4, cellCoords, n, points );
          }
        } // else
      } // i

      // Now reorder points if necessary
      this->ReorderCutPoints( n, points );

      // Storage for face vertex IDs
      vtkIdType ids[6];
      for ( int i = 0; i < n; ++ i )
      {
        // Save points and get their IDs
        ids[i] = this->Points->InsertNextPoint( points[i] );
      }

      // Insert next face
      vtkIdType outId = this->Cells->InsertNextCell( n, ids );

      // Copy face data from that of the cell from which it comes
      this->OutData->CopyData( this->InData, inId, outId );
    } // if ( cursor->IsLeaf() )
    else
    {
      // Cursor is not at leaf, recurse to all children
      int numChildren = input->GetNumberOfChildren();
      for ( int child = 0; child < numChildren; ++ child )
      {
        // Create child cursor from parent
        vtkHyperTreeGridCursor* childCursor = cursor->Clone();
        childCursor->ToChild( child );

        // Recurse
        this->RecursivelyProcessTreePrimal( childCursor, mask );

        // Clean up
        childCursor->Delete();
        childCursor = nullptr;
      } // child
    } // else
  } // CheckIntersection
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridPlaneCutter::RecursivelyPreProcessTree( vtkHyperTreeGridCursor* cursor,
                                                             vtkBitArray* mask )
{
  // If cursor is at a masked cell stop recursion
  vtkIdType id = cursor->GetGlobalNodeIndex();
  if ( mask && mask->GetValue( id ) )
  {
    return false;
  }

  // A node is not selected until proven otherwise
  bool selected = false;

  // Retrieve input grid
  vtkHyperTreeGrid* input = cursor->GetGrid();

  // Retrieve cursor geometry
  double* origin = cursor->GetOrigin();
  double* size = cursor->GetSize();

  // Initialize cell coordinates
  double cellCoords[8][3];
  for ( int i = 0; i < 8; ++ i )
  {
    cellCoords[i][0] = ( i & 1 ) ? origin[0] + size[0] : origin[0];
    cellCoords[i][1] = ( i & 2 ) ? origin[1] + size[1] : origin[1];
    cellCoords[i][2] = ( i & 4 ) ? origin[2] + size[2] : origin[2];
  }

  // Check cell-plane intersection
  if ( this->CheckIntersection( cellCoords ) )
  {
    // Selected this node
    if ( cursor->IsLeaf() )
    {
      selected = true;
    } // if ( cursor->IsLeaf() )
    else
    {
      // Cursor is not at leaf, recurse to all children
      int numChildren = input->GetNumberOfChildren();
      for ( int child = 0; child < numChildren; ++ child )
      {
        // Create child cursor from parent
        vtkHyperTreeGridCursor* childCursor = cursor->Clone();
        childCursor->ToChild( child );

        // Recurse and keep track of whether this branch is selected
        selected |= this->RecursivelyPreProcessTree( childCursor, mask );

        // Clean up
        childCursor->Delete();
        childCursor = nullptr;
      } // child
    } // else
  } // if ( this->CheckIntersection )

  // Update list of selected cells
  this->SelectedCells->SetTuple1( id, selected );

  // Return whether current node was selected
  return selected;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridPlaneCutter::RecursivelyProcessTreeDual( vtkHyperTreeGridCursor* cursor,
                                                              vtkBitArray* mask )
{
  // If cursor is at a masked cell stop recursion
  vtkIdType id = cursor->GetGlobalNodeIndex();
  if ( mask && mask->GetValue( id ) )
  {
    return;
  }

  // Descend further into input trees only if cursor is not a leaf
  if ( ! cursor->IsLeaf() )
  {
    // Check if cursor is at selected cell
    if ( ! this->SelectedCells->GetTuple1( id ) )
    {
      // Cell is not selected until proven otherwise
      bool selected = false;

      // Iterate over all cursors of Von Neumann neighborhood around center
      for( unsigned int neighbor = 0; neighbor < 26 && ! selected; ++ neighbor )
      {
        // Retrieve global index of neighbor
        vtkIdType idN = cursor->GetCursor( MooreCursors3D[neighbor] )->GetGlobalNodeIndex();

        // Decide whether neighbor was selected
        selected = (this->SelectedCells->GetTuple1( idN ) != 0.0);
      } // neighbor

      // No dual cell with a corner at cursor center will be intersected
      if ( ! selected )
      {
        return;
      }
    } // if ( this->SelectedCells->GetTuple1( id ) )

    // Recurse to all children
    int numChildren = cursor->GetGrid()->GetNumberOfChildren();
    for ( int child = 0; child < numChildren; ++ child )
    {
      // Create child cursor from parent in input grid
      vtkHyperTreeGridCursor* childCursor = cursor->Clone();
      childCursor->ToChild( child );

      // Recurse
      this->RecursivelyProcessTreeDual( childCursor, mask );

      // Clean up
      childCursor->Delete();
      childCursor = nullptr;
    } // child
  } // if ( ! cursor->IsLeaf() )
  else
  {
    // Cursor is at leaf, iterate over its corners
    for ( unsigned int cornerIdx = 0; cornerIdx < 8; ++ cornerIdx )
    {
      // Cell is not selected until proven otherwise
      bool owner = true;

      // Iterate over every leaf touching the corner and check ownership
      for ( unsigned int leafIdx = 0; leafIdx < 8 && owner; ++ leafIdx )
      {
        owner = cursor->GetCornerCursors( cornerIdx, leafIdx, this->Leaves );
      } // leafIdx

      // If cell owns dual cell, compute intersection thereof
      if ( owner )
      {
        // Create dual cell to be intersected as unstructured grid
        vtkUnstructuredGrid* dual = vtkUnstructuredGrid::New();
        dual->Allocate( 1, 1 );
        dual->InsertNextCell( VTK_VOXEL, 8, First8Integers );
        dual->GetPointData()->CopyAllocate( this->InData );

        // Iterate over cell corners
        double x[3];
        for ( int _cornerIdx = 0; _cornerIdx < 8; ++ _cornerIdx )
        {
          // Get cursor corresponding to this corner
          vtkIdType cursorId = this->Leaves->GetId( _cornerIdx );
          vtkHyperTreeGridCursor* cursorN = cursor->GetCursor( cursorId );

          // Retrieve neighbor coordinates and store them
          cursorN->GetPoint( x );
          this->Centers->SetPoint( _cornerIdx, x );

          // Retrieve neighbor index and corresponding input scalar value
          vtkIdType idN = cursorN->GetGlobalNodeIndex();

          // Assign scalar value attached to this cell corner
          dual->GetPointData()->CopyData( this->InData, idN, _cornerIdx );
        } // _cornerIdx

        // Assign geometry of dual cell
        dual->SetPoints( this->Centers );

        // Compute intersection with plane
        this->Cutter->SetInputData( dual );
        this->Cutter->Update();

        // Append computed polygons if some are present in cutter output
        vtkPolyData* pd = this->Cutter->GetOutput();
        vtkIdType nPoints = pd->GetNumberOfPoints();
        if ( nPoints )
        {
          // Keep handle to cut point data
          vtkPointData* pdata = pd->GetPointData();

          // Append new points to existing cut points
          vtkIdType offset = this->Points->GetNumberOfPoints();
          double pt[3];
          for ( vtkIdType i = 0; i < nPoints; ++ i )
          {
            // Retrieve cut point coordinates and insert them into output points
            pd->GetPoint( i, pt );
            this->Points->InsertNextPoint( pt );

            // Copy cut point data to that of corresponding output point
            this->OutData->CopyData( pdata, i, i + offset );
          } // i

          // Append new elements to existing cut element
          vtkIdType ids[6];
          vtkIdType nCells = pd->GetNumberOfCells();
          for ( int i = 0; i < nCells; ++ i )
          {
            // Retrieve element vertex ids
            vtkIdList* vertices = pd->GetCell( i )->GetPointIds();

            // Appropriately offset vertex ids
            vtkIdType n = vertices->GetNumberOfIds();
            for ( int j = 0; j < n; ++ j )
            {
              ids[j] = vertices->GetId( j ) + offset;
            } // j

            // Insert next cell with offset ids
            this->Cells->InsertNextCell( n, ids );
          } // i
        } // if ( nPoints )

        // Clean up
        dual->Delete();
      } // if ( owner )
    } // cornerIdx
  } //else
}

//----------------------------------------------------------------------------
bool vtkHyperTreeGridPlaneCutter::CheckIntersection( double cellCoords[8][3],
                                                     double functEval[8] )
{
  // Iterate over cell vertices
  int i;
  for ( i = 0; i < 8; ++ i )
  {
    // Evaluate the plane in every coordinate
    functEval[i] = cellCoords[i][0] * this->Plane[0] +
      cellCoords[i][1] * this->Plane[1] +
      cellCoords[i][2] * this->Plane[2] -
      this->Plane[3];
  } // i

  // Evaluate plane equation at first corner
  double firstVal = functEval[0];

  // Check if there is any sign change
  i = 7;
  while ( i && functEval[i] * firstVal > 0. )
  {
    -- i;
  }

  // Intersection if while statement broke early
  return ( i != 0 );
}

//----------------------------------------------------------------------------
bool vtkHyperTreeGridPlaneCutter::CheckIntersection( double cellCoords[8][3] )
{
  // Evaluate plane equation at first corner
  double firstVal = cellCoords[0][0] * this->Plane[0] +
    cellCoords[0][1] * this->Plane[1] +
    cellCoords[0][2] * this->Plane[2] -
    this->Plane[3];

  // Check if there is any sign change
  int i = 1;
  bool sameSign = true;
  while ( i < 8 && sameSign )
  {
    double functEval = cellCoords[i][0] * this->Plane[0] +
      cellCoords[i][1] * this->Plane[1] +
      cellCoords[i][2] * this->Plane[2] -
      this->Plane[3];
    sameSign = ( firstVal * functEval > 0 );
    ++ i;
  }

  return ! sameSign;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridPlaneCutter::PlaneCut( int i, int j, double cellCoords[8][3],
                                            int &n, double point[][3] )
{
  // The distance between vertex indices gives us the axis direction
  if ( j - i == 1 )
  {
    // X direction
    point[n][0] = ( this->Plane[3] - this->Plane[1] * cellCoords[i][1]
                    - this->Plane[2] * cellCoords[i][2] ) / this->Plane[0];
    point[n][1] = cellCoords[i][1];
    point[n][2] = cellCoords[i][2];
  } // if ( j - i == 1 )
  else if ( j - i == 2 )
  {
    // Y direction
    point[n][0] = cellCoords[i][0];
    point[n][1] = ( this->Plane[3] - this->Plane[0] * cellCoords[i][0]
                    - this->Plane[2] * cellCoords[i][2] ) / this->Plane[1];
    point[n][2] = cellCoords[i][2];
  } // else if ( j - i == 2 )
  else // if ( j - i == 4 )
  {
    // Z direction
    point[n][0] = cellCoords[i][0];
    point[n][1] = cellCoords[i][1];
    point[n][2] = ( this->Plane[3] - this->Plane[0] * cellCoords[i][0]
                    - this->Plane[1] * cellCoords[i][1] ) / this->Plane[2];
  } // else if ( j - i == 4 )

  // Move to next point
  ++ n;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridPlaneCutter::ReorderCutPoints( int n, double points[][3] )
{
  // Iterate over all polygonal vertices but the last one
  for ( int i = 0; i < n - 2; ++ i )
  {
    // Search the closest point to i in the sense of sharing the most coordinates
    int index = i+1; //in practice a don't care, set to satisfy -Wuninitialized
    int minDistance = 4;
    for ( int j = i + 1; j < n; ++ j )
    {
      // Compute the distance: number of different coordinates
      int distance = 0;
      if ( points[j][0] != points[i][0] )
      {
        ++ distance;
      }
      if ( points[j][1] != points[i][1] )
      {
        ++ distance;
      }
      if ( points[j][2] != points[i][2] )
      {
        ++ distance;
      }
      if ( distance < minDistance )
      {
        // Store the index of current point and update minDistance
        index = j;
        minDistance = distance;
      }
    }
    if ( index != i + 1 )
    {
      // If the closest point is not i + 1, then swap point positions
      double swap[3];
      memcpy( swap, points[index], 3 * sizeof( double ) );
      memcpy( points[index], points[i + 1], 3 * sizeof( double ) );
      memcpy( points[i + 1], swap, 3 * sizeof( double ) );
    }
  }
}
