/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkStructuredGridConnectivity.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkStructuredGridConnectivity.h"

// VTK includes
#include "vtkObjectFactory.h"
#include "vtkGhostArray.h"
#include "vtkStructuredData.h"
#include "vtkStructuredExtent.h"
#include "vtkIdList.h"
#include "vtkStructuredNeighbor.h"
#include "vtkUnsignedCharArray.h"

// C++ STL library includes
#include <set>
#include <vector>
#include <algorithm>

#define NO_OVERLAP      0
#define NODE_OVERLAP    1
#define EDGE_OVERLAP    2
#define PARTIAL_OVERLAP 3

vtkStandardNewMacro( vtkStructuredGridConnectivity );

// Description:
// An enum to define the 6 block faces
namespace BlockFace {
  enum
  {
    FRONT         = 0, // (+k diretion)
    BACK          = 1, // (-k direction)
    RIGHT         = 2, // (+i direction)
    LEFT          = 3, // (-i direction)
    TOP           = 4, // (+j direction)
    BOTTOM        = 5, // (-j direction)
    NOT_ON_BLOCK_FACE = 6
  };
}

//------------------------------------------------------------------------------
vtkStructuredGridConnectivity::vtkStructuredGridConnectivity()
{
  this->DataDescription = -1;
  this->NumberOfGrids   = 0;
  this->WholeExtent[0]  = this->WholeExtent[1] = this->WholeExtent[2] =
  this->WholeExtent[3]  = this->WholeExtent[4] = this->WholeExtent[5] = -1;
  this->GridExtents.clear();
}

//------------------------------------------------------------------------------
vtkStructuredGridConnectivity::~vtkStructuredGridConnectivity()
{
  this->GridExtents.clear();
}

//------------------------------------------------------------------------------
void vtkStructuredGridConnectivity::PrintSelf(std::ostream& os,vtkIndent indent)
{
  this->Superclass::PrintSelf( os, indent );

  os << "========================\n";
  os << "CONNECTIVITY INFORMATION: \n";
  for( int gridID=0; gridID < this->NumberOfGrids; ++gridID )
    {
    int GridExtent[6];
    this->GetGridExtent( gridID, GridExtent );
    os << "GRID:";
    for( int i=0; i < 6; i+=2 )
      {
      os << " [";
      os << GridExtent[i] << ", " << GridExtent[i+1] << "]";
      }
    os << std::endl;
    os << " Connecting faces: "
       << this->GetNumberOfConnectingBlockFaces( gridID ) << " ";

    os << "[ ";
    if( this->HasBlockConnection( gridID, BlockFace::FRONT ) )
      {
      os << "FRONT(+k) ";
      }
    if( this->HasBlockConnection( gridID, BlockFace::BACK ) )
      {
      os << "BACK(-k) ";
      }
    if( this->HasBlockConnection( gridID, BlockFace::RIGHT ) )
      {
      os << "RIGHT(+i) ";
      }
    if( this->HasBlockConnection( gridID, BlockFace::LEFT ) )
      {
      os << "LEFT(-i) ";
      }
    if( this->HasBlockConnection( gridID, BlockFace::TOP) )
      {
      os << "TOP(+j) ";
      }
    if( this->HasBlockConnection( gridID, BlockFace::BOTTOM) )
      {
      os << "BOTTOM(-j) ";
      }
    os << "] ";
    os << std::endl;

    for( unsigned int nei=0; nei < this->Neighbors[gridID].size(); ++nei )
      {
      int NeiExtent[6];
      this->GetGridExtent( this->Neighbors[gridID][nei].NeighborID, NeiExtent );

      os << "\t N[" << nei << "]: ";
      for( int i=0; i < 6; i+=2 )
        {
        os << " [";
        os << NeiExtent[i] << ", " << NeiExtent[i+1] << "] ";
        }

      os << " overlaps @ ";
      for( int i=0; i < 6; i+=2 )
        {
        os << " [";
        os << this->Neighbors[gridID][nei].OverlapExtent[ i ] << ", ";
        os << this->Neighbors[gridID][nei].OverlapExtent[ i+1 ] << "] ";
        }

      os << " orientation: (";
      os << this->Neighbors[gridID][nei].Orientation[ 0 ] << ", ";
      os << this->Neighbors[gridID][nei].Orientation[ 1 ] << ", ";
      os << this->Neighbors[gridID][nei].Orientation[ 2 ] << ")\n ";
      os << std::endl;
      } // END for all neis
    } // END for all grids
}

