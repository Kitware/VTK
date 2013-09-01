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
#include "vtkCellData.h"
#include "vtkCellLinks.h"
#include "vtkCellType.h"
#include "vtkCollection.h"
#include "vtkDoubleArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkGenericCell.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeCursor.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkLine.h"
#include "vtkVoxel.h"
#include "vtkTimerLog.h"

#include <cassert>


vtkInformationKeyMacro( vtkHyperTreeGrid, LEVELS, Integer );
vtkInformationKeyMacro( vtkHyperTreeGrid, DIMENSION, Integer );
vtkInformationKeyRestrictedMacro( vtkHyperTreeGrid, SIZES, DoubleVector, 3 );

vtkStandardNewMacro( vtkHyperTreeGrid );

vtkCxxSetObjectMacro( vtkHyperTreeGrid, MaterialMask, vtkBitArray );
vtkCxxSetObjectMacro( vtkHyperTreeGrid, MaterialMaskIndex, vtkIdTypeArray );
vtkCxxSetObjectMacro( vtkHyperTreeGrid, XCoordinates, vtkDataArray );
vtkCxxSetObjectMacro( vtkHyperTreeGrid, YCoordinates, vtkDataArray );
vtkCxxSetObjectMacro( vtkHyperTreeGrid, ZCoordinates, vtkDataArray );

// Helpers to quickly fetch a HT at a given index or iterator
#define GetHTGHyperTreeAtIndexMacro( _obj_, _index_ )             \
  ( static_cast<vtkHyperTree*>(                                   \
  _obj_->HyperTrees.find( _index_ ) != _obj_->HyperTrees.end() ?  \
  _obj_->HyperTrees[ _index_ ] : 0 ) )

#define GetHyperTreeAtIndexMacro( _index_ ) \
  GetHTGHyperTreeAtIndexMacro( this, _index_ )

//-----------------------------------------------------------------------------
vtkHyperTreeGrid::vtkHyperTreeGrid()
{
  // Dual grid corners (primal grid leaf centers)
  this->Points = 0;
  this->Connectivity = 0;

  // Internal links
  this->Links = 0;

  // Grid topology
  this->GridSize[0] = 0;
  this->GridSize[1] = 0;
  this->GridSize[2] = 0;
  this->TransposedRootIndexing = false;

  // Grid parameters
  this->BranchFactor = 2;
  this->Dimension =  1;
  this->NumberOfChildren = 2;

  // Masked primal leaves
  this->MaterialMask = vtkBitArray::New();
  this->MaterialMaskIndex = 0;

  // Grid geometry
  this->XCoordinates = vtkDoubleArray::New();
  this->YCoordinates = vtkDoubleArray::New();
  this->ZCoordinates = vtkDoubleArray::New();

  // For data set API
  this->Voxel = vtkVoxel::New();
  this->Pixel = vtkPixel::New();
  this->Line = vtkLine::New();
}

//-----------------------------------------------------------------------------
vtkHyperTreeGrid::~vtkHyperTreeGrid()
{
  if ( this->MaterialMask )
    {
    this->MaterialMask->UnRegister( this );
    }

  if ( this->MaterialMaskIndex )
    {
    this->MaterialMaskIndex->UnRegister( this );
    }

  if ( this->XCoordinates )
    {
    this->XCoordinates->UnRegister( this );
    }

  if ( this->YCoordinates )
    {
    this->YCoordinates->UnRegister( this );
    }

  if ( this->ZCoordinates )
    {
    this->ZCoordinates->UnRegister( this );
    }

  if ( this->Voxel )
    {
    this->Voxel->UnRegister( this );
    }

  if ( this->Pixel )
    {
    this->Pixel->UnRegister( this );
    }

  if ( this->Line )
    {
    this->Line->UnRegister( this );
    }

  this->DeleteInternalArrays();
  this->DeleteTrees();
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "Dimension: " << this->Dimension << endl;
  os << indent << "GridSize: "
     << this->GridSize[0] <<","
     << this->GridSize[1] <<","
     << this->GridSize[2] << endl;

  if ( this->XCoordinates )
    {
    this->XCoordinates->PrintSelf( os, indent.GetNextIndent() );
    }
  if ( this->YCoordinates )
    {
    this->YCoordinates->PrintSelf( os, indent.GetNextIndent() );
    }
  if ( this->ZCoordinates )
    {
    this->ZCoordinates->PrintSelf( os, indent.GetNextIndent() );
    }
}

//-----------------------------------------------------------------------------
// Description:
// Return what type of dataset this is.
int vtkHyperTreeGrid::GetDataObjectType()
{
  return VTK_HYPER_TREE_GRID;
}

//-----------------------------------------------------------------------------

void vtkHyperTreeGrid::DeleteTrees()
{
  if ( this->HyperTrees.size() )
    {
    vtkHyperTreeIterator it;
    this->InitializeTreeIterator( it );
    while ( vtkHyperTree* tree = it.GetNextTree() )
      {
      tree->UnRegister( this );
      }
    this->HyperTrees.clear();
    }
}

