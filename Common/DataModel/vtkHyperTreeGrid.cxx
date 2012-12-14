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
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTimerLog.h"
#include "vtkVoxel.h"

#include <assert.h>

vtkInformationKeyMacro(vtkHyperTreeGrid, LEVELS, Integer);
vtkInformationKeyMacro(vtkHyperTreeGrid, DIMENSION, Integer);
vtkInformationKeyRestrictedMacro(vtkHyperTreeGrid, SIZES, DoubleVector, 3 );

vtkStandardNewMacro(vtkHyperTreeGrid);

vtkCxxSetObjectMacro(vtkHyperTreeGrid,MaterialMask,vtkBitArray);
vtkCxxSetObjectMacro(vtkHyperTreeGrid,XCoordinates,vtkDataArray);
vtkCxxSetObjectMacro(vtkHyperTreeGrid,YCoordinates,vtkDataArray);
vtkCxxSetObjectMacro(vtkHyperTreeGrid,ZCoordinates,vtkDataArray);

//-----------------------------------------------------------------------------
vtkHyperTreeGrid::vtkHyperTreeGrid()
{
  // Grid of hyper trees
  this->HyperTrees = vtkCollection::New();
  this->HyperTreesLeafIdOffsets = 0;

  // Primal grid corners
  this->CornerPoints = 0;
  this->LeafCornerIds = 0;

  // Dual grid corners (primal grid leaf centers)
  this->LeafCenters = 0;
  this->LeafCenterIds = 0;

  // Internal links
  this->Links = 0;

  // Grid topology
  this->GridSize[0] = 0;
  this->GridSize[1] = 0;
  this->GridSize[2] = 0;
  this->NumberOfRoots = 0;

  // Grid parameters
  this->BranchFactor = 2;
  this->Dimension =  1;
  this->NumberOfChildren = 2;
  this->UseDualGrid = 1;

  // Masked primal leaves
  this->MaterialMask = vtkBitArray::New();

  // Grid geometry
  this->XCoordinates = vtkDoubleArray::New();
  this->XCoordinates->SetNumberOfTuples( 1 );
  this->XCoordinates->SetComponent( 0, 0, 0. );
  this->YCoordinates = vtkDoubleArray::New();
  this->YCoordinates->SetNumberOfTuples( 1 );
  this->YCoordinates->SetComponent( 0, 0, 0. );
  this->ZCoordinates = vtkDoubleArray::New();
  this->ZCoordinates->SetNumberOfTuples( 1 );
  this->ZCoordinates->SetComponent( 0, 0, 0. );

  // For data set API
  this->Voxel = vtkVoxel::New();
  this->Pixel = vtkPixel::New();
  this->Line = vtkLine::New();
}