//------------------------------------------------------------------------------
void vtkStructuredGridConnectivity::RegisterGrid(
    const int gridID, int ext[6],
    vtkUnsignedCharArray* nodesGhostArray,
    vtkUnsignedCharArray* cellGhostArray,
    vtkPointData* pointData,
    vtkCellData* cellData,
    vtkPoints* gridNodes )
{
  assert( "pre: gridID out-of-bounds!" &&
        (gridID >= 0  && gridID < this->NumberOfGrids) );

  for( int i=0; i < 6; ++i )
    {
    this->GridExtents[ gridID*6+i ] = ext[i];
    }

  this->RegisterGridGhostArrays( gridID, nodesGhostArray, cellGhostArray );
  this->RegisterFieldData( gridID, pointData, cellData );
  this->RegisterGridNodes( gridID, gridNodes );
}

//------------------------------------------------------------------------------
void vtkStructuredGridConnectivity::GetGridExtent(const int gridID, int ext[6])
{
  assert( "pre: gridID out-of-bounds!" &&
        (gridID >= 0  && gridID < this->NumberOfGrids) );
  for( int i=0; i < 6; ++i )
    {
    ext[i] = this->GridExtents[ gridID*6+i ];
    }
}

//------------------------------------------------------------------------------
void vtkStructuredGridConnectivity::GetGhostedGridExtent(
    const int gridID, int ext[6])
{
  assert( "pre: gridID out-of-bounds!" &&
        (gridID >= 0  && gridID < this->NumberOfGrids) );

  if( this->GhostedExtents.size() == 0 )
    {
    vtkErrorMacro( "No ghosted extents found for registered grid extends!!!" );
    return;
    }

  assert( "GhostedExtents are not aligned with registered grid extents" &&
        ( this->GhostedExtents.size() == this->GridExtents.size() ) );
  for( int i=0; i < 6; ++i )
    {
    ext[i] = this->GhostedExtents[ gridID*6+i ];
    }
}

//------------------------------------------------------------------------------
void vtkStructuredGridConnectivity::AcquireDataDescription()
{
  if( this->DataDescription != -1 )
    {
    return;
    }

  int dims[3];

  vtkStructuredExtent::GetDimensions( this->WholeExtent, dims );

  this->DataDescription = vtkStructuredData::GetDataDescription( dims );
  assert( "pre: Error acquiring data description" &&
        (this->DataDescription >= 0) );
  assert( "pre: grid description cannot be empty" &&
        (this->DataDescription != VTK_EMPTY) );
}

//------------------------------------------------------------------------------
vtkIdList* vtkStructuredGridConnectivity::GetNeighbors(
    const int gridID,int *extents )
{
  assert( "pre: input extents array is NULL" && (extents != NULL) );

  int N = this->GetNumberOfNeighbors( gridID );
  if( N < 1 )
    {
    return NULL;
    }

  vtkIdList *neiList = vtkIdList::New();
  neiList->SetNumberOfIds( N );

  unsigned int nei=0;
  for( ; nei < this->Neighbors[ gridID ].size(); ++nei )
    {
    vtkIdType neiId = this->Neighbors[ gridID ][ nei ].NeighborID;
    neiList->SetId( nei, neiId );
    for( int i=0; i < 6; ++i  )
      {
      extents[ nei*6+i ] = this->Neighbors[ gridID ][ nei ].OverlapExtent[ i ];
      }
    } // END for all neighbors

  assert( "post: N==neiList.size()" && (N == neiList->GetNumberOfIds()) );
  return( neiList );
}

//------------------------------------------------------------------------------
void vtkStructuredGridConnectivity::ComputeNeighbors()
{
  // STEP 0: Acquire data description, i.e., determine how the structured data
  // is laid out, e.g., is it volumetric or 2-D along some plane, XY, XZ, or YZ.
  this->AcquireDataDescription( );
  if( this->DataDescription == VTK_EMPTY ||
      this->DataDescription == VTK_SINGLE_POINT )
    {
    return;
    }

  // STEP 1: Establish neighbors based on the structured extents.
  for( int i=0; i < this->NumberOfGrids; ++i )
    {
    this->SetBlockTopology( i );
    for( int j=i+1; j < this->NumberOfGrids; ++j )
      {
      this->EstablishNeighbors(i,j);
      } // END for all j
    } // END for all i

  // STEP 2: Fill the ghost arrays
  for( int i=0; i < this->NumberOfGrids; ++i )
    {
    if( this->GridPointGhostArrays[ i ] != NULL )
      {
      this->FillGhostArrays(
        i, this->GridPointGhostArrays[ i ], this->GridCellGhostArrays[ i ] );
      }
    } // END for all grids
}