//-----------------------------------------------------------------------------
// Description:
// Copy the geometric and topological structure of a hyper tree grid
// object.
void vtkHyperTreeGrid::CopyStructure( vtkDataSet* ds )
{
  assert( "pre: ds_exists" && ds!=0 );
  assert( "pre: same_type" && vtkHyperTreeGrid::SafeDownCast(ds)!=0 );

  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast( ds );

  // Copy grid parameters
  this->Dimension = htg->Dimension;
  this->BranchFactor = htg->BranchFactor;
  this->NumberOfChildren = htg->NumberOfChildren;
  this->TransposedRootIndexing = htg->TransposedRootIndexing;
  memcpy( this->GridSize, htg->GetGridSize(), 3 * sizeof( int ) );

  // Un-register existing trees
  DeleteTrees();

  // Shallow copy and register new trees
  this->HyperTrees = htg->HyperTrees;

  if ( this->HyperTrees.size() )
    {
    vtkHyperTreeIterator it;
    this->InitializeTreeIterator( it );
    while ( vtkHyperTree* tree = it.GetNextTree() )
      {
      tree->Register( this );
      }
    }

  this->DeleteInternalArrays();

  if ( htg->Points )
    {
    this->Points = htg->Points;
    this->Points->Register( this );
    }

  if ( htg->Connectivity )
    {
    this->Connectivity = htg->Connectivity;
    this->Connectivity->Register( this );
    }

  if ( htg->Links )
    {
    this->Links = htg->Links;
    this->Links->Register( this );
    }

  // Shallow copy masked leaf IDs
  this->SetMaterialMask( htg->MaterialMask );

  // Shallow copy masked leaf IDs
  this->SetMaterialMaskIndex( htg->MaterialMaskIndex );

  // Shallow copy coordinates
  this->SetXCoordinates( htg->XCoordinates );
  this->SetYCoordinates( htg->YCoordinates );
  this->SetZCoordinates( htg->ZCoordinates );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::SetGridSize( unsigned int n[3] )
{
  if( this->GridSize[0] == n[0] && this->GridSize[1] == n[1] && this->GridSize[2] == n[2] )
    {
    return;
    }
  this->GridSize[0] = n[0];
  this->GridSize[1] = n[1];
  this->GridSize[2] = n[2];

  this->Modified();
}

//-----------------------------------------------------------------------------
// Set the dimension of the tree with `dim'. See GetDimension() for details.
// \pre valid_dim: dim >= 1 && dim <= 3
// \post dimension_is_set: GetDimension()==dim
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
// \pre valid_dim: factor == 2 or factor == 3;
// \post dimension_is_set: GetBranchFactor()==dim
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
void vtkHyperTreeGrid::GenerateTrees()
{
  // Clean up existing trees
  this->DeleteTrees();

  // Generate concrete instance of hyper tree and append it to list of roots
  vtkIdType nr = this->GetNumberOfTrees();

  for ( vtkIdType r = 0; r < nr; ++ r )
    {
    vtkHyperTree* tree = vtkHyperTree::CreateInstance( this->BranchFactor, this->Dimension );
    this->HyperTrees[ this->MaterialMaskIndex ? this->MaterialMaskIndex->GetValue(r) : r ] = tree;
    } //r

  this->Modified();
  this->DeleteInternalArrays();
}

//----------------------------------------------------------------------------
void vtkHyperTreeGrid::ComputeBounds()
{
  // Retrieve coordinate arrays
  vtkDataArray* coords[3];
  coords[0] = this->XCoordinates;
  coords[1] = this->YCoordinates;
  coords[2] = this->ZCoordinates;
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
vtkIdType vtkHyperTreeGrid::GetNumberOfLeaves()
{
  vtkIdType nLeaves = 0;
  vtkHyperTreeIterator it;
  this->InitializeTreeIterator( it );
  while ( vtkHyperTree* tree = it.GetNextTree() )
    {
    nLeaves += tree->GetNumberOfLeaves();
    }
  return nLeaves;
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::GetNumberOfLevels( vtkIdType id )
{
  vtkHyperTree* tree = GetHyperTreeAtIndexMacro( id );
  return tree ? tree->GetNumberOfLevels() : 0;
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::GetNumberOfTrees()
{
  return this->MaterialMaskIndex ?
    this->MaterialMaskIndex->GetNumberOfTuples() :
    this->GridSize[0] * this->GridSize[1] * this->GridSize[2];
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::InitializeTreeIterator( vtkHyperTreeIterator& it )
{
  it.Initialize( this );
}

//-----------------------------------------------------------------------------
vtkHyperTreeCursor* vtkHyperTreeGrid::NewCursor( vtkIdType id )
{
  vtkHyperTree* tree = GetHyperTreeAtIndexMacro( id );
  return tree ? tree->NewCursor() : 0;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::SubdivideLeaf( vtkHyperTreeCursor* leaf, vtkIdType id )
{
  assert( "pre: leaf_exists" && leaf );
  assert( "pre: is_a_leaf" && leaf->IsLeaf() );
  vtkHyperTree* tree = GetHyperTreeAtIndexMacro( id );
  if ( tree )
    {
    tree->SubdivideLeaf( leaf );
    this->DeleteInternalArrays();
    }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::Initialize()
{
  if ( this->HyperTrees.size() )
    {
    vtkHyperTreeIterator it;
    this->InitializeTreeIterator( it );
    while ( vtkHyperTree* tree = it.GetNextTree() )
      {
      tree->Initialize();
      }
    }

  this->DeleteInternalArrays();
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGrid::GetMaxCellSize()
{
  switch( this->Dimension )
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
      assert( "check: impossible_case" && 0 );
      return 0;
    }
}

//-----------------------------------------------------------------------------
// Description:
// Shallow and Deep copy.
void vtkHyperTreeGrid::ShallowCopy( vtkDataObject* src )
{
  assert( "src_same_type" && vtkHyperTreeGrid::SafeDownCast( src ) );
  this->CopyStructure(vtkHyperTreeGrid::SafeDownCast( src ) );

  // Call superclass
  this->Superclass::ShallowCopy( src );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::DeepCopy( vtkDataObject* src )
{
  assert( "src_same_type" && vtkHyperTreeGrid::SafeDownCast( src ) );
  this->CopyStructure( vtkHyperTreeGrid::SafeDownCast( src ) );

  // Call superclass
  this->Superclass::DeepCopy( src );
}

//=============================================================================
// DataSet API that returns dual grid.

//-----------------------------------------------------------------------------
// Description:
// Return the number of leaves.
// \post positive_result: result>=0
vtkIdType vtkHyperTreeGrid::GetNumberOfCells()
{
  this->ComputeDualGrid();
  return this->GetConnectivity()->GetNumberOfTuples();
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::GetNumberOfPoints()
{
  return this->GetNumberOfLeaves();
}

//-----------------------------------------------------------------------------
double* vtkHyperTreeGrid::GetPoint( vtkIdType ptId )
{
  this->ComputeDualGrid();
  vtkPoints* leafCenters = this->GetPoints();
  assert( "Index out of bounds." &&
          ptId >= 0 && ptId < leafCenters->GetNumberOfPoints() );
  return leafCenters->GetPoint( ptId );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetPoint( vtkIdType ptId, double x[3] )
{
  this->ComputeDualGrid();
  vtkPoints* leafCenters = this->GetPoints();
  assert( "Index out of bounds." &&
          ptId >= 0 && ptId < leafCenters->GetNumberOfPoints() );
  leafCenters->GetPoint( ptId, x );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetCell( vtkIdType cellId, vtkCell* cell )
{
  assert( "Null cell ptr." && cell != 0 );

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
  vtkCell* cell = 0;
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
    }

  GetCell( cellId, cell );
  return cell;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetCell( vtkIdType cellId, vtkGenericCell* cell )
{
  assert( "GetCell on null cell." && cell != 0 );
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
    }

  GetCell( cellId, static_cast<vtkCell*>( cell ) );
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGrid::GetCellType( vtkIdType vtkNotUsed(cellId) )
{
  switch ( this->Dimension )
    {
    case 3:
      return VTK_VOXEL; // hexahedron=8 points
    case 2:
      return VTK_PIXEL; // quad=4 points
    case 1:
      return VTK_LINE;  // line=2 points
    default:
      assert( "post: positive_result" && false );
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
  memcpy( ptIds->GetPointer(0), ptr, numPts * sizeof( vtkIdType ) );
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
  npts = static_cast<vtkIdType>( 1 << this->Dimension );
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
  this->Links->UnRegister( this );
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

  int numPts = ptIds->GetNumberOfIds();
  int minNumCells = VTK_INT_MAX;
  vtkIdType* pts = ptIds->GetPointer( 0 );
  vtkIdType* minCells = 0;
  vtkIdType minPtId = 0;
  // Find the point used by the fewest number of cells
  for ( int i = 0; i < numPts; i++ )
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

  if ( minNumCells == VTK_INT_MAX && numPts == 0 )
    {
    vtkErrorMacro( "input point ids empty." );
    minNumCells = 0;
    }

  // For all cells that contNow for each cell, see if it contains all the points
  // in the ptIds list.
  for ( int i = 0; i < minNumCells; i++ )
    {
    // Do not include current cell
    if ( minCells[i] != cellId )
      {
      vtkIdType *cellPts;
      vtkIdType npts;
      this->GetCellPoints( minCells[i], npts, cellPts );
      // Iterate over all points in input cell
      int match = 1;
      for ( int j = 0; j < numPts && match; j++ )
        {
        // Skip point with index minPtId which is contained by current cell
        if ( pts[j] != minPtId )
          {
          // Iterate over all points in current cell
          int k;
          for ( match = k = 0; k < npts; ++ k )
            {
            if ( pts[j] == cellPts[k] )
              {
              // A match was found
              match = 1;
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

  vtkHyperTreeSimpleCursor cursor;
  int pos[] = { 0, 0, 0 };
  cursor.Initialize( this, index, pos );

  // Geometry of the cell
  double origin[3];
  this->XCoordinates->GetTuple( ix, origin );
  this->YCoordinates->GetTuple( iy, origin + 1 );
  this->ZCoordinates->GetTuple( iz, origin + 2 );

  double extreme[3];
  this->XCoordinates->GetTuple( ix + 1, extreme );
  this->YCoordinates->GetTuple( iy + 1, extreme + 1);
  this->ZCoordinates->GetTuple( iz + 1, extreme + 2 );

  double size[3];
  size[0] = extreme[0] - origin[0];
  size[1] = extreme[1] - origin[1];
  size[2] = extreme[2] - origin[2];

  return this->RecursiveFindPoint( x, &cursor, origin, size );
}

//----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::RecursiveFindPoint( double x[3],
                                                vtkHyperTreeSimpleCursor* cursor,
                                                double* origin,
                                                double* size )
{
  if ( cursor->IsLeaf() )
    {
    return cursor->GetLeafIndex();
    }

  vtkHyperTreeSimpleCursor newCursor = *cursor;
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
  newCursor.ToChild( child );

  return this->RecursiveFindPoint( x, &newCursor, newOrigin, newSize );
}

//----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::FindCell( double x[3], vtkCell* cell,
                                      vtkGenericCell* gencell, vtkIdType cellId,
                                      double tol2, int& subId, double pcoords[3],
                                      double* weights )
{
  vtkIdType ptId = this->FindPoint( x );
  if ( ptId < 0 )
    {
    // Return invalid Id if point is completely outside of data set
    return -1;
    }

  vtkIdList* cellIds = vtkIdList::New();
  cellIds->Allocate( 8, 100 );
  this->GetPointCells( ptId, cellIds );
  if ( cellIds->GetNumberOfIds() <= 0 )
    {
    cellIds->UnRegister( this );
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
    double dx[3];
    memcpy( dx, x, 3 * sizeof(double) );
    if ( ( gencell &&
           gencell->EvaluatePosition( dx, closestPoint, subId,
                                      pcoords, dist2, weights ) == 1
           && dist2 <= tol2 )  ||
         ( !gencell &&
           cell->EvaluatePosition( dx, closestPoint, subId,
                                   pcoords, dist2, weights ) == 1
           && dist2 <= tol2 ) )
      {
      cellIds->UnRegister( this );
      return cellId;
      }
    }

  // This should never happen.
  vtkErrorMacro( "Could not find cell." );
  return -1;
}

//----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::FindCell( double x[3], vtkCell* cell,
                                      vtkIdType cellId, double tol2,
                                      int& subId, double pcoords[3],
                                      double* weights )
{
  return this->FindCell( x, cell, NULL, cellId, tol2, subId, pcoords, weights );
}

//----------------------------------------------------------------------------
unsigned long vtkHyperTreeGrid::GetActualMemorySize()
{
  unsigned long size = this->vtkDataSet::GetActualMemorySize();

  vtkHyperTreeIterator it;
  this->InitializeTreeIterator( it );
  while ( vtkHyperTree* tree = it.GetNextTree() )
    {
    size += tree->GetActualMemorySize();
    }

  // Approximate map memory size
  size += static_cast<unsigned long>(
    ( this->HyperTrees.size() * sizeof(vtkIdType) * 3 ) / 1024 );

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

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetLevelZeroCoordsFromIndex( vtkIdType index,
                                                    unsigned int &i,
                                                    unsigned int &j,
                                                    unsigned int &k )
{
  if ( !this->TransposedRootIndexing )
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
void vtkHyperTreeGrid::InitializeSuperCursor( vtkHyperTreeGridSuperCursor* sc,
                                              vtkIdType index )
{
  unsigned int i, j, k;
  this->GetLevelZeroCoordsFromIndex( index, i, j, k );
  // Initialize center cursor
  this->InitializeSuperCursor( sc, i, j, k, index );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::InitializeSuperCursor( vtkHyperTreeGridSuperCursor* sc,
                                              unsigned int i,
                                              unsigned int j,
                                              unsigned int k,
                                              vtkIdType index )
{
  // Location and size of the middle cursor/node
  double origin[3];
  this->XCoordinates->GetTuple( i, origin );
  this->YCoordinates->GetTuple( j, origin + 1);
  this->ZCoordinates->GetTuple( k, origin + 2 );

  double extreme[3];
  this->XCoordinates->GetTuple( i + 1, extreme );
  this->YCoordinates->GetTuple( j + 1, extreme + 1);
  this->ZCoordinates->GetTuple( k + 1, extreme + 2 );

  memcpy( sc->Origin, origin, 3 * sizeof( double ) );
  sc->Size[0] = extreme[0] - origin[0];
  sc->Size[1] = extreme[1] - origin[1];
  sc->Size[2] = extreme[2] - origin[2];

  // Initialize middle cursors and bounds for other cursors
  sc->NumberOfCursors = 3;
  for ( unsigned int c = 1; c < this->Dimension; ++ c )
    {
    sc->NumberOfCursors *= 3;
    }
  sc->MiddleCursorId = sc->NumberOfCursors / 2;

  int lowI = -1;
  int highI = 1;
  int lowJ = this->Dimension > 1 ? -1 : 0;
  int highJ = this->Dimension > 1 ? 1 : 0;
  int lowK = this->Dimension > 2 ? -1 : 0;
  int highK = this->Dimension > 2 ? 1 : 0;
  switch ( this->Dimension )
    {
    case 3:
      lowK = ( k == 0 ) ? 0 : -1;
      highK = ( k + 1 < this->GridSize[2] ) ? 1 : 0;
    case 2:
      lowJ = ( j == 0 ) ? 0 : -1;
      highJ = ( j + 1 < this->GridSize[1] ) ? 1 : 0;
    case 1:
      lowI = ( i == 0 ) ? 0 : -1;
      highI = ( i + 1 < this->GridSize[0] ) ? 1 : 0;
    }

  // Initialize all connectivity cursors
  for ( int ck = lowK; ck <= highK; ++ ck )
    {
    for ( int cj = lowJ; cj <= highJ; ++ cj )
      {
      for ( int ci = lowI; ci <= highI; ++ ci )
        {
        int pos[3] = { ci, cj, ck };
        int d = ci + 3 * cj + 9 * ck;
        sc->GetCursor( d )->Initialize( this, index, pos );
        } // i
      } // j
    } // k
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::InitializeSuperCursorChild( vtkHyperTreeGridSuperCursor* parent,
                                                   vtkHyperTreeGridSuperCursor* child,
                                                   unsigned int childIdx )
{
  // Retrieve child's parameters that are identical to parent's ones
  child->NumberOfCursors = parent->NumberOfCursors;
  child->MiddleCursorId = parent->MiddleCursorId;

  // Compute size of child
  child->Size[0] = parent->Size[0] / double( this->BranchFactor );
  child->Size[1] = parent->Size[1] / double( this->BranchFactor );
  child->Size[2] = parent->Size[2] / double( this->BranchFactor );

  // Compute origin of child
  unsigned int x, y, z;
  if ( this->BranchFactor == 2 )
    {
    x = childIdx & 1;
    y = ( childIdx & 2 ) >> 1;
    z = ( childIdx & 4 ) >> 2;
    }
  else
    {
    div_t d = div( childIdx, 9 );
    z = d.quot;
    y = d.rem / 3;
    x = childIdx % 3;
    }
  child->Origin[0] = parent->Origin[0] + ( x * child->Size[0] );
  child->Origin[1] = parent->Origin[1] + ( y * child->Size[1] );
  child->Origin[2] = parent->Origin[2] + ( z * child->Size[2] );

  // Move each cursor in the superCursor down to a child
  vtkSuperCursorEntry* cursorPtr =
    this->SuperCursorTraversalTable + ( childIdx * 27 );
  for ( int cursorIdx = 0; cursorIdx < child->NumberOfCursors; ++ cursorIdx )
    {
    // Extract the parent and child of the new node from the traversal table
    // Child is encoded in the first three bits for all dimensions
    int tParent = cursorPtr[cursorIdx].Parent;
    child->Cursors[cursorIdx] = parent->Cursors[tParent];
    if ( parent->Cursors[tParent].GetTree()
      && ! parent->Cursors[tParent].IsLeaf() )
      {
      // Move to child
      child->Cursors[cursorIdx] = parent->Cursors[tParent];
      int tChild = cursorPtr[cursorIdx].Child;
      child->Cursors[cursorIdx].ToChild( tChild );
      }
    }
}

//-----------------------------------------------------------------------------
// Traverse tree with 3x3x3 super cursor.  Center cursor generates dual point
// Smallest leaf (highest level) owns corners/dual cell.  Ties are given to
// smallest index (z,y,x order)
// post: Generate Points and Connectivity.
void vtkHyperTreeGrid::ComputeDualGrid()
{
  // Check if we can break out early
  vtkIdType numPoints = this->GetNumberOfPoints();
  if ( this->Points )
    {
    if ( this->Points->GetNumberOfPoints() == numPoints )
      {
      return;
      }
    this->Points->UnRegister( this );
    this->Connectivity->UnRegister( this );
    }

  vtkTimerLog* timer = vtkTimerLog::New();
  timer->StartTimer();

  // Primal cell centers are dual points
  this->Points = vtkPoints::New();
  this->Points->SetNumberOfPoints( numPoints );

  this->Connectivity = vtkIdTypeArray::New();
  int numVerts = 1 << this->Dimension;
  this->Connectivity->SetNumberOfComponents( numVerts );

  // Create an array of cursors that occupy 1 3x3x3 neighborhood
  // Will traverse the tree as one
  // NB: Lower dimensions will not use them all
  this->GenerateSuperCursorTraversalTable();

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
            if ( !tree )
              {
              continue;
              }

            // Compute scale along z-axis and set tree scale
            scale[2] = this->ZCoordinates->GetTuple1( k + 1 ) -
              this->ZCoordinates->GetTuple1( k );
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
            if ( !tree )
              {
              continue;
              }

            // Compute scale along x-axis and set tree scale
            scale[0] = this->XCoordinates->GetTuple1( i + 1 ) -
              this->XCoordinates->GetTuple1( i );
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

  // Iterate over all hyper trees
  vtkIdType index;
  vtkHyperTreeIterator it;
  this->InitializeTreeIterator( it );
  while ( it.GetNextTree( index ) )
    {
    // Storage for super cursors
    vtkHyperTreeGridSuperCursor superCursor;

    // Initialize center cursor
    this->InitializeSuperCursor( &superCursor, index );

    // Traverse and populate dual recursively
    this->TraverseDualRecursively( &superCursor, 0 );
    } // it

  // Adjust dual points as needed to fit the primal boundary
  for ( unsigned int d = 0; d < this->Dimension; ++ d )
    {
    // Iterate over all adjustments for current dimension
    for ( std::map<vtkIdType, double>::const_iterator itps =
      this->PointShifts[d].begin();
      itps != this->PointShifts[d].end(); ++ itps )
      {
      double pt[3];
      this->Points->GetPoint( itps->first, pt );
      pt[d] += itps->second;
      this->Points->SetPoint( itps->first, pt );
      } // it
    this->PointShifts[d].clear();
    } // d
  this->PointShifted.clear();

  timer->StopTimer();
  cerr << "Internal dual update : " << timer->GetElapsedTime() << endl;
  timer->Delete();
}

//-----------------------------------------------------------------------------
// Iterate over leaves.  Generate dual point.  Highest level (smallest leaf)
// owns the corner and generates that dual cell.
void vtkHyperTreeGrid::TraverseDualRecursively(
  vtkHyperTreeGridSuperCursor* superCursor, unsigned int level )
{
  // Get cursor at super cursor center
  vtkHyperTreeSimpleCursor* cursor0 = superCursor->GetCursor( 0 );

  // Retrieve global index of center cursor
  vtkIdType id0 = cursor0->GetGlobalNodeIndex();

  // Initialize non leaf point coordinates
  double pt[3] = { 0., 0., 0. };
  for ( unsigned int d = 0; d < this->Dimension; ++ d )
    {
    pt[d] = superCursor->Origin[d] + superCursor->Size[d] / 2.;
    }
  // Retrieve global index of center cursor
  this->Points->SetPoint( id0, pt );
  if ( !cursor0->IsLeaf() )
    {
    // If cursor 0 is not at leaf, recurse to all children
    for ( unsigned int child = 0; child < this->NumberOfChildren; ++ child )
      {
      vtkHyperTreeGridSuperCursor newSuperCursor;
      this->InitializeSuperCursorChild( superCursor, &newSuperCursor, child );
      this->TraverseDualRecursively( &newSuperCursor, level + 1 );
      }
    }
  else
    {
    // Center is a leaf, create a dual point
    if ( this->GetMaterialMask()->GetTuple1( id0 ) )
      {
      this->TraverseDualMaskedLeaf( superCursor );
      }
    else
      {
      this->TraverseDualLeaf( superCursor );
      }
    }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::TraverseDualMaskedLeaf(
  vtkHyperTreeGridSuperCursor* superCursor )
{
  // Get cursor at super cursor center
  vtkHyperTreeSimpleCursor* cursor0 = superCursor->GetCursor( 0 );

  // Check across D-face neighbors whether point must be adjusted
  unsigned int f = 1;
  for ( unsigned int d = 0; d < this->Dimension; ++ d, f *= 3 )
    {
    // For each direction, check both orientations
    for ( int o = -1; o < 2; o += 2 )
      {
      // Retrieve face neighbor cursor
      vtkHyperTreeSimpleCursor* cursor = superCursor->GetCursor( o * f );

      // Detect faces shared by an unmasked cell, break ties at same level
      if ( cursor->GetTree()
           && cursor->IsLeaf()
           && cursor->GetLevel() < cursor0->GetLevel() )
        {
        vtkIdType id = cursor->GetGlobalNodeIndex();
        if ( ! this->GetMaterialMask()->GetTuple1( id ) )
          {
          // Move to corresponding D-face
          this->PointShifted[id] = true;
          this->PointShifts[d][id] = - o
            * this->ReductionFactors[cursor->GetLevel()]
            * cursor->GetTree()->GetScale( d );
          }
        } // if cursor
      } // o
    } // d

  switch ( this->Dimension )
    {
    case 2:
      {
      // Check across (D-1)-face neighbors (corners)
      for ( int o2 = -1; o2 < 2; o2 += 2 )
        {
        int c = o2 + 3;
        for ( int o1 = -1; o1 < 2; o1 += 2 )
          {
          vtkHyperTreeSimpleCursor* cursor = superCursor->GetCursor( o1 * c );
          if ( cursor->GetTree()
               &&
               cursor->IsLeaf()
               && cursor->GetLevel() < cursor0->GetLevel() )
            {
            vtkIdType id = cursor->GetGlobalNodeIndex();
            if ( ! this->GetMaterialMask()->GetTuple1( id )
                 && ! this->PointShifted[id] )
              {
              // Move to corresponding corner
              double halfL[3];
              cursor->GetTree()->GetScale( halfL );
              double fac = this->ReductionFactors[cursor->GetLevel()];
              this->PointShifts[0][id] = - o1 * o2 * fac * halfL[0];
              this->PointShifts[1][id] = - o1 * fac * halfL[1];
              }
            } // if cursor
          } // o1
        } // o2
      break;
      } // case 2
    case 3:
      {
      // Check across (D-1)-face neighbors (edges)
      int tpa1 = 1;
      for ( int a1 = 0; a1 < 2; ++ a1, tpa1 *= 3 )
        {
        int tpa2 = 3 * tpa1;
        for ( int a2 = a1 + 1; a2 < 3; ++ a2, tpa2 *= 3 )
          {
          for ( int o2 = -1; o2 < 2; o2 += 2 )
            {
            int c  = tpa1 * o2 + tpa2;
            for ( int o1 = -1; o1 < 2; o1 += 2 )
              {
              vtkHyperTreeSimpleCursor* cursor = superCursor->GetCursor( o1 * c );
              if ( cursor->GetTree()
                   &&
                   cursor->IsLeaf()
                   && cursor->GetLevel() < cursor0->GetLevel() )
                {
                vtkIdType id = cursor->GetGlobalNodeIndex();
                if ( ! this->GetMaterialMask()->GetTuple1( id )
                     && ! this->PointShifted[id] )
                  {
                  // Move to corresponding edge
                  double halfL[3];
                  cursor->GetTree()->GetScale( halfL );
                  double fac = this->ReductionFactors[cursor->GetLevel()];
                  this->PointShifts[a1][id] = - o1 * o2 * fac * halfL[a1];
                  this->PointShifts[a2][id] = - o1 * fac * halfL[a2];
                  this->PointShifted[id] = true;
                  }
                } // if cursor
              } // o1
            } // o2
          } // a2
        } // a1

      // Check across (D-2)-face neighbors (corners)
      for ( int o3 = -1; o3 < 2; o3 += 2 )
        {
        for ( int o2 = -1; o2 < 2; o2 += 2 )
          {
          int c = o2 * ( o3 + 3 ) + 9;
          for ( int o1 = -1; o1 < 2; o1 += 2 )
            {
            vtkHyperTreeSimpleCursor* cursor = superCursor->GetCursor( o1 * c );
            if ( cursor->GetTree()
                 &&
                 cursor->IsLeaf()
                 && cursor->GetLevel() < cursor0->GetLevel() )
              {
              vtkIdType id = cursor->GetGlobalNodeIndex();
              if ( ! this->GetMaterialMask()->GetTuple1( id )
                   && ! this->PointShifted[id] )
                {
                // Move to corresponding corner
                double halfL[3];
                cursor->GetTree()->GetScale( halfL );
                double fac = this->ReductionFactors[cursor->GetLevel()];
                this->PointShifts[0][id] = - o1 * o2 * o3 * fac * halfL[0];
                this->PointShifts[1][id] = - o1 * o2 * fac * halfL[1];
                this->PointShifts[2][id] = - o1 * fac * halfL[2];
                this->PointShifted[id] = true;
                }
              } // if cursor
            } // o1
          } // o2
        } // o3
      break;
      } // case 3
    } // switch  ( this->Dimension )
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::TraverseDualLeaf( vtkHyperTreeGridSuperCursor* superCursor )
{
  // Get cursor at super cursor center
  vtkHyperTreeSimpleCursor* cursor0 = superCursor->GetCursor( 0 );

  // Initialize dual point coordinates and D-face adjustment flag
  double pt[] = { 0., 0., 0. };
  double halfL[] = { 0., 0., 0. };
  bool movedToDFace = false;

  // In 1D:
  //   (D-0)-faces are corners, neighbors are +/- 1
  //   (D-1)-faces do not exist
  //   (D-2)-faces do not exist
  // In 2D:
  //   (D-0)-faces are edges, neighbors are +/- 1, 3
  //   (D-1)-faces are corners, neighbors are +/- 2, 4
  //   (D-2)-faces do not exist
  // In 3D:
  //   (D-0)-faces are faces, neighbors are +/- 1, 3, 9
  //   (D-1)-faces are edges, neighbors are +/- 2, 4, 6, 8, 10, 12
  //   (D-2)-faces are corners, neighbors are +/-  5, 7, 11, 13

  // Check across D-face neighbors whether point must be adjusted
  unsigned int f = 1;
  for ( unsigned int d = 0; d < this->Dimension; ++ d, f *= 3 )
    {
    // Start at center
    halfL[d] = .5 * superCursor->Size[d];
    pt[d] = superCursor->Origin[d] + halfL[d];

    // Check
    for ( int o = -1; o < 2; o += 2 )
      {
      vtkHyperTreeSimpleCursor* cursor = superCursor->GetCursor( o * f );
      if ( ! cursor->GetTree()
           ||
           ( cursor->IsLeaf()
           && this->GetMaterialMask()->GetTuple1( cursor->GetGlobalNodeIndex() ) ) )
        {
        // Move to corresponding D-face
        pt[d] += o * halfL[d];
        movedToDFace = true;
        } // if cursor
      } // o
    } // d

  // Only when point was not moved to D-face, check D-1 and D-2 neighbors
  if ( ! movedToDFace )
    {
    switch ( this->Dimension )
      {
      case 2:
        {
        // Check across (D-1)-face neighbors (corners)
        for ( int o2 = -1; o2 < 2; o2 += 2 )
          {
          int c = o2 + 3;
          for ( int o1 = -1; o1 < 2; o1 += 2 )
            {
            vtkHyperTreeSimpleCursor* cursor = superCursor->GetCursor( o1 * c );
            if ( ! cursor->GetTree()
                 ||
                 ( cursor->IsLeaf()
                   && this->GetMaterialMask()->GetTuple1( cursor->GetGlobalNodeIndex() ) ) )
              {
              // Move to corresponding corner
              pt[0] += o1 * o2 * halfL[0];
              pt[1] += o1 * halfL[1];
              } // if cursor
            } // o1
          } // o2
        break;
        } // case 2
      case 3:
        {
        // Initialize edge adjustment flag
        bool movedToEdge = false;

        // Check across (D-1)-face neighbors (edges)
        int tpa1 = 1;
        for ( int a1 = 0; a1 < 2; ++ a1, tpa1 *= 3 )
          {
          int tpa2 = 3 * tpa1;
          for ( int a2 = a1 + 1; a2 < 3; ++ a2, tpa2 *= 3 )
            {
            for ( int o2 = -1; o2 < 2; o2 += 2 )
              {
              int c  = tpa1 * o2 + tpa2;
              for ( int o1 = -1; o1 < 2; o1 += 2 )
                {
                vtkHyperTreeSimpleCursor* cursor = superCursor->GetCursor( o1 * c );
                if ( ! cursor->GetTree()
                     ||
                     ( cursor->IsLeaf()
                       && this->GetMaterialMask()->GetTuple1( cursor->GetGlobalNodeIndex() ) ) )
                  {
                  // Move to corresponding edge
                  pt[a1] += o1 * o2 * halfL[a1];
                  pt[a2] += o1 * halfL[a2];
                  movedToEdge = true;
                  } // if cursor
                } // o1
              } // o2
            } // a2
          } // a1

        // Only when point was not moved to edge, check across corners
        if ( ! movedToEdge )
          {
          for ( int o3 = -1; o3 < 2; o3 += 2 )
            {
            for ( int o2 = -1; o2 < 2; o2 += 2 )
              {
              int c = o2 * ( o3 + 3 ) + 9;
              for ( int o1 = -1; o1 < 2; o1 += 2 )
                {
                vtkHyperTreeSimpleCursor* cursor = superCursor->GetCursor( o1 * c );
                if ( ! cursor->GetTree()
                     ||
                     ( cursor->IsLeaf()
                       && this->GetMaterialMask()->GetTuple1( cursor->GetGlobalNodeIndex() ) ) )
                  {
                  // Move to corresponding corner
                  pt[0] += o1 * o2 * o3 * halfL[0];
                  pt[1] += o1 * o2 * halfL[1];
                  pt[2] += o1 * halfL[2];
                  } // if cursor
                } // o1
              } // o2
            } // o3
          } // if ( ! movedToEdge )
        break;
        } // case 3
      } // switch ( this->Dimension )
    } // if ( ! movedToDFace )

  // Retrieve global index of center cursor
  vtkIdType id0 = cursor0->GetGlobalNodeIndex();

  // Insert dual point corresponding to current primal cell
  this->Points->SetPoint( id0, pt );

  // If cell is masked, terminate recursion, no dual cell will be generated
  if ( this->GetMaterialMask()->GetTuple1( id0 ) )
    {
    return;
    }

  // Now see if the center leaf owns any of the corners
  // If it does, create the dual cell
  // Iterate over the corners around the middle leaf
  int numLeavesCorners = 1 << this->Dimension;
  for ( int cornerIdx = 0; cornerIdx < numLeavesCorners; ++ cornerIdx )
    {
    bool owner = true;
    vtkIdType leaves[8];
    // Iterate over every leaf touching the corner
    for ( int leafIdx = 0; leafIdx < numLeavesCorners && owner; ++ leafIdx )
      {
      // Compute the cursor index into the superCursor
      int cursorIdx = 0;
      switch ( this->Dimension )
        {
        // Warning: Run through is intended! Do NOT add break statements
        case 3:
          cursorIdx += 9 * ( ( ( cornerIdx >> 2 ) & 1 ) + ( ( leafIdx >> 2 ) & 1 ) );
        case 2:
          cursorIdx += 3 * ( ( ( cornerIdx >> 1 ) & 1 ) + ( ( leafIdx >> 1 ) & 1 ) );
        case 1:
          cursorIdx += ( cornerIdx & 1) + ( leafIdx & 1);
        }

      // Collect the leaf indices for the dual cell
      leaves[leafIdx] = superCursor->Cursors[cursorIdx].GetGlobalNodeIndex();

      // Compute if the mid leaf owns the corner
      if ( cursorIdx != superCursor->MiddleCursorId )
        {
        vtkHyperTreeSimpleCursor* cursor = superCursor->Cursors + cursorIdx;
        if ( ! cursor->GetTree() || ! cursor->IsLeaf() )
          {
          // If neighbor leaf is out of bounds or has not been
          // refined to a leaf, this leaf does not own the corner
          owner = false;
          }
        else if ( this->GetMaterialMask()->GetTuple1( cursor->GetGlobalNodeIndex() ) )
          {
          owner = false;
          }
        else if ( cursor->GetLevel() == cursor0->GetLevel()
                  && superCursor->MiddleCursorId < cursorIdx )
          {
          // A level tie is broken in favor of the largest index
          // All points are set before defining the cell
          owner = false;
          }
        }
      } // leafIdx

    if ( owner )
      {
      this->Connectivity->InsertNextTupleValue( leaves );
      }
    } // cornerIdx
}

//----------------------------------------------------------------------------
// This table is used to move a 3x3x3 neighborhood of cursors through the tree.
void vtkHyperTreeGrid::GenerateSuperCursorTraversalTable()
{
  int bf = static_cast<int>( this->BranchFactor );

  int xChildDim = bf;
  int yChildDim = this->Dimension > 1 ? bf : 1;
  int zChildDim = this->Dimension > 2 ? bf : 1;
  int xCursorDim = 3;
  int yCursorDim = this->Dimension > 1 ? 3 : 1;
  int zCursorDim = this->Dimension > 2 ? 3 : 1;

  int childId = 0;
  for ( int zChild = 0; zChild < zChildDim; ++ zChild )
    {
    for ( int yChild = 0; yChild < yChildDim; ++ yChild )
      {
      for ( int xChild = 0; xChild < xChildDim; ++ xChild, ++ childId )
        {
        int cursorId = 0;
        for ( int zCursor = 0; zCursor < zCursorDim; ++ zCursor )
          {
          for ( int yCursor = 0; yCursor < yCursorDim; ++ yCursor )
            {
            for ( int xCursor = 0; xCursor < xCursorDim; ++ xCursor, ++ cursorId )
              {
              // Compute the x, y, z index into the
              // 6x6x6 (9x9x9) neighborhood of children.
              int xNeighbor = xCursor + xChild + xChildDim - 1;
              int yNeighbor = yCursor + yChild + yChildDim - 1;
              int zNeighbor = zCursor + zChild + zChildDim - 1;

              // Separate neighbor index into Cursor/Child index.
              div_t dx = div( xNeighbor, bf );
              div_t dy = div( yNeighbor, bf );
              div_t dz = div( zNeighbor, bf );
              int tableId = childId * 27 + cursorId;
              this->SuperCursorTraversalTable[tableId].Parent
                = dx.quot + 3 * ( dy.quot + 3 * dz.quot );
              this->SuperCursorTraversalTable[tableId].Child
                = dx.rem + bf * ( dy.rem + bf * dz.rem );
              } // xCursor
            } // yCursor
          } // zCursor
        } // xChild
      } // yChild
    } // zChild
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::DeleteInternalArrays()
{
  if ( this->Points )
    {
    this->Points->UnRegister( this );
    this->Points = 0;
    }

  if ( this->Connectivity )
    {
    this->Connectivity->UnRegister( this );
    this->Connectivity = 0;
    }

  if ( this->Links )
    {
    this->Links->UnRegister( this );
    this->Links = 0;
    }
}

//=============================================================================
// Hyper tree grid iterator
// Implemented here because it needs access to the internal classes.

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::vtkHyperTreeIterator::Initialize( vtkHyperTreeGrid* tree )
{
  this->Tree = tree;
  this->Iterator = tree->HyperTrees.begin();
}

//-----------------------------------------------------------------------------
vtkHyperTree* vtkHyperTreeGrid::vtkHyperTreeIterator::GetNextTree( vtkIdType &index )
{
  if ( this->Iterator == this->Tree->HyperTrees.end() )
    {
    return 0;
    }
  vtkHyperTree* t = this->Iterator->second;
  index = this->Iterator->first;
  ++ this->Iterator;
  return t;
}

//-----------------------------------------------------------------------------
vtkHyperTree* vtkHyperTreeGrid::vtkHyperTreeIterator::GetNextTree()
{
  vtkIdType index;
  return GetNextTree( index );
}

//=============================================================================
// Hyper tree grid cursor
// Implemented here because it needs access to the internal classes.

//-----------------------------------------------------------------------------
// Constructor.
vtkHyperTreeGrid::vtkHyperTreeSimpleCursor::vtkHyperTreeSimpleCursor()
{
  Clear();
}

//-----------------------------------------------------------------------------
// Set the state back to the initial contructed state
void vtkHyperTreeGrid::vtkHyperTreeSimpleCursor::Clear()
{
  this->Tree = 0;
  this->Index = 0;
  this->Leaf = false;
  this->Level = 0;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::vtkHyperTreeSimpleCursor::Initialize( vtkHyperTreeGrid* grid,
                                                             vtkIdType index,
                                                             int pos[3] )
{
  // Convert local index into global one
  unsigned int n[3];
  grid->GetGridSize( n );
  vtkIdType globalIndex = grid->GetTransposedRootIndexing() ?
    index + pos[2] +
    pos[1] * static_cast<int>( n[2] ) +
    pos[0] * static_cast<int>( n[2] ) * static_cast<int>( n[1] ) :
    index + pos[0] +
    pos[1] * static_cast<int>( n[0] ) +
    pos[2] * static_cast<int>( n[0] ) * static_cast<int>( n[1] );

  // Assign hypertree to this cursor
  this->Tree = GetHTGHyperTreeAtIndexMacro( grid, globalIndex );
  this->ToRoot();
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGrid::vtkHyperTreeSimpleCursor::IsLeaf()
{
  // Empty cursors appear like a leaf so that recursion stop
  return this->Tree ? this->Leaf : true;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::vtkHyperTreeSimpleCursor::ToRoot()
{
  if ( ! this->Tree )
    {
    return;
    }

  // Return to root level
  this->Level = 0;
  this->Index = 0;

  this->Leaf = ( this->Tree->GetNumberOfLeaves() == 1 );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::vtkHyperTreeSimpleCursor::ToChild( int child )
{
  if ( ! this->Tree || this->Leaf )
    {
    // Leaves do not have children.
    return;
    }

  this->Tree->FindChildParameters( child, this->Index, this->Leaf );
  this->Level++;

  assert( "Bad index" && this->Index >= 0 );
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::vtkHyperTreeSimpleCursor::GetGlobalNodeIndex()
{
  return this->Tree ?
    this->Tree->GetGlobalIndexFromLocal( this->Index ) : 0;
}
