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

    for( unsigned int nei=0; nei < this->Neighbors[gridID].size(); ++nei )
      {
      int NeiExtent[6];
      this->GetGridExtent( this->Neighbors[gridID][nei].NeighborID, NeiExtent );

      os << "\t";
      for( int i=0; i < 6; i+=2 )
        {
        os << " [";
        os << NeiExtent[i] << ", " << NeiExtent[i+1] << "] ";
        }

      os << " @ ";
      for( int i=0; i < 6; i+=2 )
        {
        os << " [";
        os << this->Neighbors[gridID][nei].OverlapExtent[ i ] << ", ";
        os << this->Neighbors[gridID][nei].OverlapExtent[ i+1 ] << "] ";
        }
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
  this->AcquireDataDescription( );
  if( this->DataDescription == VTK_EMPTY ||
      this->DataDescription == VTK_SINGLE_POINT )
    {
    return;
    }

  for( int i=0; i < this->NumberOfGrids; ++i )
    {
    for( int j=i+1; j < this->NumberOfGrids; ++j )
      {
      this->EstablishNeighbors(i,j);
      } // END for all j
    } // END for all i
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

  if( this->IsNodeInterior( i, j, k, ext ) )
    {
    vtkGhostArray::SetProperty(p,vtkGhostArray::INTERNAL );
    }
  else
    {
    if( this->IsNodeOnBoundary(i,j,k) )
      {
      vtkGhostArray::SetProperty(p,vtkGhostArray::BOUNDARY );
      }

    // Figure out if the point is shared and who owns the point
    vtkIdList *neiList = vtkIdList::New();
    this->SearchNeighbors( gridID, i, j, k, neiList );

    if( neiList->GetNumberOfIds() > 0 )
      {
      vtkGhostArray::SetProperty(p,vtkGhostArray::SHARED );

      for( vtkIdType nei=0; nei < neiList->GetNumberOfIds(); ++nei )
        {
        // If my gridID is not the smallest gridID that shares the point, then
        // I don't own the point.
        // The convention is that the grid with the smallest gridID will own the
        // point and all other grids should IGNORE it when computing statistics
        // etc.
        if( gridID > neiList->GetId( nei ) )
          {
          vtkGhostArray::SetProperty(p,vtkGhostArray::IGNORE );
          break;
          }
        } //END for all neis
      } // END if  shared
    neiList->Delete();
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

//  std::cout << "GRID: " << gridID << " has extent ";
//  std::cout.flush();
//  this->PrintExtent( GridExtent );
//  std::cout << std::endl;
//  std::cout.flush();

  int ijkmin[3];
  ijkmin[0] = GridExtent[0];
  ijkmin[1] = GridExtent[2];
  ijkmin[2] = GridExtent[4];

  int dims[3];
  vtkStructuredExtent::GetDimensions( GridExtent, dims );

  for( int i=GridExtent[0]; i <= GridExtent[1]; ++i )
    {
    for( int j=GridExtent[2]; j <= GridExtent[3]; ++j )
      {
      for( int k=GridExtent[4]; k <= GridExtent[5]; ++k )
        {
        // Convert global indices to local indices
        // TODO: handle arbitrary dimensions
        int li = i - ijkmin[0];
        int lj = j - ijkmin[1];
        int lk = k - ijkmin[2];

        int idx = vtkStructuredData::ComputePointId( dims, li, lj, lk );
        this->MarkNodeProperty(
            gridID,i,j,k,GridExtent,
            *nodesArray->GetPointer( idx ) );
        } // END for all k
      } // END for all j
    } // END for all i
}

//------------------------------------------------------------------------------
bool vtkStructuredGridConnectivity::IsNodeOnBoundary(
    const int i, const int j, const int k )
{
  bool status = false;

  switch( this->DataDescription )
    {
    case VTK_X_LINE:
       if( i==this->WholeExtent[0] || i==this->WholeExtent[1] )
         {
         status = true;
         }
       break;
     case VTK_Y_LINE:
       if( j==this->WholeExtent[2] || j==this->WholeExtent[3] )
         {
         status = true;
         }
       break;
     case VTK_Z_LINE:
       if( k==this->WholeExtent[4] || k==this->WholeExtent[5] )
         {
         status = true;
         }
       break;
     case VTK_XY_PLANE:
       if( i==this->WholeExtent[0] || i==this->WholeExtent[1] ||
           j==this->WholeExtent[2] || j==this->WholeExtent[3] )
         {
         status = true;
         }
       break;
     case VTK_YZ_PLANE:
       if( j==this->WholeExtent[2] || j==this->WholeExtent[3] ||
           k==this->WholeExtent[4] || k==this->WholeExtent[5] )
         {
         status = true;
         }
       break;
     case VTK_XZ_PLANE:
       if( i==this->WholeExtent[0] || i==this->WholeExtent[1] ||
           k==this->WholeExtent[4] || k==this->WholeExtent[5] )
         {
         status = true;
         }
       break;
     case VTK_XYZ_GRID:
       if( i==this->WholeExtent[0] || i==this->WholeExtent[1] ||
           j==this->WholeExtent[2] || j==this->WholeExtent[3] ||
           k==this->WholeExtent[4] || k==this->WholeExtent[5] )
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
    const int idx, int A[2], int B[2], int overlap[2], int orient[3] )
{
  assert( "pre: idx is out-of-bounds" && (idx >= 0) && (idx < 3) );
  if( overlap[0] > A[0] )
    {
    orient[ idx ] = vtkStructuredNeighbor::HI;
    }
  else
    {
    orient[ idx ] = vtkStructuredNeighbor::LO;
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
  int neighborOrientation[3];

  int overlapExtent[6];
  for( int ii=0; ii < 3; ++ii )
    {
    overlapExtent[ ii*2 ]     =  overlapExtent[ ii*2+1 ] = 0;
    neighborOrientation[ ii ] = vtkStructuredNeighbor::UNDEFINED;
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

    this->DetermineNeighborOrientation(
        idx, A, B, overlap, neighborOrientation );
    } // END for all dimensions

  this->SetNeighbors( i, j, overlapExtent );
}

//------------------------------------------------------------------------------
void vtkStructuredGridConnectivity::SetNeighbors(
            const int i, const int j, int overlapExtent[6] )
{
  vtkStructuredNeighbor Ni2j( j, overlapExtent );
  vtkStructuredNeighbor Nj2i( i, overlapExtent );

  this->Neighbors[ i ].push_back( Ni2j );
  this->Neighbors[ j ].push_back( Nj2i );

// BEGIN DEBUG
//  int iGridExtent[6];
//  int jGridExtent[6];
//  this->GetGridExtent( i, iGridExtent );
//  this->GetGridExtent( j, jGridExtent );
//
//  std::cout << "===\n";
//  this->PrintExtent( iGridExtent );
//  this->PrintExtent( jGridExtent );
//  std::cout << "\n\n";
//  this->PrintExtent( overlapExtent );
// END DEBUG

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
void vtkStructuredGridConnectivity::CreateGhostLayers( const int N )
{
  // TODO: implement this
}