//------------------------------------------------------------------------------
void vtkStructuredGridConnectivity::SearchNeighbors(
    const int gridID, const int i, const int j, const int k,
    vtkIdList *neiList )
{
  assert( "pre: neiList should not be NULL" && (neiList != NULL) );
  assert( "pre: gridID is out-of-bounds" &&
           ( (gridID >= 0) && (gridID < this->NumberOfGrids) ) );

  for( unsigned int nei=0; nei < this->Neighbors[ gridID ].size(); ++nei )
    {
    vtkStructuredNeighbor *myNei = &this->Neighbors[ gridID ][ nei ];
    assert( "pre: myNei != NULL" && (myNei != NULL) );

    if( this->IsNodeWithinExtent( i, j, k, myNei->OverlapExtent) )
      {
      neiList->InsertNextId( myNei->NeighborID );
      }
    } // END for all neis
}

//------------------------------------------------------------------------------
void vtkStructuredGridConnectivity::MarkNodeProperty(
    const int gridID, const int i, const int j, const int k,
    int ext[6], unsigned char &p )
{
  vtkGhostArray::Reset( p );

  int realExtent[6];
  this->GetRealExtent( ext, realExtent );

  // Check if the node is an interior a node, i.e., it is not on any boundary
  // shared or real boundary and not in a ghost region. Interior nodes can only
  // be internal nodes!
  if( this->IsNodeInterior( i,j,k, realExtent) )
    {
    vtkGhostArray::SetProperty( p, vtkGhostArray::INTERNAL );
    }
  else
    {
    // If the node is on the boundary of the computational domain mark it
    if( this->IsNodeOnBoundary(i,j,k) )
      {
      vtkGhostArray::SetProperty( p, vtkGhostArray::BOUNDARY );
      }

    // Check if the node is also on a shared boundary or if it is a ghost node
    if(this->IsNodeOnSharedBoundary(gridID,ext,realExtent,i,j,k))
      {
      vtkGhostArray::SetProperty( p, vtkGhostArray::SHARED );

      // For shared nodes we must check for ownership
      vtkIdList *neiList = vtkIdList::New();
      this->SearchNeighbors( gridID, i, j, k, neiList );

      if( neiList->GetNumberOfIds() > 0 )
        {
        for( vtkIdType nei=0; nei < neiList->GetNumberOfIds(); ++nei )
          {
          // If my gridID is not the smallest gridID that shares the point,then
          // I don't own the point.
          // The convention is that the grid with the smallest gridID will own the
          // point and all other grids should IGNORE it when computing statistics
          // etc.
          if( gridID > neiList->GetId( nei ) )
            {
            vtkGhostArray::SetProperty(p,vtkGhostArray::IGNORE );
            break;
            }
          } // END for all neis
        } // END if neisList isn't empty
        neiList->Delete();
      }// END if node is on a shared boundary
    else if( this->IsGhostNode(gridID,ext,realExtent,i,j,k) )
      {
      vtkGhostArray::SetProperty( p, vtkGhostArray::GHOST );

      // Ghost nodes are always ignored!
      vtkGhostArray::SetProperty( p, vtkGhostArray::IGNORE );
      }
    }

}

//------------------------------------------------------------------------------
void vtkStructuredGridConnectivity::FillGhostArrays(
    const int gridID,
    vtkUnsignedCharArray *nodesArray,
    vtkUnsignedCharArray *cellsArray )
{
  assert( "pre: Nodes array is not NULL" && (nodesArray != NULL) );
  assert( "pre: Cell array is not NULL" && (cellsArray != NULL) );

  int GridExtent[6];
  this->GetGridExtent( gridID, GridExtent );

  int ijk[3];
  for( int i=GridExtent[0]; i <= GridExtent[1]; ++i )
    {
    for( int j=GridExtent[2]; j <= GridExtent[3]; ++j )
      {
      for( int k=GridExtent[4]; k <= GridExtent[5]; ++k )
        {
        ijk[0]=i;ijk[1]=j;ijk[2]=k;
        vtkIdType idx =
          vtkStructuredData::ComputePointIdForExtent(GridExtent,ijk);

        this->MarkNodeProperty(
            gridID,i,j,k,GridExtent,
            *nodesArray->GetPointer( idx ) );
        } // END for all k
      } // END for all j
    } // END for all i

  // TODO: mark cells
}