//-----------------------------------------------------------------------------
vtkHyperTreeGrid::~vtkHyperTreeGrid()
{
  if ( this->HyperTrees )
    {
    vtkCollectionSimpleIterator it;
    for ( this->HyperTrees->InitTraversal( it );
          vtkObject* obj = this->HyperTrees->GetNextItemAsObject( it ) ; )
      {
      if ( obj )
        {
        obj->UnRegister( this );
        }
      }
    this->HyperTrees->UnRegister( this );
    }

  if ( this->MaterialMask )
    {
    this->MaterialMask->UnRegister( this );
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
  os << indent << "NumberOfRoots: " << this->NumberOfRoots << endl;

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
  os << indent << "UseDualGrid: " << this->UseDualGrid << endl;
}

//-----------------------------------------------------------------------------
// Description:
// Return what type of dataset this is.
int vtkHyperTreeGrid::GetDataObjectType()
{
  return VTK_HYPER_TREE_GRID;
}

//-----------------------------------------------------------------------------
// Description:
// Copy the geometric and topological structure of a hyper tree grid
// object.
void vtkHyperTreeGrid::CopyStructure( vtkDataSet* ds )
{
  assert( "pre: ds_exists" && ds!=0 );
  assert( "pre: same_type" && vtkHyperTreeGrid::SafeDownCast(ds)!=0 );
  assert( "pre: cell_tree" && vtkHyperTreeGrid::SafeDownCast(ds)->HyperTrees!=0 );

  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast( ds );

  // Copy grid parameters
  this->Dimension = htg->Dimension;
  this->BranchFactor = htg->BranchFactor;
  this->NumberOfChildren = htg->NumberOfChildren;
  memcpy( this->GridSize, htg->GetGridSize(), 3 * sizeof( int ) );
  this->NumberOfRoots = this->GridSize[0] * this->GridSize[1] * this->GridSize[2];
  this->UseDualGrid = htg->UseDualGrid;

  // Un-register existing tree
  if ( this->HyperTrees )
    {
    vtkCollectionSimpleIterator it;
    for ( this->HyperTrees->InitTraversal( it );
          vtkObject* obj = this->HyperTrees->GetNextItemAsObject( it ) ; )
      {
      if ( obj )
        {
        obj->UnRegister( this );
        }
      }
    this->HyperTrees->UnRegister( this );
    }
  
  // Shallow copy and register new tree
  this->HyperTrees = htg->HyperTrees;
  if ( this->HyperTrees )
    {
    this->HyperTrees->Register( this );

    vtkCollectionSimpleIterator it1;
    this->HyperTrees->InitTraversal( it1 );
    vtkCollectionSimpleIterator it2;
    htg->HyperTrees->InitTraversal( it2 );
    while ( vtkObject* obj = this->HyperTrees->GetNextItemAsObject( it1 ) )
      {
      obj = htg->HyperTrees->GetNextItemAsObject( it2 );
      if ( obj )
        {
        obj->Register( this );
        }
      }
    }

  // Shallow copy cell tree leaf ID offsets
  if ( this->HyperTreesLeafIdOffsets )
    {
    delete [] this->HyperTreesLeafIdOffsets;
    this->HyperTreesLeafIdOffsets = 0;
    }
  this->HyperTreesLeafIdOffsets = htg->HyperTreesLeafIdOffsets;

  // Shallow copy masked leaf IDs
  this->SetMaterialMask( htg->MaterialMask );

  // Shallow copy coordinates
  this->SetXCoordinates( htg->XCoordinates );
  this->SetYCoordinates( htg->YCoordinates );
  this->SetZCoordinates( htg->ZCoordinates );
}

//-----------------------------------------------------------------------------
// Set the number of root cells of the tree.
void vtkHyperTreeGrid::SetGridSize( unsigned int n[3] )
{
  if( this->GridSize[0] == n[0] && this->GridSize[1] == n[1] && this->GridSize[2] == n[2] )
    {
    return;
    }
  this->GridSize[0] = n[0];
  this->GridSize[1] = n[1];
  this->GridSize[2] = n[2];
  this->NumberOfRoots = n[0] * n[1] * n[2];

  this->Modified();
  this->UpdateTree();
}

//-----------------------------------------------------------------------------
// Set the dimension of the tree with `dim'. See GetDimension() for details.
// \pre valid_dim: dim>=1 && dim<=3
// \post dimension_is_set: GetDimension()==dim
void vtkHyperTreeGrid::SetDimension( unsigned int dim )
{
  assert( "pre: valid_dim" && dim >= 1 && dim <= 3 );
  if( this->Dimension == dim )
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
  this->UpdateTree();
}

//-----------------------------------------------------------------------------
// \pre valid_dim: factor == 2 or factor == 3;
// \post dimension_is_set: GetBranchFactor()==dim
void vtkHyperTreeGrid::SetBranchFactor( unsigned int factor )
{
  assert( "pre: valid_factor" && factor>=2 && factor<=3 );
  if( this->BranchFactor == factor )
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
  this->UpdateTree();
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::SetUseDualGrid( int dual )
{
  if ( dual )
    {
    dual = 1;
    }
  if ( this->UseDualGrid == dual )
    {
    return;
    }
  if ( ( this->UseDualGrid && ! dual ) || ( ! this->UseDualGrid && dual ) )
    {
    // Swap point and cell data.
    vtkDataSetAttributes* attr = vtkDataSetAttributes::New();
    attr->ShallowCopy( this->CellData );
    this->CellData->ShallowCopy( this->PointData );
    this->PointData->ShallowCopy( attr );
    attr->UnRegister( this );
    }
  this->DeleteInternalArrays();
  this->UseDualGrid = dual;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::UpdateTree()
{
  // Clean up existing trees
  if ( this->HyperTrees )
    {
    vtkCollectionSimpleIterator it;
    for ( this->HyperTrees->InitTraversal( it );
          vtkObject* obj = this->HyperTrees->GetNextItemAsObject( it ) ; )
      {
      if ( obj )
        {
        obj->UnRegister( this );
        }
      }
    this->HyperTrees->RemoveAllItems();
    }

  // Generate concrete instance of hyper tree and append it to list of roots
  for ( unsigned int i = 0; i < this->NumberOfRoots; ++ i )
    {
    vtkHyperTree* tree = vtkHyperTree::CreateInstance( this->BranchFactor, this->Dimension );
    this->HyperTrees->AddItem( tree );
    }

  this->Modified();
  this->DeleteInternalArrays();
}

//----------------------------------------------------------------------------
void vtkHyperTreeGrid::ComputeBounds()
{
  double tmp;

  if ( ! this->XCoordinates || ! this->YCoordinates || ! this->ZCoordinates )
    {
    vtkMath::UninitializeBounds(this->Bounds);
    return;
    }

  if ( ! this->XCoordinates->GetNumberOfTuples() ||
       ! this->YCoordinates->GetNumberOfTuples() ||
       ! this->ZCoordinates->GetNumberOfTuples() )
    {
    vtkMath::UninitializeBounds( this->Bounds );
    return;
    }

  this->Bounds[0] = this->XCoordinates->GetComponent( 0, 0 );
  this->Bounds[2] = this->YCoordinates->GetComponent( 0, 0 );
  this->Bounds[4] = this->ZCoordinates->GetComponent( 0, 0 );

  this->Bounds[1] = this->XCoordinates->GetComponent( this->XCoordinates->GetNumberOfTuples() - 1, 0 );
  this->Bounds[3] = this->YCoordinates->GetComponent( this->YCoordinates->GetNumberOfTuples() - 1, 0 );
  this->Bounds[5] = this->ZCoordinates->GetComponent( this->ZCoordinates->GetNumberOfTuples() - 1, 0 );

  // Ensure that the bounds are increasing
  for (int i = 0; i < 5; i += 2)
    {
    if (this->Bounds[i + 1] < this->Bounds[i])
      {
      tmp = this->Bounds[i + 1];
      this->Bounds[i + 1] = this->Bounds[i];
      this->Bounds[i] = tmp;
      }
    }
}

//-----------------------------------------------------------------------------
// Description:
// Return the number of levels.
// \post result_greater_or_equal_to_one: result>=1
int vtkHyperTreeGrid::GetNumberOfLevels( unsigned int i )
{
  vtkObject* obj = this->HyperTrees->GetItemAsObject( i );
  if ( obj )
    {
    vtkHyperTree* tree = static_cast<vtkHyperTree*>( obj );
    return tree->GetNumberOfLevels();
    }

  return 0;
}


//-----------------------------------------------------------------------------
// Description:
// Create a new cursor: an object that can traverse vtkHyperTree cells
vtkHyperTreeCursor* vtkHyperTreeGrid::NewCursor( int idx )
{
  vtkObject* obj = this->HyperTrees->GetItemAsObject( idx );
  if ( obj )
    {
    vtkHyperTree* tree = static_cast<vtkHyperTree*>( obj );
    return tree->NewCursor();
    }

  return NULL;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::SubdivideLeaf( vtkHyperTreeCursor* leaf, vtkIdType i )
{
  assert( "pre: leaf_exists" && leaf );
  assert( "pre: is_a_leaf" && leaf->IsLeaf() );
  vtkObject* obj = this->HyperTrees->GetItemAsObject( i );
  if ( obj )
    {
    vtkHyperTree* tree = static_cast<vtkHyperTree*>( obj );
    tree->SubdivideLeaf( leaf );
    this->DeleteInternalArrays();
    }

  return;
}

//-----------------------------------------------------------------------------
// Description:
// Restore data object to initial state,
// THIS METHOD IS NOT THREAD SAFE.
void vtkHyperTreeGrid::Initialize()
{
  if ( this->HyperTrees )
    {
    vtkCollectionSimpleIterator it;
    for ( this->HyperTrees->InitTraversal( it );
          vtkObject* obj = this->HyperTrees->GetNextItemAsObject( it ) ; )
      if ( obj )
        {
        vtkHyperTree* tree = static_cast<vtkHyperTree*>( obj );
        tree->Initialize();
        }
    }

  this->DeleteInternalArrays();
}

//-----------------------------------------------------------------------------
// Description:
// Convenience method returns largest cell size in dataset. This is generally
// used to allocate memory for supporting data structures.
// This is the number of points of a cell.
// THIS METHOD IS THREAD SAFE
int vtkHyperTreeGrid::GetMaxCellSize()
{
  int result;
  switch( this->Dimension )
    {
    case 3:
      result = 8; // hexahedron=8 points
      break;
    case 2:
      result = 4; // quad=4 points
      break;
    case 1:
      result = 2; // line=2 points
      break;
    default:
      result = 0; // useless, just to avoid a warning
      assert( "check: impossible_case" && 0 );
      break;
    }
  assert( "post: positive_result" && result>0 );
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Shallow and Deep copy.
void vtkHyperTreeGrid::ShallowCopy(vtkDataObject* src)
{
  assert( "src_same_type" && vtkHyperTreeGrid::SafeDownCast( src ) );
  this->CopyStructure(vtkHyperTreeGrid::SafeDownCast( src ) );

  // Do superclass
  this->Superclass::ShallowCopy( src );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::DeepCopy( vtkDataObject* src )
{
  assert( "src_same_type" && vtkHyperTreeGrid::SafeDownCast( src ) );
  this->CopyStructure( vtkHyperTreeGrid::SafeDownCast( src ) );

  // Do superclass
  this->Superclass::DeepCopy( src );
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGrid::GetNumberOfLeaves()
{
  int nLeaves = 0;
  vtkCollectionSimpleIterator it;
    for ( this->HyperTrees->InitTraversal( it );
          vtkObject* obj = this->HyperTrees->GetNextItemAsObject( it ) ; )
    {
    if ( obj )
      {
      vtkHyperTree* tree = static_cast<vtkHyperTree*>( obj );
      nLeaves += tree->GetNumberOfLeaves();
      }
    }

  return nLeaves;
}

//=============================================================================
// DataSet API that returns dual grid.

//-----------------------------------------------------------------------------
// Description:
// Return the number of leaves.
// \post positive_result: result>=0
vtkIdType vtkHyperTreeGrid::GetNumberOfCells()
{
  if ( this->UseDualGrid )
    {
    this->UpdateDualArrays();
    vtkIdTypeArray* cornerLeafIds = this->GetLeafCenterIds();
    return cornerLeafIds->GetNumberOfTuples();
    }
  else
    {
    return this->GetNumberOfLeaves();
    }
}

//-----------------------------------------------------------------------------
// Description:
// Return the number of points.
// \post positive_result: result>=0
vtkIdType vtkHyperTreeGrid::GetNumberOfPoints()
{
  if ( this->UseDualGrid )
    {
    return this->GetNumberOfLeaves();
    }
  else
    {
    this->UpdateGridArrays();
    vtkPoints* cornerPoints = this->GetCornerPoints();
    return cornerPoints->GetNumberOfPoints();
    }
}


//-----------------------------------------------------------------------------
// Description:
// Get point coordinates with ptId such that: 0 <= ptId < NumberOfPoints.
// THIS METHOD IS NOT THREAD SAFE.
double* vtkHyperTreeGrid::GetPoint(vtkIdType ptId)
{
  if ( this->UseDualGrid )
    {
    this->UpdateDualArrays();
    vtkPoints* leafCenters = this->GetLeafCenters();
    assert( "Index out of bounds." &&
            ptId >= 0 && ptId < leafCenters->GetNumberOfPoints() );
    return leafCenters->GetPoint(ptId);
    }
  else
    {
    this->UpdateGridArrays();
    vtkPoints* cornerPoints = this->GetCornerPoints();
    assert( "Index out of bounds." &&
            ptId >= 0 && ptId < cornerPoints->GetNumberOfPoints() );
    return cornerPoints->GetPoint(ptId);
    }
}

//-----------------------------------------------------------------------------
// Description:
// Copy point coordinates into user provided array x[3] for specified
// point id.
// THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
// THE DATASET IS NOT MODIFIED
void vtkHyperTreeGrid::GetPoint(vtkIdType id, double x[3])
{
  if ( this->UseDualGrid )
    {
    this->UpdateDualArrays();
    vtkPoints* leafCenters = this->GetLeafCenters();
    assert( "Index out of bounds." &&
            id >= 0 && id < leafCenters->GetNumberOfPoints() );
    leafCenters->GetPoint(id, x );
    }
  else
    {
    this->UpdateGridArrays();
    vtkPoints* cornerPoints = this->GetCornerPoints();
    assert( "Index out of bounds." &&
            id >= 0 && id < cornerPoints->GetNumberOfPoints() );
    cornerPoints->GetPoint(id, x );
    }
}

//-----------------------------------------------------------------------------
// Description:
// Get cell with cellId such that: 0 <= cellId < NumberOfCells.
// THIS METHOD IS NOT THREAD SAFE.
vtkCell* vtkHyperTreeGrid::GetCell(vtkIdType cellId)
{
  vtkCell* cell = NULL;
  int numPts = 1<<this->GetDimension();
  int ptIdx;
  double x[3];

  switch ( this->GetDimension() )
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

  if ( this->UseDualGrid )
    {
    this->UpdateDualArrays();
    vtkIdTypeArray* cornerLeafIds = this->GetLeafCenterIds();
    assert( "Index out of bounds." &&
            cellId >= 0 && cellId < cornerLeafIds->GetNumberOfTuples() );
    vtkPoints* leafCenters = this->GetLeafCenters();
    vtkIdType* ptr = cornerLeafIds->GetPointer( 0 ) + cellId*numPts;
    for (ptIdx = 0; ptIdx < numPts; ++ptIdx )
      {
      cell->PointIds->SetId(ptIdx, *ptr);
      leafCenters->GetPoint(*ptr, x );
      cell->Points->SetPoint(ptIdx,x );
      ++ptr;
      }
    }
  else
    {
    this->UpdateGridArrays();
    vtkIdTypeArray* leafCornerIds = this->GetLeafCornerIds();
    assert( "Index out of bounds." &&
            cellId >= 0 && cellId < leafCornerIds->GetNumberOfTuples() );
    vtkPoints* cornerPoints = this->GetCornerPoints();
    vtkIdType* ptr = leafCornerIds->GetPointer( 0 ) + cellId*numPts;
    for (ptIdx = 0; ptIdx < numPts; ++ptIdx )
      {
      cell->PointIds->SetId( ptIdx, *ptr );
      cornerPoints->GetPoint( *ptr, x );
      cell->Points->SetPoint( ptIdx, x );
      ++ptr;
      }
    }

  return cell;
}

//-----------------------------------------------------------------------------
// Description:
// Get cell with cellId such that: 0 <= cellId < NumberOfCells.
// This is a thread-safe alternative to the previous GetCell()
// method.
// THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
// THE DATASET IS NOT MODIFIED
void vtkHyperTreeGrid::GetCell(vtkIdType cellId, vtkGenericCell* cell)
{
  int numPts = 1<<this->GetDimension();
  int ptIdx;
  double x[3];

  switch ( this->GetDimension() )
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

  if ( this->UseDualGrid )
    {
    this->UpdateDualArrays();
    vtkIdTypeArray* cornerLeafIds = this->GetLeafCenterIds();
    assert( "Index out of bounds." &&
            cellId >= 0 && cellId < cornerLeafIds->GetNumberOfTuples() );
    vtkPoints* leafCenters = this->GetLeafCenters();
    vtkIdType* ptr = cornerLeafIds->GetPointer( 0 ) + cellId*numPts;
    for (ptIdx = 0; ptIdx < numPts; ++ptIdx )
      {
      cell->PointIds->SetId(ptIdx, *ptr);
      leafCenters->GetPoint(*ptr, x );
      cell->Points->SetPoint(ptIdx,x );
      ++ptr;
      }
    }
  else
    {
    this->UpdateGridArrays();
    vtkIdTypeArray* leafCornerIds = this->GetLeafCornerIds();
    assert( "Index out of bounds." &&
            cellId >= 0 && cellId < leafCornerIds->GetNumberOfTuples() );
    vtkPoints* cornerPoints = this->GetCornerPoints();
    vtkIdType* ptr = leafCornerIds->GetPointer( 0 ) + cellId*numPts;
    for (ptIdx = 0; ptIdx < numPts; ++ptIdx )
      {
      cell->PointIds->SetId(ptIdx, *ptr);
      cornerPoints->GetPoint(*ptr, x );
      cell->Points->SetPoint(ptIdx,x );
      ++ptr;
      }
    }
}


//-----------------------------------------------------------------------------
// Description:
// Get type of cell with cellId such that: 0 <= cellId < NumberOfCells.
// THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
// THE DATASET IS NOT MODIFIED
int vtkHyperTreeGrid::GetCellType(vtkIdType vtkNotUsed(cellId) )
{
  int result;
  switch( this->Dimension )
    {
    case 3:
      result = VTK_VOXEL; // hexahedron=8 points
      break;
    case 2:
      result = VTK_PIXEL; // quad=4 points
      break;
    case 1:
      result = VTK_LINE; // line=2 points
      break;
    default:
      result = 0; // impossible case
      break;
    }
  assert( "post: positive_result" && result>0 );
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Topological inquiry to get points defining cell.
// THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
// THE DATASET IS NOT MODIFIED
void vtkHyperTreeGrid::GetCellPoints( vtkIdType cellId, vtkIdList* ptIds)
{
  int numPts = 1 << this->GetDimension();
  ptIds->Initialize();

  if ( this->UseDualGrid )
    {
    this->UpdateDualArrays();
    vtkIdTypeArray* cornerLeafIds = this->GetLeafCenterIds();
    assert( "Index out of bounds." &&
            cellId >= 0 && cellId < cornerLeafIds->GetNumberOfTuples() );
    vtkIdType* ptr = cornerLeafIds->GetPointer( 0 ) + cellId*numPts;
    for ( int i = 0; i < numPts; ++ i )
      {
      ptIds->InsertId( i, *ptr++ );
      }
    }
  else
    {
    this->UpdateGridArrays();
    vtkIdTypeArray* leafCornerIds = this->GetLeafCornerIds();
    assert( "Index out of bounds." &&
            cellId >= 0 && cellId < leafCornerIds->GetNumberOfTuples() );
    vtkIdType* ptr = leafCornerIds->GetPointer( 0 ) + cellId * numPts;
    for ( int i = 0; i < numPts; ++ i )
      {
      ptIds->InsertId( i, *ptr++ );
      }
    }
}

//----------------------------------------------------------------------------
// Return a pointer to a list of point ids defining cell. (More efficient than alternative
// method.)
void vtkHyperTreeGrid::GetCellPoints( vtkIdType cellId,
                                      vtkIdType& npts,
                                      vtkIdType* &pts )
{
  if ( this->UseDualGrid )
    {
    this->UpdateDualArrays();
    vtkIdTypeArray* cornerLeafIds = this->GetLeafCenterIds();
    assert( "Index out of bounds." &&
            cellId >= 0 && cellId < cornerLeafIds->GetNumberOfTuples() );
    // Casting of 1 is necessary to remove 64bit Compiler warning C4334 on
    // Visual Studio 2005.
    npts = static_cast<vtkIdType>( 1 ) << this->GetDimension();
    pts = cornerLeafIds->GetPointer( 0 ) + cellId * npts;
    }
  else
    {
    this->UpdateGridArrays();
    vtkIdTypeArray* leafCornerIds = this->GetLeafCornerIds();
    assert( "Index out of bounds." &&
            cellId >= 0 && cellId < leafCornerIds->GetNumberOfTuples() );
    // Casting of 1 is necessary to remove 64bit Compiler warning C4334 on
    // Visual Studio 2005.
    npts = static_cast<vtkIdType>( 1 ) << this->GetDimension();
    pts = leafCornerIds->GetPointer( 0 ) + cellId * npts;
    }
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
// This is unecessary because we have the info already.
// Is it really a part of the vtkDataSet API?
// It would be better to build the links of both dual and grid.
void vtkHyperTreeGrid::BuildLinks()
{
  assert( "Not tested for 27 trees" && 0 );
  this->Links = vtkCellLinks::New();
  this->Links->Allocate( this->GetNumberOfPoints() );
  this->Links->Register( this );
  this->Links->BuildLinks( this );
  this->Links->UnRegister( this );
}


//-----------------------------------------------------------------------------
// This is exactly the same as GetCellNeighbors in unstructured grid.
void vtkHyperTreeGrid::GetCellNeighbors( vtkIdType cellId,
                                         vtkIdList* ptIds,
                                         vtkIdList* cellIds )
{
  int i, j, k;
  int numPts, minNumCells, numCells;
  vtkIdType *pts, ptId, *cellPts, *cells;
  vtkIdType *minCells = NULL;
  int match;
  vtkIdType minPtId = 0, npts;

  if ( ! this->Links )
    {
    this->BuildLinks();
    }

  cellIds->Reset();

  //Find the point used by the fewest number of cells
  numPts = ptIds->GetNumberOfIds();
  pts = ptIds->GetPointer( 0 );
  for (minNumCells=VTK_LARGE_INTEGER,i=0; i<numPts; i++)
    {
    ptId = pts[i];
    numCells = this->Links->GetNcells(ptId);
    cells = this->Links->GetCells(ptId);
    if ( numCells < minNumCells )
      {
      minNumCells = numCells;
      minCells = cells;
      minPtId = ptId;
      }
    }

  if (minNumCells == VTK_LARGE_INTEGER && numPts == 0 ) {
  vtkErrorMacro( "input point ids empty." );
  minNumCells = 0;
  }

  // For all cells that contNow for each cell, see if it contains all the points
  // in the ptIds list.
  for ( i = 0; i < minNumCells; i ++ )
    {
    // Do not include current cell
    if ( minCells[i] != cellId )
      {
      this->GetCellPoints(minCells[i],npts,cellPts);
      // Iterate over all points in input cell
      for ( match = 1, j = 0; j < numPts && match; j++ )
        {
        // Skip point with index minPtId which is contained by current cell
        if ( pts[j] != minPtId )
          {
          // Iterate over all points in current cell
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
// Note: This always returns the closest point, even if the point is outside
// tree.
// Since dual points are leaves, use the structure of the Tree instead
// of a point locator.
vtkIdType vtkHyperTreeGrid::FindPoint( double x[3] )
{
  // Find cell to which this point belongs, or at least closest one
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

  int index = ( iz * this->GridSize[1] + iy ) * this->GridSize[0] + ix;
  vtkHyperTreeSimpleCursor cursor;
  vtkIdType offsets[3];
  int pos[] = { 0, 0, 0 };
  cursor.Initialize( this, offsets, index, pos );

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

  vtkHyperTreeSimpleCursor newCursor;
  newCursor = *cursor;
  double newSize[3];
  double newOrigin[3];
  unsigned char child = 0;
  for ( int i = 0; i < 3; ++ i)
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

  return this->RecursiveFindPoint(x, &newCursor, newOrigin, newSize);
}

//----------------------------------------------------------------------------
// No need for a starting cell.  Just use the point.
// Tree is efficient enough.
vtkIdType vtkHyperTreeGrid::FindCell(double x[3], vtkCell* cell,
                                     vtkGenericCell* gencell, vtkIdType cellId,
                                     double tol2, int& subId, double pcoords[3],
                                     double* weights)
{
  vtkIdType       ptId;
  double          closestPoint[3];
  double          dist2;
  vtkIdList      * cellIds;

  ptId = this->FindPoint( x );
  if ( ptId < 0 )
    {
    return -1; //if point completely outside of data
    }

  cellIds = vtkIdList::New();
  cellIds->Allocate( 8, 100 );
  this->GetPointCells(ptId, cellIds);
  if ( cellIds->GetNumberOfIds() <= 0 )
    {
    cellIds->UnRegister( this );
    return -1;
    }

  int num = cellIds->GetNumberOfIds();
  for ( int i = 0; i < num; ++ i )
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
    dx[0] = x[0];
    dx[1] = x[1];
    dx[2] = x[2];
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
vtkIdType vtkHyperTreeGrid::FindCell(double x[3], vtkCell* cell, vtkIdType cellId,
                                     double tol2, int& subId,double pcoords[3],
                                     double* weights)
{
  return this->FindCell( x, cell, NULL, cellId, tol2, subId, pcoords, weights );
}



//-----------------------------------------------------------------------------
// Generic way to set the leaf data attributes.
vtkDataSetAttributes* vtkHyperTreeGrid::GetLeafData()
{
  if ( this->UseDualGrid )
    {
    return this->PointData;
    }
  else
    {
    return this->CellData;
    }
}

//----------------------------------------------------------------------------
unsigned long vtkHyperTreeGrid::GetActualMemorySize()
{
  unsigned long size = this->vtkDataSet::GetActualMemorySize();
  
  vtkCollectionSimpleIterator it;
  for ( this->HyperTrees->InitTraversal( it );
        vtkObject* obj = this->HyperTrees->GetNextItemAsObject( it ) ; )
    {
    if ( obj )
      {
      vtkHyperTree* tree = vtkHyperTree::SafeDownCast( obj );
      if ( tree )
        {
        size += tree->GetActualMemorySize();
        }
      }
    }

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

  if ( this->LeafCenters)
    {
    size += this->LeafCenters->GetActualMemorySize();
    }
  if ( this->LeafCenterIds)
    {
    size += this->LeafCenterIds->GetActualMemorySize();
    }
  if ( this->CornerPoints)
    {
    size += this->CornerPoints->GetActualMemorySize();
    }
  if ( this->LeafCornerIds)
    {
    size += this->LeafCornerIds->GetActualMemorySize();
    }
  if ( this->MaterialMask)
    {
    size += this->MaterialMask->GetActualMemorySize();
    }

  return size;
}

//=============================================================================
// Internal arrays used to generate dual grid.  Random access to cells
// requires the cell leaves connectively array which costs memory.


//-----------------------------------------------------------------------------
vtkPoints* vtkHyperTreeGrid::GetLeafCenters()
{
  this->UpdateDualArrays();
  return this->LeafCenters;
}

//-----------------------------------------------------------------------------
vtkIdTypeArray* vtkHyperTreeGrid::GetLeafCenterIds()
{
  this->UpdateDualArrays();
  return this->LeafCenterIds;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::InitializeSuperCursor( vtkHyperTreeGridSuperCursor* superCursor,
                                              unsigned int i,
                                              unsigned int j,
                                              unsigned int k )
{
  // TODO:  This only needs to be done once.  Use MTime instead
  this->UpdateHyperTreesLeafIdOffsets();

  // Calculate global index of hyper tree
  int index = ( k * this->GridSize[1] + j ) * this->GridSize[0] + i;

  // Location and size of the middle cursor/node
  double origin[3];
  this->XCoordinates->GetTuple( i, origin );
  this->YCoordinates->GetTuple( j, origin + 1);
  this->ZCoordinates->GetTuple( k, origin + 2 );

  double extreme[3];
  this->XCoordinates->GetTuple( i + 1, extreme );
  this->YCoordinates->GetTuple( j + 1, extreme + 1);
  this->ZCoordinates->GetTuple( k + 1, extreme + 2 );

  superCursor->Size[0] = extreme[0] - origin[0];
  superCursor->Size[1] = extreme[1] - origin[1];
  superCursor->Size[2] = extreme[2] - origin[2];
  superCursor->Origin[0] = origin[0];
  superCursor->Origin[1] = origin[1];
  superCursor->Origin[2] = origin[2];

  // 3x3x3 has nothing to do with whether subdivision is binary or ternary
  int dim = this->GetDimension();
  if ( dim == 1 )
    {
    superCursor->MiddleCursorId = 1;
    superCursor->NumberOfCursors = 3;
    }
  if ( dim == 2 )
    {
    superCursor->MiddleCursorId = 4;
    superCursor->NumberOfCursors = 9;
    }
  if ( dim == 3 )
    {
    superCursor->MiddleCursorId = 13;
    superCursor->NumberOfCursors = 27;
    }

  // Initialize all connectivity cursors by generating all possible cases
  int pos[3];
  for ( pos[2] = -1; pos[2] < 2; ++ pos[2] )
    {
    bool tk = true;
    switch ( pos[2] )
      {
      case -1:
        tk = ( k > 0 );
        break;
      case 1:
        tk = ( k + 1 < this->GridSize[2] );
        break;
      } // switch( pos[2] )

    for ( pos[1] = -1; pos[1] < 2; ++ pos[1] )
      {
      bool tj = true;
      switch ( pos[1] )
        {
        case -1:
          tj = ( j > 0 );
          break;
        case 1:
          tj = ( j + 1 < this->GridSize[1] );
          break;
        } // switch( pos[1] )

      for ( pos[0] = -1; pos[0] < 2; ++ pos[0] )
        {
        bool ti = true;
        switch ( pos[0] )
          {
          case -1:
            ti = ( i > 0 );
            break;
          case 1:
            ti = ( i + 1 < this->GridSize[0] );
            break;
          } // switch( pos[0] )

        // Initialize cursor if connectivity conditions are satisfied
        int d = pos[0] + 3 * pos[1] + 9 * pos[2];
        if ( ti && tj && tk )
          {
          superCursor->GetCursor( d )->Initialize( this,
                                                   this->HyperTreesLeafIdOffsets,
                                                   index, pos );
          } // if ( ti && tj && tk )
        else
          {
          if ( this->Dimension < 3 && ! pos[2] )
            {
            superCursor->GetCursor( d )->Clear();
            }
          } // else
        } // c
      } // b
    } // a
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::InitializeSuperCursorChild( vtkHyperTreeGridSuperCursor* parent, 
                                                   vtkHyperTreeGridSuperCursor* child,
                                                   int childIdx )
{
  // Retrieve child's parameters that are identical to parent's ones
  child->NumberOfCursors = parent->NumberOfCursors;
  child->MiddleCursorId = parent->MiddleCursorId;

  // Compute size of child
  child->Size[0] = parent->Size[0] / double( this->BranchFactor );
  child->Size[1] = parent->Size[1] / double( this->BranchFactor );
  child->Size[2] = parent->Size[2] / double( this->BranchFactor );

  // Compute origin of child
  int x, y, z;
  if ( this->BranchFactor == 2 )
    {
    x = childIdx & 1;
    y = ( childIdx & 2 )>>1;
    z = ( childIdx & 4 )>>2;
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
  vtkSuperCursorEntry* cursorPtr = this->SuperCursorTraversalTable + ( childIdx * 27 );
  for ( int cursorIdx = 0; cursorIdx < child->NumberOfCursors; ++ cursorIdx )
    {
    // Extract the parent and child of the new node from the traversal table
    // Child is encoded in the first three bits for all dimensions
    int tChild = cursorPtr[cursorIdx].Child;
    int tParent = cursorPtr[cursorIdx].Parent;
    if ( ! parent->Cursors[tParent].GetTree() )
      {
      // No node for this cursor
      child->Cursors[cursorIdx] = parent->Cursors[tParent];
      }
    else if ( parent->Cursors[tParent].IsLeaf() )
      {
      // Parent is a leaf, cannot traverse further
      child->Cursors[cursorIdx] = parent->Cursors[tParent];
      }
    else
      {
      // Move to child
      child->Cursors[cursorIdx] = parent->Cursors[tParent];
      child->Cursors[cursorIdx].ToChild( tChild );
      }
    }
}

//-----------------------------------------------------------------------------
// Traverse tree with 3x3x3 super cursor.  Center cursor generates dual point
// Smallest leaf (highest level) owns corners/dual cell.  Ties are given to
// smallest index (z,y,x order)
// post: Generate LeafCenters and LeafCenterIds.
void vtkHyperTreeGrid::UpdateDualArrays()
{
  int numLeaves = this->UpdateHyperTreesLeafIdOffsets();

  // Check if we can break out early
  if ( this->LeafCenters )
    {
    if ( this->LeafCenters->GetNumberOfPoints() == numLeaves )
      {
      return;
      }
    this->LeafCenters->UnRegister( this );
    this->LeafCenters = 0;
    this->LeafCenterIds->UnRegister( this );
    this->LeafCenterIds = 0;
    }

  vtkTimerLog* timer = vtkTimerLog::New();
  timer->StartTimer();

  // Primal cell centers are dual points
  this->LeafCenters = vtkPoints::New();
  this->LeafCenters->Allocate( numLeaves );

  this->LeafCenterIds = vtkIdTypeArray::New();
  int dim = this->GetDimension();
  int numComps = 1 << dim;
  this->LeafCenterIds->SetNumberOfComponents( numComps );
  this->LeafCenterIds->Allocate( numLeaves * numComps );

  // Create an array of cursors that occupy 1 3x3x3 neighborhood
  // Will traverse the tree as one
  // NB: Lower dimensions will not use them all
  this->GenerateSuperCursorTraversalTable();

  // Iterate over all hyper trees
  for ( unsigned int k = 0; k < this->GridSize[2]; ++ k )
    {
    for ( unsigned int j = 0; j < this->GridSize[1]; ++ j )
      {
      for ( unsigned int i = 0; i < this->GridSize[0]; ++ i )
        {
        // Initialize super cursors
        vtkHyperTreeGridSuperCursor superCursor;
        this->InitializeSuperCursor( &superCursor, i, j, k );

        // Traverse and populate dual recursively
        this->TraverseDualRecursively( &superCursor, 0 );
        } // i
      } // j
    } // k

  timer->StopTimer();
  cerr << "Internal dual update : " << timer->GetElapsedTime() << endl;
  timer->UnRegister( this );
}

//-----------------------------------------------------------------------------
// Iterate over leaves.  Generate dual point.  Highest level (smallest leaf)
// owns the corner and generates that dual cell.
// Note: The recursion here is the same as TraverseGridRecursively.  The only
// difference is which arrays are generated.  We should merge the two.
void vtkHyperTreeGrid::TraverseDualRecursively( vtkHyperTreeGridSuperCursor* superCursor,
                                                int level )
{
  // Level of the middle cursor.
  int midLevel = superCursor->GetCursor( 0 )->GetLevel();
  if ( superCursor->GetCursor( 0 )->IsLeaf() )
    { 
    // Center is a leaf, create a dual point
    double pt[3];

    // Start at minimum boundary of cell
    pt[0] = superCursor->Origin[0];
    pt[1] = superCursor->Origin[1];
    pt[2] = superCursor->Origin[2];

    // Adjust point so the boundary of the dataset does not shrink
    if ( superCursor->GetCursor( -1 )->GetTree() &&
         superCursor->GetCursor( 1 )->GetTree() )
      {
      // Middle of cell
      pt[0] += superCursor->Size[0] * 0.5;
      }
    else if ( superCursor->GetCursor(1)->GetTree() == 0 )
      {
      // Move to maximum boundary of cell
      pt[0] += superCursor->Size[0];
      }
    if ( this->Dimension > 1 && 
         superCursor->GetCursor( -3 )->GetTree() &&
         superCursor->GetCursor( 3 )->GetTree() )
      {
      // Middle of cell
      pt[1] += superCursor->Size[1] * 0.5;
      }
    else if ( this->Dimension > 1 &&
              superCursor->GetCursor( 3 )->GetTree() == 0 )
      {
      // Move to maximum boundary of cell
      pt[1] += superCursor->Size[1];
      }
    if ( this->Dimension > 2 && 
         superCursor->GetCursor( -9 )->GetTree() &&
         superCursor->GetCursor( 9 )->GetTree() )
      {
      // Middle of cell
      pt[2] += superCursor->Size[2] * 0.5;
      }
    else if ( this->Dimension > 2 && 
              superCursor->GetCursor( 9 )->GetTree() == 0 )
      {
      // Move to maximum boundary of cell
      pt[2] += superCursor->Size[2];
      }

    // Insert point with given offset into leaf centers array
    int index = superCursor->GetCursor( 0 )->GetGlobalLeafIndex();
    this->LeafCenters->InsertPoint( index, pt );

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
            cursorIdx += ( cornerIdx& 1) + ( leafIdx& 1);
          }

        // Collect the leaf indexes for the dual cell
        leaves[leafIdx] = superCursor->Cursors[cursorIdx].GetGlobalLeafIndex();

        // Compute if the mid leaf owns the corner.
        if ( cursorIdx != superCursor->MiddleCursorId )
          {
          vtkHyperTreeSimpleCursor* cursor = superCursor->Cursors + cursorIdx;
          if ( ! cursor->GetTree() || ! cursor->IsLeaf() )
            {
            // If neighbor leaf is out of bounds or has not been
            // refined to a leaf, this leaf does not own the corner.
            owner = false;
            }
          else if ( cursor->GetLevel() == midLevel && superCursor->MiddleCursorId < cursorIdx )
            {
            // A level tie is broken in favor of the largest index
            // All points are set before defining the cell
            owner = false;
            }
          }
        } // leafIdx

      if ( owner )
        {
        this->LeafCenterIds->InsertNextTupleValue( leaves );
        }
      } // cornerIdx
    // Middle cursor was a leaf, terminate recursion
    return;
    } // if ( superCursor->GetCursor( 0 )->IsLeaf() )
  
  // Center is not a leaf, continue recursion
  vtkHyperTreeGridSuperCursor newSuperCursor;
  for ( unsigned int child = 0; child < this->NumberOfChildren; ++ child )
    {

    this->InitializeSuperCursorChild( superCursor, &newSuperCursor, child );
    this->TraverseDualRecursively( &newSuperCursor, level + 1 );
    }
}

//----------------------------------------------------------------------------
// Returns id if a new corner was created, -1 otherwise.
vtkIdType vtkHyperTreeGrid::EvaluateGridCorner( int level,
                                                vtkHyperTreeGridSuperCursor* superCursor,
                                                unsigned char* visited,
                                                int* cornerCursorIds )
{
  // This is correct for 27trees too because it is the number of cells around a point.
  int numLeaves = 1 << this->GetDimension();
  int leaf;
  vtkIdType cornerId;

  for ( leaf = 0; leaf < numLeaves; ++ leaf )
    {
    // All corners must be leaves
    // Note: this test also makes sure all are initialized.
    if ( superCursor->Cursors[cornerCursorIds[leaf]].GetTree() &&
         !superCursor->Cursors[cornerCursorIds[leaf]].IsLeaf() )
      {
      return -1;
      }
    // If any cursor on the same level has already generated this point ...
    if ( superCursor->Cursors[cornerCursorIds[leaf]].GetLevel() == level &&
         visited[superCursor->Cursors[cornerCursorIds[leaf]].GetLeafIndex()])
      {
      return -1;
      }
    }

  // Point is actually inserted in the Traverse method that calls this method.
  cornerId = this->CornerPoints->GetNumberOfPoints();

  // Loop through the leaves to determine which use this point.
  for ( leaf = 0; leaf < numLeaves; ++ leaf )
    {
    if ( superCursor->Cursors[cornerCursorIds[leaf]].GetTree() )
      {
      // We know it is a leaf from the previous check.
      // use bitwise exculsive or to find cursors of leaf.
      int leafId = superCursor->Cursors[cornerCursorIds[leaf]].GetLeafIndex();
      int sideLeaf = leaf^1;
      if ( superCursor->Cursors[cornerCursorIds[sideLeaf]].GetTree() &&
           leafId == superCursor->Cursors[cornerCursorIds[sideLeaf]].GetLeafIndex() )
        {
        // Two cursors are the same.
        // We are not inserting face or edge points.
        continue;
        }
      if ( this->Dimension > 1 )
        {
        sideLeaf = leaf^2;
        if ( superCursor->Cursors[cornerCursorIds[sideLeaf]].GetTree() &&
             leafId == superCursor->Cursors[cornerCursorIds[sideLeaf]].GetLeafIndex() )
          {
          // Two cursors are the same.
          // We are not inserting face or edge points.
          continue;
          }
        }
      if ( this->Dimension > 2 )
        {
        sideLeaf = leaf^4;
        if ( superCursor->Cursors[cornerCursorIds[sideLeaf]].GetTree() &&
             leafId == superCursor->Cursors[cornerCursorIds[sideLeaf]].GetLeafIndex() )
          {
          // Two cursors are the same.
          // We are not inserting face or edge points.
          continue;
          }
        }
      // Center point is opposite to the leaf position in supercursor.
      leafId += superCursor->Cursors[cornerCursorIds[leaf]].GetOffset();
      this->LeafCornerIds->InsertComponent( leafId, 
                                            numLeaves - leaf - 1,
                                            static_cast<double>( cornerId ) );
      }
    }

  return cornerId;
}



//-----------------------------------------------------------------------------
vtkPoints* vtkHyperTreeGrid::GetCornerPoints()
{
  this->UpdateGridArrays();
  return this->CornerPoints;
}

//-----------------------------------------------------------------------------
vtkIdTypeArray* vtkHyperTreeGrid::GetLeafCornerIds()
{
  this->UpdateGridArrays();
  return this->LeafCornerIds;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGrid::UpdateHyperTreesLeafIdOffsets()
{
  if ( this->HyperTreesLeafIdOffsets )
    {
    delete [] this->HyperTreesLeafIdOffsets;
    this->HyperTreesLeafIdOffsets = 0;
    }
  this->HyperTreesLeafIdOffsets = new vtkIdType[this->NumberOfRoots];

  // Calculate point offsets into individual trees
  int numLeaves = 0;
  int idx = 0;
  vtkCollectionSimpleIterator it;
  for ( this->HyperTrees->InitTraversal( it );
        vtkObject* obj = this->HyperTrees->GetNextItemAsObject( it ) ; ++ idx )
    {
    if ( obj )
      {
      vtkHyperTree* tree = static_cast<vtkHyperTree*>( obj );
      // Partial sum is current offset
      this->HyperTreesLeafIdOffsets[idx] = numLeaves;

      // Update partial sum
      numLeaves += tree->GetNumberOfLeaves();
      }
    }

  return numLeaves;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::UpdateGridArrays()
{
  int numLeaves = this->UpdateHyperTreesLeafIdOffsets();

  // Check if we can break out early
  if ( this->LeafCornerIds )
    {
    if ( this->LeafCornerIds->GetNumberOfTuples() == numLeaves )
      {
      return;
      }
    this->LeafCornerIds->UnRegister( this );
    this->LeafCornerIds = 0;
    this->CornerPoints->UnRegister( this );
    this->CornerPoints = 0;
    }

  vtkTimerLog* timer = vtkTimerLog::New();
  timer->StartTimer();

  // Primal corner points
  this->CornerPoints = vtkPoints::New();
  this->CornerPoints->Allocate( numLeaves );

  this->LeafCornerIds = vtkIdTypeArray::New();
  int dim = this->GetDimension();
  int numComps = 1 << dim;
  this->LeafCornerIds->SetNumberOfComponents( numComps );
  //this->LeafCornerIds->SetNumberOfTuples( numLeaves );

  // Create an array of cursors that occupy 1 3x3x3 neighborhhood.  This
  // will traverse the tree as one.
  // Lower dimensions will not use them all.
  this->GenerateSuperCursorTraversalTable();

  // Iterate over all hyper trees
  for ( unsigned int k = 0; k < this->GridSize[2]; ++ k )
    {
    for ( unsigned int j = 0; j < this->GridSize[1]; ++ j )
      {
      for ( unsigned int i = 0; i < this->GridSize[0]; ++ i )
        {
        // Storage for super cursors
        vtkHyperTreeGridSuperCursor superCursor;

        // Initialize center cursor
        this->InitializeSuperCursor( &superCursor, i, j, k );

        // Create a mask array to keep a record of which leaves have already
        // generated their corner cell entries
        unsigned char* leafMask = new unsigned char[numLeaves];

        // Initialize to 0
        memset( leafMask, 0, numLeaves );
        
        // Traverse and populate dual recursively
        this->TraverseGridRecursively( &superCursor, leafMask);

        delete [] leafMask;
        } // i
      } // j
    } // k

  timer->StopTimer();
  cerr << "Internal grid update : " << timer->GetElapsedTime() << endl;
  timer->UnRegister( this );
}

// NB: Primal method creates the corner points on the boundaries of the tree.
// Dual method does not.
//----------------------------------------------------------------------------
// The purpose of traversing the supercursor / cells is to visit
// every corner and have the leaves connected to that corner.
void vtkHyperTreeGrid::TraverseGridRecursively( vtkHyperTreeGridSuperCursor* superCursor,
                                                unsigned char* visited)
{
  // This is the number of corners that a leaf has (valid for all hypertrees)
  int numCorners = 1 << this->Dimension;

  int cornerId;
  int cornerIds[8];
  int level = superCursor->GetCursor( 0 )->GetLevel();
  if ( superCursor->GetCursor( 0 )->IsLeaf() )
    {
    // Center is a leaf.
    // Evaluate each corner to see if we should process it now.
    // This is looping over the 8 corner points of the center cursor leaf.
    for ( int corner = 0; corner < numCorners; ++ corner )
      {
      // We will not use all of these if dim < 3, but generate anyway.
      // These are the cursor index (into the supercursor) of the eight
      // cursors (nodes) surrounding the corner.
      cornerIds[0] = (corner&1) + 3*((corner>>1)&1) + 9*((corner>>2)&1);
      cornerIds[1] = cornerIds[0] + 1;
      cornerIds[2] = cornerIds[0] + 3;
      cornerIds[3] = cornerIds[1] + 3;
      cornerIds[4] = cornerIds[0] + 9;
      cornerIds[5] = cornerIds[1] + 9;
      cornerIds[6] = cornerIds[2] + 9;
      cornerIds[7] = cornerIds[3] + 9;
      cornerId = this->EvaluateGridCorner( level,
                                           superCursor,
                                           visited,
                                           cornerIds );
      if ( cornerId >= 0 )
        {
        // A bit funny inserting the point here, but we need to determine
        // the id for the corner leaves in EvaluateGridCorner, and we
        // do not want to compute the point unless absolutely necessary.
        double pt[3];

        // Create the corner point.
        pt[0] = superCursor->Origin[0];
        if ( corner&1 )
          {
          pt[0] += superCursor->Size[0];
          }
        pt[1] = superCursor->Origin[1];
        if ( (corner>>1)&1 )
          {
          pt[1] += superCursor->Size[1];
          }
        pt[2] = superCursor->Origin[2];
        if ( (corner>>2)&1 )
          {
          pt[2] += superCursor->Size[2];
          }
        this->CornerPoints->InsertPoint( cornerId, pt );
        }
      }
    // Mark this leaf as visited.
    // Neighbor value is leafId for leaves, nodeId for nodes.
    visited[superCursor->GetCursor( 0 )->GetLeafIndex()] = 1;
    return;
    } // if ( superCursor->GetCursor( 0 )->IsLeaf() )

  // Center is not a leaf, continue recursion
  for ( unsigned int child = 0; child < this->NumberOfChildren; ++ child )
    {
    vtkHyperTreeGridSuperCursor newSuperCursor;
    this->InitializeSuperCursorChild( superCursor,&newSuperCursor, child );
    this->TraverseGridRecursively( &newSuperCursor, visited );
    }
}

//----------------------------------------------------------------------------
// This table is used to move a 3x3x3 neighborhood of cursors through the tree.
void vtkHyperTreeGrid::GenerateSuperCursorTraversalTable()
{
  int xChildDim = 1;
  int yChildDim = 1;
  int zChildDim = 1;
  int xCursorDim = 1;
  int yCursorDim = 1;
  int zCursorDim = 1;

  assert( "Dimension cannot be 0." && this->GetDimension() );

  switch ( this->GetDimension() )
    {
    case 1:
      xChildDim = this->BranchFactor;
      xCursorDim = 3;
      break;
    case 2:
      xChildDim = yChildDim = this->BranchFactor;
      xCursorDim = yCursorDim = 3;
      break;
    case 3:
      xChildDim = yChildDim = zChildDim = this->BranchFactor;
      xCursorDim = yCursorDim = zCursorDim = 3;
      break;
    }

  int fac = this->BranchFactor;
  int childIdx = 0;
  for ( int zChild = 0; zChild < zChildDim; ++ zChild )
    {
    for ( int yChild = 0; yChild < yChildDim; ++ yChild )
      {
      for ( int xChild = 0; xChild < xChildDim; ++ xChild )
        {
        int cursorIdx = 0;
        for ( int zCursor = 0; zCursor < zCursorDim; ++ zCursor )
          {
          for ( int yCursor = 0; yCursor < yCursorDim; ++ yCursor )
            {
            for ( int xCursor = 0; xCursor < xCursorDim; ++ xCursor )
              {
              // Compute the x, y, z index into the
              // 6x6x6 (9x9x9) neighborhood of children.
              int xNeighbor = xCursor + xChild + xChildDim - 1;
              int yNeighbor = yCursor + yChild + yChildDim - 1;
              int zNeighbor = zCursor + zChild + zChildDim - 1;

              // Separate neighbor index into Cursor/Child index.
              int xNewCursor = xNeighbor / fac;
              int yNewCursor = yNeighbor / fac;
              int zNewCursor = zNeighbor / fac;
              int xNewChild = xNeighbor - xNewCursor * fac;
              int yNewChild = yNeighbor - yNewCursor * fac;
              int zNewChild = zNeighbor - zNewCursor * fac;
              int tableIdx = childIdx * 27 + cursorIdx;
              this->SuperCursorTraversalTable[tableIdx].Parent
                = xNewCursor + 3 * ( yNewCursor + 3 * zNewCursor );
              this->SuperCursorTraversalTable[tableIdx].Child
                = xNewChild + fac * ( yNewChild + fac * zNewChild );
              ++ cursorIdx;
              }
            }
          }
        ++ childIdx;
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::DeleteInternalArrays()
{
  if ( this->HyperTreesLeafIdOffsets )
    {
    delete [] this->HyperTreesLeafIdOffsets;
    this->HyperTreesLeafIdOffsets = 0;
    }

  if ( this->LeafCenters )
    {
    this->LeafCenters->UnRegister( this );
    this->LeafCenters = 0;
    }

  if ( this->LeafCenterIds )
    {
    this->LeafCenterIds->UnRegister( this );
    this->LeafCenterIds = 0;
    }

  if ( this->CornerPoints )
    {
    this->CornerPoints->UnRegister( this );
    this->CornerPoints = 0;
    }

  if ( this->LeafCornerIds )
    {
    this->LeafCornerIds->UnRegister( this );
    this->LeafCornerIds = 0;
    }

  if ( this->Links )
    {
    this->Links->UnRegister( this );
    this->Links = 0;
    }
}

//=============================================================================
// Hyper tree grid cursor
// Implemented here because it needs access to the internal classes.

//-----------------------------------------------------------------------------
// Constructor.
vtkHyperTreeSimpleCursor::vtkHyperTreeSimpleCursor()
{
  this->Tree = 0;
  this->Index = 0;
  this->Offset = 0;
  this->Leaf = false;
  this->Level = 0;
}


//-----------------------------------------------------------------------------
// Destructor.
vtkHyperTreeSimpleCursor::~vtkHyperTreeSimpleCursor()
{
  this->Tree = 0;
}


//-----------------------------------------------------------------------------
// Set the state back to the initial contructed state
void vtkHyperTreeSimpleCursor::Clear()
{
  this->Tree = 0;
  this->Index = 0;
  this->Offset = 0;
  this->Leaf = false;
  this->Level = 0;
}

//-----------------------------------------------------------------------------²
void vtkHyperTreeSimpleCursor::Initialize( vtkHyperTreeGrid* grid,
                                           vtkIdType* offsets,
                                           int index,
                                           int pos[3] )
{ 
  // Convert local index into global one
  unsigned int n[3];
  grid->GetGridSize( n );
  int globalIndex = index + pos[0] + pos[1] * n[0] + pos[2] * n[0] * n[1];
  // Assign leaf ID offset to this cursor
  this->Offset = offsets[globalIndex];

  // Assign hypertree to this cursor
  vtkObject* obj = grid->HyperTrees->GetItemAsObject( globalIndex );
  if ( obj )
    {
    vtkHyperTree* tree = static_cast<vtkHyperTree*>( obj );
    this->Tree = tree;
    }
  else
    {
    return;
    }

  this->ToRoot();
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeSimpleCursor::IsLeaf()
{
  // Empty cursors appear like a leaf so that recursion stop
  if ( ! this->Tree )
    {
    return true;
    }

  return this->Leaf;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeSimpleCursor::ToRoot()
{
  if ( ! this->Tree )
    {
    return;
    }

  // Return to root level
  this->Level = 0;

  if ( this->Tree->GetNumberOfLeaves() == 1 )
    {
    // Root is a leaf.
    this->Index = 0;
    this->Leaf = true;
    }
  else
    {
    // Root is a node.
    this->Index = 1; // First node ( 0 ) is a special empty node.
    this->Leaf = false;
    }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeSimpleCursor::ToChild( int child )
{
  if ( ! this->Tree )
    {
    return;
    }
  if ( this->Leaf )
    {
    // Leaves do not have children.
    return;
    }

  this->Tree->FindChildParameters( child, this->Index, this->Leaf );

  ++ this->Level;

  assert( "Bad index" && this->Index >= 0 );
  if ( this->Leaf )
    {
    assert( "Bad leaf index" && this->Index < this->Tree->GetNumberOfLeaves() );
    }
  else
    {
    assert( "Bad node index" && this->Index < this->Tree->GetNumberOfNodes() );
    }
}