//------------------------------------------------------------------------------
void vtkStructuredGridConnectivity::GetRealExtent(
    int GridExtent[6], int RealExtent[6] )
{
  for( int i=0; i < 6; ++i)
    {
    RealExtent[i] = GridExtent[i];
    }

  if( this->NumberOfGhostLayers == 0 )
    {
    return;
    }

  switch( this->DataDescription )
    {
    case VTK_X_LINE:
      RealExtent[0] += this->NumberOfGhostLayers; // imin
      RealExtent[1] -= this->NumberOfGhostLayers; // imax
      break;
    case VTK_Y_LINE:
      RealExtent[2] += this->NumberOfGhostLayers; // jmin
      RealExtent[3] -= this->NumberOfGhostLayers; // jmax
      break;
    case VTK_Z_LINE:
      RealExtent[4] += this->NumberOfGhostLayers; // kmin
      RealExtent[5] -= this->NumberOfGhostLayers; // kmax
      break;
    case VTK_XY_PLANE:
      RealExtent[0] += this->NumberOfGhostLayers; // imin
      RealExtent[1] -= this->NumberOfGhostLayers; // imax
      RealExtent[2] += this->NumberOfGhostLayers; // jmin
      RealExtent[3] -= this->NumberOfGhostLayers; // jmax
      break;
    case VTK_YZ_PLANE:
      RealExtent[2] += this->NumberOfGhostLayers; // jmin
      RealExtent[3] -= this->NumberOfGhostLayers; // jmax
      RealExtent[4] += this->NumberOfGhostLayers; // kmin
      RealExtent[5] -= this->NumberOfGhostLayers; // kmax
      break;
    case VTK_XZ_PLANE:
      RealExtent[0] += this->NumberOfGhostLayers; // imin
      RealExtent[1] -= this->NumberOfGhostLayers; // imax
      RealExtent[4] += this->NumberOfGhostLayers; // kmin
      RealExtent[5] -= this->NumberOfGhostLayers; // kmax
      break;
    case VTK_XYZ_GRID:
      RealExtent[0] += this->NumberOfGhostLayers; // imin
      RealExtent[1] -= this->NumberOfGhostLayers; // imax
      RealExtent[2] += this->NumberOfGhostLayers; // jmin
      RealExtent[3] -= this->NumberOfGhostLayers; // jmax
      RealExtent[4] += this->NumberOfGhostLayers; // kmin
      RealExtent[5] -= this->NumberOfGhostLayers; // kmax
      break;
    default:
      std::cout << "Data description is: " << this->DataDescription << "\n";
      std::cout.flush();
      assert( "pre: Undefined data-description!" && false );
    }
  vtkStructuredExtent::Clamp( RealExtent, this->WholeExtent );
}

//------------------------------------------------------------------------------
bool vtkStructuredGridConnectivity::IsNodeOnBoundaryOfExtent(
    const int i, const int j, const int k, int ext[6] )
{
  bool status = false;
  switch( this->DataDescription )
    {
    case VTK_X_LINE:
       if( i==ext[0] || i==ext[1] )
         {
         status = true;
         }
       break;
     case VTK_Y_LINE:
       if( j==ext[2] || j==ext[3] )
         {
         status = true;
         }
       break;
     case VTK_Z_LINE:
       if( k==ext[4] || k==ext[5] )
         {
         status = true;
         }
       break;
     case VTK_XY_PLANE:
       if( i==ext[0] || i==ext[1] ||
           j==ext[2] || j==ext[3] )
         {
         status = true;
         }
       break;
     case VTK_YZ_PLANE:
       if( j==ext[2] || j==ext[3] ||
           k==ext[4] || k==ext[5] )
         {
         status = true;
         }
       break;
     case VTK_XZ_PLANE:
       if( i==ext[0] || i==ext[1] ||
           k==ext[4] || k==ext[5] )
         {
         status = true;
         }
       break;
     case VTK_XYZ_GRID:
       if( i==ext[0] || i==ext[1] ||
           j==ext[2] || j==ext[3] ||
           k==ext[4] || k==ext[5] )
         {
         status = true;
         }
       break;
     default:
       std::cout << "Data description is: " << this->DataDescription << "\n";
       std::cout.flush();
       assert( "pre: Undefined data-description!" && false );
    } // END switch

  return( status );
}

//------------------------------------------------------------------------------
bool vtkStructuredGridConnectivity::IsNodeOnSharedBoundary(
    const int gridID, int GridExtent[6], int RealExtent[6],
    const int i, const int j, const int k )
{
  if( this->IsNodeOnBoundaryOfExtent(i,j,k,RealExtent) )
    {
    int orient[3];
    this->GetIJKBlockOrientation( i,j,k,RealExtent,orient);
    for( int ii=0; ii < 3; ++ii )
      {
      if( (orient[ii] != BlockFace::NOT_ON_BLOCK_FACE) &&
          this->HasBlockConnection(gridID, orient[ii]) )
        {
        return true;
        }
      } // END for all dimensions
    return false;
    }
  else
    {
    return false;
    }
}

//------------------------------------------------------------------------------
bool vtkStructuredGridConnectivity::IsGhostNode(
        const int gridID, int GridExtent[6], int RealExtent[6],
        const int i, const int j, const int k )
{
  // STEP 0: Check if there are any ghost-layers. Note, if the original data
  // that the user is registering contains ghost-layers, the users must set
  // the number of ghost-layers.
  if( this->NumberOfGhostLayers == 0 )
    {
    // Grid has no ghost-layers, so, the node cannot be a ghost node
    return false;
    }

  // STEP 1: Initialize the return status and the real extent
  bool status = false;
  if( this->IsNodeWithinExtent( i,j,k,RealExtent ) )
    {
    status = false;
    }
  else if( this->IsNodeWithinExtent( i,j,k, GridExtent ) )
    {
    status = true;
    }

  return( status );
}

//------------------------------------------------------------------------------
bool vtkStructuredGridConnectivity::IsNodeOnBoundary(
    const int i, const int j, const int k )
{
  return( this->IsNodeOnBoundaryOfExtent( i,j,k, this->WholeExtent) );
}

//------------------------------------------------------------------------------
bool vtkStructuredGridConnectivity::IsNodeInterior(
    const int i, const int j, const int k,
    int GridExtent[6] )
{
  bool status = false;

  switch( this->DataDescription )
    {
    case VTK_X_LINE:
      if( (GridExtent[0] < i) && (i < GridExtent[1]) )
        {
        status = true;
        }
      break;
    case VTK_Y_LINE:
      if( (GridExtent[2] < j) && (j < GridExtent[3] ) )
        {
        status = true;
        }
      break;
    case VTK_Z_LINE:
      if( (GridExtent[4] < k) && (k < GridExtent[5] ) )
        {
        status = true;
        }
      break;
    case VTK_XY_PLANE:
      if( (GridExtent[0] < i) && (i < GridExtent[1]) &&
          (GridExtent[2] < j) && (j < GridExtent[3])  )
        {
        status = true;
        }
      break;
    case VTK_YZ_PLANE:
      if( (GridExtent[2] < j) && (j < GridExtent[3] ) &&
          (GridExtent[4] < k) && (k < GridExtent[5] ) )
        {
        status = true;
        }
      break;
    case VTK_XZ_PLANE:
      if( (GridExtent[0] < i) && (i < GridExtent[1] ) &&
          (GridExtent[4] < k) && (k < GridExtent[5] ) )
        {
        status = true;
        }
      break;
    case VTK_XYZ_GRID:
      if( (GridExtent[0] < i) && (i < GridExtent[1]) &&
          (GridExtent[2] < j) && (j < GridExtent[3]) &&
          (GridExtent[4] < k) && (k < GridExtent[5]) )
        {
        status = true;
        }
      break;
    default:
      std::cout << "Data description is: " << this->DataDescription << "\n";
      std::cout.flush();
      assert( "pre: Undefined data-description!" && false );
    } // END switch

  return( status );
}

//------------------------------------------------------------------------------
bool vtkStructuredGridConnectivity::IsNodeWithinExtent(
    const int i, const int j, const int k,
    int GridExtent[6] )
{
  bool status = false;

  switch( this->DataDescription )
    {
    case VTK_X_LINE:
      if( (GridExtent[0] <= i) && (i <= GridExtent[1]) )
        {
        status = true;
        }
      break;
    case VTK_Y_LINE:
      if( (GridExtent[2] <= j) && (j <= GridExtent[3] ) )
        {
        status = true;
        }
      break;
    case VTK_Z_LINE:
      if( (GridExtent[4] <= k) && (k <= GridExtent[5] ) )
        {
        status = true;
        }
      break;
    case VTK_XY_PLANE:
      if( (GridExtent[0] <= i) && (i <= GridExtent[1]) &&
          (GridExtent[2] <= j) && (j <= GridExtent[3])  )
        {
        status = true;
        }
      break;
    case VTK_YZ_PLANE:
      if( (GridExtent[2] <= j) && (j <= GridExtent[3] ) &&
          (GridExtent[4] <= k) && (k <= GridExtent[5] ) )
        {
        status = true;
        }
      break;
    case VTK_XZ_PLANE:
      if( (GridExtent[0] <= i) && (i <= GridExtent[1] ) &&
          (GridExtent[4] <= k) && (k <= GridExtent[5] ) )
        {
        status = true;
        }
      break;
    case VTK_XYZ_GRID:
      if( (GridExtent[0] <= i) && (i <= GridExtent[1]) &&
          (GridExtent[2] <= j) && (j <= GridExtent[3]) &&
          (GridExtent[4] <= k) && (k <= GridExtent[5]) )
        {
        status = true;
        }
      break;
    default:
      std::cout << "Data description is: " << this->DataDescription << "\n";
      std::cout.flush();
      assert( "pre: Undefined data-description!" && false );
    } // END switch

  return( status );
}

//------------------------------------------------------------------------------
void vtkStructuredGridConnectivity::EstablishNeighbors(const int i,const int j)
{
  assert( "pre: i < j" && (i < j) );

  int iGridExtent[6];
  int jGridExtent[6];
  this->GetGridExtent( i, iGridExtent );
  this->GetGridExtent( j, jGridExtent );

  // A 3-tuple that defines the grid orientation of the form {i,j,k} where
  // i=0;, j=1, k=2. For example, let's say that we want to define the
  // orientation to be in the XZ plane, then, orientation  array would be
  // constructed as follows: {0,2 -1}, where -1 indicates a NIL value.
  int orientation[3];

  // A place holder for setting up ndim, which store the dimensionality of
  // the data.
  int ndim = 3;

  switch( this->DataDescription )
    {
    case VTK_X_LINE:
      ndim           =  1;
      orientation[0] =  0;
      orientation[1] = -1;
      orientation[2] = -1;
      break;
    case VTK_Y_LINE:
      ndim           =  1;
      orientation[0] =  1;
      orientation[1] = -1;
      orientation[2] = -1;
      break;
    case VTK_Z_LINE:
      ndim           =  1;
      orientation[0] =  2;
      orientation[1] = -1;
      orientation[2] = -1;
      break;
    case VTK_XY_PLANE:
      ndim           =  2;
      orientation[0] =  0;
      orientation[1] =  1;
      orientation[2] = -1;
      break;
    case VTK_YZ_PLANE:
      ndim           =  2;
      orientation[0] =  1;
      orientation[1] =  2;
      orientation[2] = -1;
      break;
    case VTK_XZ_PLANE:
      ndim           =  2;
      orientation[0] =  0;
      orientation[1] =  2;
      orientation[2] = -1;
      break;
    case VTK_XYZ_GRID:
      ndim           =  3;
      orientation[0] =  0;
      orientation[1] =  1;
      orientation[2] =  2;
      break;
    default:
      std::cout << "Data description is: " << this->DataDescription << "\n";
      std::cout.flush();
      assert( "pre: Undefined data-description!" && false );
    } // END switch

  this->DetectNeighbors( i, j, iGridExtent, jGridExtent, orientation, ndim );
}

//------------------------------------------------------------------------------
void vtkStructuredGridConnectivity::DetermineNeighborOrientation(
    const int idx, int A[2], int overlap[2], int orient[3] )
{
  // We know that A,B are overlapping!

  assert( "pre: idx is out-of-bounds" && (idx >= 0) && (idx < 3) );

  bool lo = this->InBounds( A[0], overlap[0],overlap[1] );
  bool hi = this->InBounds( A[1], overlap[0],overlap[1] );

  if( lo && !hi )
    {
    orient[ idx ] = vtkStructuredNeighbor::LO;
    }
  else if( !lo && hi )
    {
    orient[ idx ] = vtkStructuredNeighbor::HI;
    }
  else if( lo && hi )
    {
    orient[ idx ] = vtkStructuredNeighbor::BOTH;
    }
  else
    {
    orient[ idx ] = vtkStructuredNeighbor::UNDEFINED;
    }
}

//------------------------------------------------------------------------------
void vtkStructuredGridConnectivity::DetectNeighbors(
    const int i, const int j,
    int ex1[6], int ex2[6], int orientation[3], int ndim )
{
  vtkstd::vector< int > status;
  status.resize( ndim );

  int A[2];
  int B[2];
  int overlap[2];
  int iOrientation[3];
  int jOrientation[3];

  int overlapExtent[6];
  for( int ii=0; ii < 3; ++ii )
    {
    overlapExtent[ ii*2 ] = overlapExtent[ ii*2+1 ] = 0;
    iOrientation[ ii ]    = vtkStructuredNeighbor::UNDEFINED;
    jOrientation[ ii ]    = vtkStructuredNeighbor::UNDEFINED;
    }

  int dim = 0;
  int idx = 0;
  for( dim=0; dim < ndim; ++dim )
    {
    idx           = orientation[dim];
    A[0]          = ex1[ idx*2 ];
    A[1]          = ex1[ idx*2+1 ];
    B[0]          = ex2[ idx*2 ];
    B[1]          = ex2[ idx*2+1 ];
    status[ idx ] = this->IntervalOverlap( A, B, overlap );
    if( status[idx] == NO_OVERLAP )
      {
      return; /* No neighbors */
      }

    overlapExtent[ idx*2 ]   = overlap[0];
    overlapExtent[ idx*2+1 ] = overlap[1];

    this->DetermineNeighborOrientation( idx, A, overlap, iOrientation );
    this->DetermineNeighborOrientation( idx, B, overlap, jOrientation );
    } // END for all dimensions

  this->SetNeighbors( i, j, iOrientation, jOrientation, overlapExtent );
}

//------------------------------------------------------------------------------
void vtkStructuredGridConnectivity::SetBlockTopology( const int gridID )
{
  int idx;

  int gridExtent[6];
  this->GetGridExtent( gridID, gridExtent );

  // Check in IMIN
  if( gridExtent[0] > this->WholeExtent[0] )
    {
    this->AddBlockConnection( gridID, BlockFace::LEFT);
    }

  // Check in IMAX
  if( gridExtent[1] < this->WholeExtent[1] )
    {
    this->AddBlockConnection( gridID, BlockFace::RIGHT);
    }

  // Check in JMIN
  if( gridExtent[2] > this->WholeExtent[2] )
    {
    this->AddBlockConnection( gridID, BlockFace::BOTTOM );
    }

  // Check in JMAX
  if( gridExtent[3] < this->WholeExtent[3] )
    {
    this->AddBlockConnection( gridID, BlockFace::TOP );
    }

  // Check in KMIN
  if( gridExtent[4] > this->WholeExtent[4] )
    {
    this->AddBlockConnection( gridID, BlockFace::BACK );
    }

  // Check in KMAX
  if( gridExtent[5] < this->WholeExtent[5] )
    {
    this->AddBlockConnection( gridID, BlockFace::FRONT );
    }
}

//------------------------------------------------------------------------------
void vtkStructuredGridConnectivity::SetNeighbors(
            const int i, const int j,
            int i2jOrientation[3], int j2iOrientation[3],
            int overlapExtent[6] )
{
  vtkStructuredNeighbor Ni2j( j, overlapExtent, i2jOrientation );
  vtkStructuredNeighbor Nj2i( i, overlapExtent, j2iOrientation );

  this->Neighbors[ i ].push_back( Ni2j );
  this->Neighbors[ j ].push_back( Nj2i );
}

//------------------------------------------------------------------------------
void vtkStructuredGridConnectivity::PrintExtent( int ex[6] )
{
  for( int i=0; i < 3; ++i )
    {
    std::cout << " [" << ex[i*2] << ", " << ex[i*2+1] << "] ";
    }
  std::cout << std::endl;
  std::cout.flush();
}

//------------------------------------------------------------------------------
int vtkStructuredGridConnectivity::DoPartialOverlap(
    int s[2], int S[2], int overlap[2] )
{
  if( this->InBounds(s[0],S[0],S[1]) && this->InBounds(s[1],S[0],S[1]) )
    {
    overlap[0] = s[0];
    overlap[1] = s[1];
    return PARTIAL_OVERLAP;
    }
  else if( this->InBounds(s[0], S[0], S[1]) )
    {
    overlap[0] = s[0];
    overlap[1] = S[1];
    if( overlap[0] == overlap[1] )
      {
      return NODE_OVERLAP;
      }
    else
      {
      return PARTIAL_OVERLAP;
      }
    }
  else if( this->InBounds(s[1], S[0],S[1]) )
    {
    overlap[0] = S[0];
    overlap[1] = s[1];
    if( overlap[0] == overlap[1] )
      {
      return NODE_OVERLAP;
      }
    else
      {
      return PARTIAL_OVERLAP;
      }
    }
  return NO_OVERLAP;
}

//------------------------------------------------------------------------------
int vtkStructuredGridConnectivity::PartialOverlap(
                                    int A[2], const int CardinalityOfA,
                                    int B[2], const int CardinalityOfB,
                                    int overlap[2] )
{
  if( CardinalityOfA > CardinalityOfB )
    {
    return( this->DoPartialOverlap( B, A, overlap ) );
    }
  else if( CardinalityOfB > CardinalityOfA )
    {
    return( this->DoPartialOverlap( A, B, overlap )   );
    }

  // Code should not reach here!
  return NO_OVERLAP;
}

//------------------------------------------------------------------------------
int vtkStructuredGridConnectivity::IntervalOverlap(
                    int A[2], int B[2], int overlap[2] )
{
  int rc = NO_OVERLAP;

  // STEP 0: Check if we must check for a partial overlap
  int CardinalityOfA = this->Cardinality( A );
  int CardinalityOfB = this->Cardinality( B );
  if( CardinalityOfA != CardinalityOfB )
    {
    return( this->PartialOverlap(A,CardinalityOfA,B,CardinalityOfB,overlap));
    }

  // Otherwise, check if the intervals overlap at a node or are 1-to-1, i.e.,
  // form an edge

  // STEP 1: Initialize overlap
  overlap[0] = overlap[1] = -1;

  // STEP 2: Allocate internal intersection vector. Note, since the cardinality
  // of A,B is 2, the intersection vector can be at most of size 2.
  std::vector< int > intersection;
  intersection.resize( 2 );

  // STEP 3: Compute intersection
  std::vector< int >::iterator it;
  it = std::set_intersection( A, A+2, B, B+2, intersection.begin() );

  // STEP 4: Find number of intersections and overlap extent
  int N = static_cast< int >( it-intersection.begin() );

  switch( N )
    {
    case 0:
      rc = NO_OVERLAP;
      overlap[ 0 ] = overlap[ 1 ] = -1;
      break;
    case 1:
      rc = NODE_OVERLAP;
      overlap[0] = overlap[1] = intersection[0];
      break;
    case 2:
      rc = EDGE_OVERLAP;
      overlap[0] = intersection[0];
      overlap[1] = intersection[1];
      break;
    default:
      rc = NO_OVERLAP;
      overlap[ 0 ] = overlap[ 1 ] = -1;
      vtkErrorMacro( "Code should not reach here!" )
    }

  return( rc );
}

//------------------------------------------------------------------------------
void vtkStructuredGridConnectivity::GetIJKBlockOrientation(
    const int i, const int j, const int k, int ext[6], int orientation[3] )
{
  orientation[0]=orientation[1]=orientation[2]=BlockFace::NOT_ON_BLOCK_FACE;

  switch( this->DataDescription )
    {
    case VTK_X_LINE:
      this->Get1DOrientation(
          i, ext[0], ext[1], BlockFace::LEFT, BlockFace::RIGHT,
          BlockFace::NOT_ON_BLOCK_FACE);
      break;
    case VTK_Y_LINE:
      this->Get1DOrientation(
          j, ext[2], ext[3], BlockFace::BOTTOM, BlockFace::TOP,
          BlockFace::NOT_ON_BLOCK_FACE );
      break;
    case VTK_Z_LINE:
      this->Get1DOrientation(
          k, ext[4], ext[5], BlockFace::BACK, BlockFace::FRONT,
          BlockFace::NOT_ON_BLOCK_FACE );
      break;
    case VTK_XY_PLANE:
      this->Get1DOrientation(
          i, ext[0], ext[1], BlockFace::LEFT, BlockFace::RIGHT,
          BlockFace::NOT_ON_BLOCK_FACE);
      this->Get1DOrientation(
          j, ext[2], ext[3], BlockFace::BOTTOM, BlockFace::TOP,
          BlockFace::NOT_ON_BLOCK_FACE );
      break;
    case VTK_YZ_PLANE:
      this->Get1DOrientation(
          j, ext[2], ext[3], BlockFace::BOTTOM, BlockFace::TOP,
          BlockFace::NOT_ON_BLOCK_FACE );
      this->Get1DOrientation(
          k, ext[4], ext[5], BlockFace::BACK, BlockFace::FRONT,
          BlockFace::NOT_ON_BLOCK_FACE );
      break;
    case VTK_XZ_PLANE:
      this->Get1DOrientation(
          i, ext[0], ext[1], BlockFace::LEFT, BlockFace::RIGHT,
          BlockFace::NOT_ON_BLOCK_FACE);
      this->Get1DOrientation(
          k, ext[4], ext[5], BlockFace::BACK, BlockFace::FRONT,
          BlockFace::NOT_ON_BLOCK_FACE );
      break;
    case VTK_XYZ_GRID:
      this->Get1DOrientation(
          i, ext[0], ext[1], BlockFace::LEFT, BlockFace::RIGHT,
          BlockFace::NOT_ON_BLOCK_FACE);
      this->Get1DOrientation(
          j, ext[2], ext[3], BlockFace::BOTTOM, BlockFace::TOP,
          BlockFace::NOT_ON_BLOCK_FACE );
      this->Get1DOrientation(
          k, ext[4], ext[5], BlockFace::BACK, BlockFace::FRONT,
          BlockFace::NOT_ON_BLOCK_FACE );
      break;
    default:
      std::cout << "Data description is: " << this->DataDescription << "\n";
      std::cout.flush();
      assert( "pre: Undefined data-description!" && false );
    }
}

//------------------------------------------------------------------------------
void vtkStructuredGridConnectivity::CreateGhostLayers( const int N )
{
  // TODO: implement this
}
