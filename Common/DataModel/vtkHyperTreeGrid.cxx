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

#include "vtkHyperTreeGridOrientedCursor.h"
#include "vtkHyperTreeGridOrientedGeometryCursor.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridNonOrientedVonNeumannSuperCursor.h"
#include "vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight.h"
#include "vtkHyperTreeGridNonOrientedMooreSuperCursor.h"
#include "vtkHyperTreeGridNonOrientedMooreSuperCursorLight.h"

#include "vtkHyperTreeGridScales.h"

#include <cassert>
#include <deque>
#include <array>

vtkInformationKeyMacro( vtkHyperTreeGrid, LEVELS, Integer );
vtkInformationKeyMacro( vtkHyperTreeGrid, DIMENSION, Integer );
vtkInformationKeyMacro( vtkHyperTreeGrid, ORIENTATION, Integer );
vtkInformationKeyRestrictedMacro( vtkHyperTreeGrid, SIZES, DoubleVector, 3 );

vtkStandardNewMacro( vtkHyperTreeGrid );
vtkCxxSetObjectMacro( vtkHyperTreeGrid, XCoordinates, vtkDataArray );
vtkCxxSetObjectMacro( vtkHyperTreeGrid, YCoordinates, vtkDataArray );
vtkCxxSetObjectMacro( vtkHyperTreeGrid, ZCoordinates, vtkDataArray );

void vtkHyperTreeGrid::SetMaterialMask ( vtkBitArray* _arg)
{
  vtkSetObjectBodyMacro( MaterialMask, vtkBitArray, _arg );

  this->InitPureMaterialMask = false;
  if( this->PureMaterialMask)
  {
    this->PureMaterialMask->Delete();
    this->PureMaterialMask = nullptr;
  }
}

// Helper macros to quickly fetch a HT at a given index or iterator
#define GetHyperTreeFromOtherMacro( _obj_, _index_ ) \
  ( static_cast<vtkHyperTree*>( _obj_->HyperTrees.find( _index_ ) \
                                != _obj_->HyperTrees.end() ? \
                                _obj_->HyperTrees[ _index_ ] : 0 ) )
#define GetHyperTreeFromThisMacro( _index_ ) GetHyperTreeFromOtherMacro( this, _index_ )

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
vtkHyperTreeGrid::vtkHyperTreeGrid()
{
  // Default state
  this->FrozenState = false;

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

  // Invalid default grid parameters to force actual initialization
  this->Dimension = 0;
  this->Orientation = UINT_MAX;
  this->BranchFactor = 0;
  this->NumberOfChildren = 0;

  // Masked primal leaves
  this->MaterialMask = vtkBitArray::New();
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
void vtkHyperTreeGrid::Squeeze()
{
  if ( ! this->FrozenState ) {
    vtkHyperTreeGridIterator itIn;
    InitializeTreeIterator( itIn );
    vtkIdType indexIn;
    while ( vtkHyperTree *ht = itIn.GetNextTree( indexIn  ) )
    {
      vtkHyperTree *htfrozen = ht->Freeze();
      if ( htfrozen != ht )
      {
        this->SetTree( indexIn, htfrozen );
        htfrozen->UnRegister( this );
      }
    }
    this->FrozenState = true;
  }
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

  os << indent << "Frozen: " << this->FrozenState << endl;
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
  assert( "pre: ds_exists" && ds != nullptr );
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast( ds );
  assert( "pre: same_type" && htg != nullptr );

  // Copy grid parameters
  this->FrozenState = htg->FrozenState;
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

  // Delete existing trees
  DeleteTrees();

  // Search for hyper tree with given index
  this->HyperTrees.clear();

  for( auto it = htg->HyperTrees.begin(); it != htg->HyperTrees.end(); ++it )
  {
    vtkIdType treeindex = it->first;

    vtkHyperTree* tree = GetHyperTreeFromThisMacro( treeindex );
    tree = vtkHyperTree::CreateInstance( this->BranchFactor, this->Dimension );
    assert( "pre: same_type" && tree != nullptr );
    tree->CopyStructure( it->second );
    this->HyperTrees[ it->first ] = tree;
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGrid::SetGridSize( unsigned int dim[3] )
{
  this->SetGridExtent( 0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1 );
}

//----------------------------------------------------------------------------
void vtkHyperTreeGrid::SetGridSize( unsigned int i, unsigned int j, unsigned int k )
{
  this->SetGridExtent( 0, i - 1, 0, j - 1, 0, k - 1 );
}

//----------------------------------------------------------------------------
void vtkHyperTreeGrid::SetGridExtent( int extent[6] )
{
  int description
    = vtkStructuredData::SetExtent( extent, this->Extent );
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
void vtkHyperTreeGrid::SetGridExtent( int i0, int i1, int j0, int j1, int k0, int k1)
{
  int extent[6];

  extent[0] = i0;
  extent[1] = i1;
  extent[2] = j0;
  extent[3] = j1;
  extent[4] = k0;
  extent[5] = k1;

  this->SetGridExtent( extent );
}

//----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetGridExtent( int extent[6] )
{
  extent[0] = 0;
  extent[1] = this->GridSize[0] - 1;
  extent[2] = 0;
  extent[3] = this->GridSize[1] - 1;
  extent[4] = 0;
  extent[5] = this->GridSize[2] - 1;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::SetDimension( unsigned int dim )
{
  assert( "pre: valid_dim" && dim >= 1 && dim <= 3 );

  // Make sure that number of children is factor^dimension
  unsigned int num = this->BranchFactor;
  for ( unsigned int i = 1; i < dim; ++ i )
  {
    num *= this->BranchFactor;
  }

  // Bail out early if nothing was changed
  if ( this->Dimension == dim && this->NumberOfChildren == num )
  {
    return;
  }

  // Otherwise modify as needed
  this->Dimension = dim;
  this->NumberOfChildren = num;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::SetBranchFactor( unsigned int factor )
{
  assert( "pre: valid_factor" && factor >= 2 && factor <= 3 );

  // Make sure that number of children is factor^dimension
  unsigned int num = factor;
  for ( unsigned int i = 1; i < this->Dimension; ++ i )
  {
    num *= factor;
  }

  // Bail out early if nothing was changed
  if ( this->BranchFactor == factor && this->NumberOfChildren == num )
  {
    return;
  }

  // Otherwise modify as needed
  this->BranchFactor = factor;
  this->NumberOfChildren = num;
  this->Modified();
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGrid::HasMaterialMask()
{
  return ( this->MaterialMask->GetNumberOfTuples() != 0 );
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
vtkIdType vtkHyperTreeGrid::GetMaxNumberOfTrees()
{
  return this->GridSize[0] * this->GridSize[1] * this->GridSize[2];
}

//-----------------------------------------------------------------------------
unsigned int vtkHyperTreeGrid::GetNumberOfLevels( vtkIdType index )
{
  vtkHyperTree* tree = GetHyperTreeFromThisMacro( index );
  return tree ? tree->GetNumberOfLevels() : 0;
}

//-----------------------------------------------------------------------------
unsigned int vtkHyperTreeGrid::GetNumberOfLevels()
{
  vtkIdType nLevels = 0;

  // Iterate over all individual trees
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  this->InitializeTreeIterator( it );
  vtkHyperTree* tree = nullptr;
  while ( ( tree = it.GetNextTree( ) ) != 0  )
  {
    const vtkIdType nl = tree->GetNumberOfLevels();
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
void vtkHyperTreeGrid::InitializeOrientedCursor(
  vtkHyperTreeGridOrientedCursor* cursor,
  vtkIdType index,
  bool create )
{
  cursor->Initialize( this, index, create );
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridOrientedCursor*
vtkHyperTreeGrid::NewOrientedCursor( vtkIdType index,
                                     bool create )
{
  vtkHyperTreeGridOrientedCursor* cursor =
    vtkHyperTreeGridOrientedCursor::New();
  cursor->Initialize( this, index, create );
  return cursor;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::InitializeOrientedGeometryCursor(
  vtkHyperTreeGridOrientedGeometryCursor* cursor,
  vtkIdType index,
  bool create
)
{
  cursor->Initialize( this, index, create );
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridOrientedGeometryCursor*
vtkHyperTreeGrid::NewOrientedGeometryCursor( vtkIdType index,
                                                bool create
)
{
  vtkHyperTreeGridOrientedGeometryCursor* cursor =
    vtkHyperTreeGridOrientedGeometryCursor::New();
  cursor->Initialize( this, index, create );
  return cursor;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::InitializeNonOrientedCursor(
  vtkHyperTreeGridNonOrientedCursor* cursor,
  vtkIdType index,
  bool create )
{
  cursor->Initialize( this, index, create );
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedCursor*
vtkHyperTreeGrid::NewNonOrientedCursor( vtkIdType index,
                                        bool create )
{
  vtkHyperTreeGridNonOrientedCursor* cursor =
    vtkHyperTreeGridNonOrientedCursor::New();
  cursor->Initialize( this, index, create );
  return cursor;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::InitializeNonOrientedGeometryCursor(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor,
  vtkIdType index,
  bool create
)
{
  cursor->Initialize( this, index, create );
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedGeometryCursor*
vtkHyperTreeGrid::NewNonOrientedGeometryCursor( vtkIdType index,
                                                bool create
)
{
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor =
    vtkHyperTreeGridNonOrientedGeometryCursor::New();
  cursor->Initialize( this, index, create );
  return cursor;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::InitializeNonOrientedVonNeumannSuperCursor(
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* cursor,
  vtkIdType index,
  bool create
)
{
  cursor->Initialize( this, index, create );
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedVonNeumannSuperCursor*
vtkHyperTreeGrid::NewNonOrientedVonNeumannSuperCursor(
  vtkIdType index,
  bool create
)
{
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* cursor =
    vtkHyperTreeGridNonOrientedVonNeumannSuperCursor::New();
  cursor->Initialize( this, index, create );
  return cursor;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::InitializeNonOrientedVonNeumannSuperCursorLight(
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight* cursor,
  vtkIdType index,
  bool create
)
{
  cursor->Initialize( this, index, create );
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight*
vtkHyperTreeGrid::NewNonOrientedVonNeumannSuperCursorLight(
  vtkIdType index,
  bool create
)
{
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight* cursor =
    vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight::New();
  cursor->Initialize( this, index, create );
  return cursor;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::InitializeNonOrientedMooreSuperCursor(
  vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor,
  vtkIdType index,
  bool create
)
{
  cursor->Initialize( this, index, create );
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedMooreSuperCursor*
vtkHyperTreeGrid::NewNonOrientedMooreSuperCursor( vtkIdType index,
                                                  bool create
)
{
  vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor =
    vtkHyperTreeGridNonOrientedMooreSuperCursor::New();
  cursor->Initialize( this, index, create );
  return cursor;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::InitializeNonOrientedMooreSuperCursorLight(
  vtkHyperTreeGridNonOrientedMooreSuperCursorLight* cursor,
  vtkIdType index,
  bool create
)
{
  cursor->Initialize( this, index, create );
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedMooreSuperCursorLight*
vtkHyperTreeGrid::NewNonOrientedMooreSuperCursorLight( vtkIdType index,
                                                  bool create
)
{
  vtkHyperTreeGridNonOrientedMooreSuperCursorLight* cursor =
    vtkHyperTreeGridNonOrientedMooreSuperCursorLight::New();
  cursor->Initialize( this, index, create );
  return cursor;
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
vtkHyperTree* vtkHyperTreeGrid::GetTree( vtkIdType index, bool create )
{
  // Wrap convenience macro for outside use
  vtkHyperTree* tree = GetHyperTreeFromThisMacro( index );

  // Create a new cursor if only required to do so
  if( create && ! tree )
  {
    tree = vtkHyperTree::CreateInstance( this->BranchFactor, this->Dimension );
    tree->SetTreeIndex( index );
    this->HyperTrees[index] = tree;

    // JB pour initialiser le scales au niveau de HT
    // Esperons qu'aucun HT n'est cree hors de l'appel a cette methode
    // Ce service ne devrait pas exister ou etre visible car c'est au niveau d'un HT ou d'un
    // cursor que cet appel est fait
    if ( ! tree->HasScales() )
    {
      double origin[3];
      double scale[3];
      this->GetLevelZeroOriginAndSizeFromIndex( tree->GetTreeIndex(), origin, scale );
      tree->SetScales( std::make_shared<vtkHyperTreeGridScales>( this->BranchFactor, scale ) );
    }
  }

  return tree;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::SetTree( vtkIdType index, vtkHyperTree* tree )
{
  // Search for hyper tree with given index
  std::map<vtkIdType, vtkHyperTree*>::iterator it = this->HyperTrees.find( index );

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
  tree->SetTreeIndex( index );
  this->HyperTrees[ index ] = tree;

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
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast( src );
  assert( "src_same_type" && htg );

  // Copy member variables
  this->CopyStructure( htg );

  // Call superclass
  this->Superclass::ShallowCopy( src );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::DeepCopy( vtkDataObject* src )
{
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast( src );
  assert( "src_same_type" && htg );
  //this->CopyStructure( htg );

  // FIXME: some member variables are missing here
  this->FrozenState = htg->FrozenState;
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
bool vtkHyperTreeGrid::RecursivelyInitializePureMaterialMask( vtkHyperTreeGridNonOrientedCursor* cursor, vtkDataArray* normale )
{
  // Retrieve mask value at cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();
  bool mask = this->HasMaterialMask() && this->MaterialMask->GetValue( id  );

  if ( ! mask && normale )
  {
     double values[3];
     normale->GetTuple( id, values);
     //FR Retrieve cell interface value at cursor (is interface if one value is non null)
     bool isInterface = ( values[0] != 0 || values[1] != 0 || values[2] != 0 );
     //FR Cell with interface is considered as "not pure"
     mask = isInterface;
  }

  //  Dot recurse if node is masked or is a leaf
  if( ! mask && ! cursor->IsLeaf() )
  {
    // Iterate over all chidren
    unsigned int numChildren = this->GetNumberOfChildren();
    bool pure = false;
    for ( unsigned int child = 0; child < numChildren; ++ child )
    {
      cursor->ToChild( child );
      // FR Obligatoire en profondeur afin d'associer une valeur a chaque maille
      pure |= this->RecursivelyInitializePureMaterialMask( cursor, normale );
      cursor->ToParent( );
    }
    // Set and return pure material mask with recursively computed value
    this->PureMaterialMask->SetTuple1( id, pure );
    return pure;
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
    if ( ! this->MaterialMask->GetNumberOfTuples() )
    {
      // Keep track of the fact that a pure material mask now exists
      this->InitPureMaterialMask = true;
      return nullptr;
    }
    // If not, then create one
    this->PureMaterialMask = vtkBitArray::New();
    this->PureMaterialMask->SetNumberOfTuples( this->MaterialMask->GetNumberOfTuples() );

    // Iterate over hyper tree grid
    vtkIdType index;
    vtkHyperTreeGridIterator it;
    it.Initialize( this );

    vtkDataArray *normale = nullptr;
    if ( this->HasInterface )
    {
      // Interface defined
      normale = this->GetPointData()->GetArray( this->InterfaceNormalsName );
    }

    vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
    while ( it.GetNextTree( index ) )
    {
      // Create cursor instance over current hyper tree
      this->InitializeNonOrientedCursor( cursor, index );
      // Recursively initialize pure material mask
      this->RecursivelyInitializePureMaterialMask( cursor, normale );
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

  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  this->InitializeNonOrientedCursor( cursor.Get(), index );

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

  return id;
}

//----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::RecursivelyFindPoint( double x[3],
                                                 vtkHyperTreeGridNonOrientedCursor* cursor,
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
unsigned long vtkHyperTreeGrid::GetActualMemorySizeBytes()
{
  size_t size = 0; // in bytes

  size += this->vtkDataSet::GetActualMemorySize() << 10;

  // Iterate over all trees in grid
  vtkHyperTreeGridIterator it;
  it.Initialize( this );
  while ( vtkHyperTree* tree = it.GetNextTree() )
  {
    size += tree->GetActualMemorySizeBytes();
  }

  // Approximate map memory size
  size += this->HyperTrees.size() * sizeof(vtkIdType) * 3;

  if ( this->XCoordinates )
  {
    size += this->XCoordinates->GetActualMemorySize() << 10;
  }

  if ( this->YCoordinates )
  {
    size += this->YCoordinates->GetActualMemorySize() << 10;
  }

  if ( this->ZCoordinates )
  {
    size += this->ZCoordinates->GetActualMemorySize() << 10;
  }

  if ( this->Points )
  {
    size += this->Points->GetActualMemorySize() << 10;
  }

  if ( this->Connectivity )
  {
    size += this->Connectivity->GetActualMemorySize() << 10;
  }

  if ( this->MaterialMask )
  {
    size += this->MaterialMask->GetActualMemorySize() << 10;
  }

  return static_cast<unsigned long>(size);
}

//----------------------------------------------------------------------------
unsigned long vtkHyperTreeGrid::GetActualMemorySize()
{
  // in kilibytes
  return ( this->GetActualMemorySizeBytes() >> 10 );
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
unsigned int vtkHyperTreeGrid::GetShiftedLevelZeroIndex( vtkIdType treeindex,
                                                         int i,
                                                         int j,
                                                         int k )
{
  // Distinguish between two cases depending on indexing order
  return this->TransposedRootIndexing ?
    treeindex + (k +
             j * static_cast<int>(this->GridSize[2]) +
             i * static_cast<int>(this->GridSize[2] * this->GridSize[1]))
    :
    treeindex + (i +
             j * static_cast<int>(this->GridSize[0]) +
             k * static_cast<int>(this->GridSize[0] * this->GridSize[1]));
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetLevelZeroCoordinatesFromIndex(
  vtkIdType treeindex,
  unsigned int &i,
  unsigned int &j,
  unsigned int &k ) const
{
  // Distinguish between two cases depending on indexing order
  if ( ! this->TransposedRootIndexing )
  {
    k = treeindex / ( this->GridSize[0] * this->GridSize[1] );
    vtkIdType rk = k * ( this->GridSize[0] * this->GridSize[1] );
    j = ( treeindex - rk ) / this->GridSize[0];
    i = treeindex - ( j * this->GridSize[0] ) - rk;
  }
  else
  {
    i = treeindex / ( this->GridSize[2] * this->GridSize[1] );
    vtkIdType rk = i * ( this->GridSize[2] * this->GridSize[1] );
    j = ( treeindex - rk ) / this->GridSize[2];
    k = treeindex - ( j * this->GridSize[2] ) - rk;
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetLevelZeroOriginAndSizeFromIndex(
  vtkIdType treeindex,
  double *Origin,
  double *Size )
{
  // Compute origin and size of the cursor
  unsigned int i, j, k;
  this->GetLevelZeroCoordinatesFromIndex( treeindex, i, j, k );

  vtkDataArray* xCoords = this->GetXCoordinates();
  vtkDataArray* yCoords = this->GetYCoordinates();
  vtkDataArray* zCoords = this->GetZCoordinates();
  Origin[0] = xCoords->GetTuple1( i );
  Origin[1] = yCoords->GetTuple1( j );
  Origin[2] = zCoords->GetTuple1( k );
  Size[0] = xCoords->GetTuple1( i + 1 ) - Origin[0];
  Size[1] = yCoords->GetTuple1( j + 1 ) - Origin[1];
  Size[2] = zCoords->GetTuple1( k + 1 ) - Origin[2];
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetLevelZeroOriginFromIndex(
  vtkIdType treeindex,
  double *Origin )
{
  // Compute origin and size of the cursor
  unsigned int i, j, k;
  this->GetLevelZeroCoordinatesFromIndex( treeindex, i, j, k );

  vtkDataArray* xCoords = this->GetXCoordinates();
  vtkDataArray* yCoords = this->GetYCoordinates();
  vtkDataArray* zCoords = this->GetZCoordinates();
  Origin[0] = xCoords->GetTuple1( i );
  Origin[1] = yCoords->GetTuple1( j );
  Origin[2] = zCoords->GetTuple1( k );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetIndexFromLevelZeroCoordinates(
  vtkIdType& treeindex,
  unsigned int i,
  unsigned int j,
  unsigned int k ) const
{
  // Distinguish between two cases depending on indexing order
  if ( ! this->TransposedRootIndexing )
  {
    treeindex = i + j * this->GridSize[0]
      + k * ( this->GridSize[0] * this->GridSize[1] );
  }
  else
  {
    treeindex = k + j * this->GridSize[2]
      + i * ( this->GridSize[2] * this->GridSize[1] );
  }
}

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGrid::GetGlobalNodeIndexMax()
{
  // Iterate over all hyper trees
  vtkIdType index;
  vtkHyperTreeGridIterator it;
  this->InitializeTreeIterator( it );
  vtkNew<vtkHyperTreeGridNonOrientedMooreSuperCursor> cursor;
  vtkIdType max = 0;
  while ( it.GetNextTree( index ) )
  {
    // Initialize new Moore cursor at root of current tree
    this->InitializeNonOrientedMooreSuperCursor( cursor, index );
    max = std::max( max, cursor->GetTree()->GetGlobalNodeIndexMax() );
  } // it
  return max;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::InitializeLocalIndexNode()
{
  // Iterate over all hyper trees
  vtkIdType index;
  vtkHyperTreeGridIterator it;
  this->InitializeTreeIterator( it );
  vtkNew<vtkHyperTreeGridNonOrientedMooreSuperCursor> cursor;
  vtkIdType local = 0;
  while ( it.GetNextTree( index ) )
  {
    // Initialize new Moore cursor at root of current tree
    this->InitializeNonOrientedMooreSuperCursor( cursor, index );
    cursor->SetGlobalIndexStart( local );
    local += cursor->GetTree()->GetNumberOfVertices();
  } // it
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::ComputeDualGrid()
{
  // Check if we can break out early
  if ( this->Points )
  {
    return;
  }

#ifndef NDBUG
  cerr << "Warning: Automatic compute dual grid" << endl;
#endif

  // Create arrays needed by dual mesh
  this->Points = vtkPoints::New();
  this->Connectivity = vtkIdTypeArray::New();

  // Primal cell centers are dual points
  // JB On ne peut pas se reduire a dimensionner le tableau de points au nombre
  // de Vertices, en effet, surtout si l'on veut conserver le mapping 1:1
  // entre les noeuds de l'HTG et ces points du maillage dual.
  // En effet, si l'on definit un GlobalIndex ou un IndexStart specifique
  // cette ecriture simpliste ne fonctionnait plus... tableau trop petit
  // car GetGlobalIndex retourne une valeur > this->GetNumberOfVertices().
  this->Points->SetNumberOfPoints( this->GetGlobalNodeIndexMax() + 1 );

  int numVerts = 1 << this->Dimension;
  this->Connectivity->SetNumberOfComponents( numVerts );

  // Initialize grid depth
  unsigned int gridDepth = 0;

  // Compute and assign scales of all tree roots
  // double scale[3] = { 1., 1., 1. };

  // Check whether coordinate arrays match grid size
  // If coordinates array are complete, compute all tree scales
  if ( static_cast<int>( this->GridSize[0] ) + 1 == this->XCoordinates->GetNumberOfTuples()
       && static_cast<int>( this->GridSize[1] ) + 1 == this->YCoordinates->GetNumberOfTuples()
       && static_cast<int>( this->GridSize[2] ) + 1 == this->ZCoordinates->GetNumberOfTuples() )
  {
    gridDepth = this->GetNumberOfLevels();
  }

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
  vtkNew<vtkHyperTreeGridNonOrientedMooreSuperCursor> cursor;
  while ( it.GetNextTree( index ) )
  {
    // Initialize new Moore cursor at root of current tree
    this->InitializeNonOrientedMooreSuperCursor( cursor, index );
    // Convert hyper tree into unstructured mesh recursively
    if ( mask )
    {
      this->TraverseDualRecursively( cursor, mask );
    }
    else
    {
      this->TraverseDualRecursively( cursor );
    }
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

      assert ( _it->first < this->GetNumberOfPoints() );
      this->Points->GetPoint( _it->first, pt );

      pt[d] += _it->second;

      assert ( _it->first < this->GetNumberOfPoints() );
      this->Points->SetPoint( _it->first, pt );
    } // it
    this->PointShifts[d].clear();
  } // d
  this->PointShifted.clear();
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::TraverseDualRecursively( vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor )
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
      cursor->ToChild( child );
      // Recurse
      this->TraverseDualRecursively( cursor );
      cursor->ToParent( );
    } // child
  } // else
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::TraverseDualRecursively( vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor,
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
      cursor->ToChild( child );
      // Recurse
      this->TraverseDualRecursively( cursor, mask );
      cursor->ToParent( );
    } // child
  } // else
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GenerateDualCornerFromLeaf1D( vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor )
{
  // With d=1:
  //   (d-0)-faces are corners, neighbor cursors are 0 and 2
  //   (d-1)-faces do not exist
  //   (d-2)-faces do not exist

  // Retrieve neighbor (left/right) cursors
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorL = cursor->GetOrientedGeometryCursor( 0 );
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorR = cursor->GetOrientedGeometryCursor( 2 );

  // Retrieve cursor center coordinates
  double pt[3];
  cursor->GetPoint( pt );

  // Check across d-face neighbors whether point must be adjusted
  if ( ! cursorL->HasTree() )
  {
    // Move to left corner
    pt[this->Orientation] -= .5 * cursor->GetSize()[this->Orientation];;
  }
  if ( ! cursorR->HasTree() )
  {
    // Move to right corner
    pt[this->Orientation] += .5 * cursor->GetSize()[this->Orientation];;
  }

  // Retrieve global index of center cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();

  // Insert dual point at center of leaf cell
  assert ( id < this->GetNumberOfPoints() );
  this->Points->SetPoint( id, pt );

  // Storage for edge vertex IDs: dual cell ownership to cursor with higher index
  vtkIdType ids[2];
  ids[0] = id;

  // Check whether a dual edge to left neighbor exists
  if ( cursorL->HasTree() && cursorL->IsLeaf() )
  {
    // If left neighbor is a leaf, always create an edge
    ids[1] = cursorL->GetGlobalNodeIndex();
    this->Connectivity->InsertNextTypedTuple( ids );
  }

  // Check whether a dual edge to right neighbor exists
  if ( ( cursorR->HasTree() && cursorR->IsLeaf() )
       && cursorR->GetLevel() != cursor->GetLevel() )
  {
    // If right neighbor is a leaf, create an edge only if right cell at higher level
    ids[1] = cursorR->GetGlobalNodeIndex();
    this->Connectivity->InsertNextTypedTuple( ids );
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GenerateDualCornerFromLeaf2D( vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor )
{
  // With d=2:
  //   (d-0)-faces are edges, neighbor cursors are 1, 3, 5, 7
  //   (d-1)-faces are corners, neighbor cursors are 0, 2, 6, 8
  //   (d-2)-faces do not exist

  // Retrieve (d-0)-neighbor (south/east/west/north) cursors
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorS = cursor->GetOrientedGeometryCursor( 1 );
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorW = cursor->GetOrientedGeometryCursor( 3 );
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorE = cursor->GetOrientedGeometryCursor( 5 );
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorN = cursor->GetOrientedGeometryCursor( 7 );

  // Retrieve (d-1)-neighbor (southwest/southeast/northwest/northeast) cursors
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorSW = cursor->GetOrientedGeometryCursor( 0 );
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorSE = cursor->GetOrientedGeometryCursor( 2 );
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorNW = cursor->GetOrientedGeometryCursor( 6 );
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorNE = cursor->GetOrientedGeometryCursor( 8 );

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
  if ( ! cursorS->HasTree() )
  {
    // Move to south edge
    pt[axisSN] -= shift[1];
  }
  if ( ! cursorW->HasTree() )
  {
    // Move to west edge
    pt[axisWE] -= shift[0];
  }
  if ( ! cursorE->HasTree() )
  {
    // Move to east edge
    pt[axisWE] += shift[0];
  }
  if ( ! cursorN->HasTree() )
  {
    // Move to north edge
    pt[axisSN] += shift[1];
  }

  // Retrieve global index of center cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();

  // Insert dual point at center of leaf cell
  assert ( id < this->GetNumberOfPoints() );
  this->Points->SetPoint( id, pt );

  // Storage for edge vertex IDs: dual cell ownership to cursor with higher index
  vtkIdType ids[4];
  ids[0] = id;

  // Retrieve current level to break corner ownership ties
  unsigned int level = cursor->GetLevel();

  // Check whether a dual cell around SW corner exists
  if ( cursorSW->HasTree() && cursorSW->IsLeaf()
       && cursorS->HasTree() && cursorS->IsLeaf()
       && cursorW->HasTree() && cursorW->IsLeaf() )
  {
    // If SW, S, and W neighbors are leaves, always create a face
    ids[1] = cursorW->GetGlobalNodeIndex();
    ids[2] = cursorS->GetGlobalNodeIndex();
    ids[3] = cursorSW->GetGlobalNodeIndex();
    this->Connectivity->InsertNextTypedTuple( ids );
  }

  // Check whether a dual cell around SE corner exists
  if ( cursorS->HasTree() && cursorS->IsLeaf()
       && cursorSE->HasTree() && cursorSE->IsLeaf()
       && cursorE->HasTree() && cursorE->IsLeaf()
       && level != cursorE->GetLevel() )
  {
    // If S, SE, and E neighbors are leaves, create a face if E at higher level
    ids[1] = cursorE->GetGlobalNodeIndex();
    ids[2] = cursorS->GetGlobalNodeIndex();
    ids[3] = cursorSE->GetGlobalNodeIndex();
    this->Connectivity->InsertNextTypedTuple( ids );
  }

  // Check whether a dual cell around NE corner exists
  if ( cursorE->HasTree() && cursorE->IsLeaf()
       && cursorNE->HasTree() && cursorNE->IsLeaf()
       && cursorN->HasTree() && cursorN->IsLeaf()
       && level != cursorE->GetLevel()
       && level != cursorNE->GetLevel()
       && level != cursorN->GetLevel() )
  {
    // If E, NE, and N neighbors are leaves, create a face if E, NE, N at higher level
    ids[1] = cursorE->GetGlobalNodeIndex();
    ids[2] = cursorN->GetGlobalNodeIndex();
    ids[3] = cursorNE->GetGlobalNodeIndex();
    this->Connectivity->InsertNextTypedTuple( ids );
  }

  // Check whether a dual cell around NW corner exists
  if ( cursorW->HasTree() && cursorW->IsLeaf()
       && cursorN->HasTree() && cursorN->IsLeaf()
       && cursorNW->HasTree() && cursorNW->IsLeaf()
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
void vtkHyperTreeGrid::GenerateDualCornerFromLeaf2D( vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor,
                                                     vtkBitArray* mask )
{
  // With d=2:
  //   (d-0)-faces are edges, neighbor cursors are 1, 3, 5, 7
  //   (d-1)-faces are corners, neighbor cursors are 0, 2, 6, 8
  //   (d-2)-faces do not exist

  const unsigned int dS  = 1;
  const unsigned int dW  = 3;
  const unsigned int dE  = 5;
  const unsigned int dN  = 7;
  const unsigned int dSW = 0;
  const unsigned int dSE = 2;
  const unsigned int dNW = 6;
  const unsigned int dNE = 8;

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
  if ( ! cursor->HasTree( dS ) )
  {
    // Move to south edge
    pt[axisSN] -= shift[1];
    shifted = true;
  } else if ( cursor->IsLeaf( dS ) && mask->GetValue( cursor->GetGlobalNodeIndex ( dS ) ) )
  {
    // Move to south edge
    pt[axisSN] -= shift[1];
    shifted = true;
  }

  if ( ! cursor->HasTree( dW ) )
  {
    // Move to west edge
    pt[axisWE] -= shift[0];
    shifted = true;
  } else if ( cursor->IsLeaf( dW ) && mask->GetValue( cursor->GetGlobalNodeIndex ( dW ) ) )
  {
    // Move to west edge
    pt[axisWE] -= shift[0];
    shifted = true;
  }

  if ( ! cursor->HasTree( dE ) )
  {
    // Move to east edge
    pt[axisWE] += shift[0];
    shifted = true;
  } else if ( cursor->IsLeaf( dE ) && mask->GetValue( cursor->GetGlobalNodeIndex ( dE ) ) )
  {
    // Move to east edge
    pt[axisWE] += shift[0];
    shifted = true;
  }

  if ( ! cursor->HasTree( dN ) )
  {
    // Move to north edge
    pt[axisSN] += shift[1];
    shifted = true;
  } else if ( cursor->IsLeaf( dN ) && mask->GetValue( cursor->GetGlobalNodeIndex ( dN ) ) )
  {
    // Move to north edge
    pt[axisSN] += shift[1];
    shifted = true;
  }

  // Only when point was not moved to edge, check corner neighbors
  if ( ! shifted )
  {
    if ( ! cursor->HasTree( dSW ) )
    {
      // Move to southwest corner
      pt[axisWE] -= shift[0];
      pt[axisSN] -= shift[1];
    } else if ( cursor->IsLeaf( dSW ) && mask->GetValue( cursor->GetGlobalNodeIndex ( dSW ) ) )
    {
      // Move to southwest corner
      pt[axisWE] -= shift[0];
      pt[axisSN] -= shift[1];
    }

    if ( ! cursor->HasTree( dSE ) )
    {
      // Move to southeast corner
      pt[axisWE] += shift[0];
      pt[axisSN] -= shift[1];
    } else if ( cursor->IsLeaf( dSE ) && mask->GetValue( cursor->GetGlobalNodeIndex ( dSE ) ) )
    {
      // Move to southeast corner
      pt[axisWE] += shift[0];
      pt[axisSN] -= shift[1];
    }

    if ( ! cursor->HasTree( dNW ) )
    {
      // Move to northwest corner
      pt[axisWE] -= shift[0];
      pt[axisSN] += shift[1];
    } else if ( cursor->IsLeaf( dNW ) && mask->GetValue( cursor->GetGlobalNodeIndex ( dNW ) ) )
    {
      // Move to northwest corner
      pt[axisWE] -= shift[0];
      pt[axisSN] += shift[1];
    }

    if ( ! cursor->HasTree( dNE ) )
    {
      // Move to northeast corner
      pt[axisWE] += shift[0];
      pt[axisSN] += shift[1];
    } else if ( cursor->IsLeaf( dNE ) && mask->GetValue( cursor->GetGlobalNodeIndex ( dNE ) ) )
    {
      // Move to northeast corner
      pt[axisWE] += shift[0];
      pt[axisSN] += shift[1];
    }
  } // if ( ! shifted )


  // Retrieve global index of center cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();

  // Insert dual point at center of leaf cell
  assert ( id < this->GetNumberOfPoints() );
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
  if (    cursor->HasTree( dSW ) && cursor->HasTree( dS ) && cursor->HasTree( dW )
       && cursor->IsLeaf( dSW )  && cursor->IsLeaf( dS )  && cursor->IsLeaf( dW ) )
  {
    vtkIdType idSW, idS, idW;
    if (    ! mask->GetValue( idSW = cursor->GetGlobalNodeIndex ( dSW ) )
         && ! mask->GetValue( idS = cursor->GetGlobalNodeIndex ( dS ) )
         && ! mask->GetValue( idW = cursor->GetGlobalNodeIndex ( dW ) ) )
    {
      // If SW, S, and W neighbors are leaves, always create a face
      ids[1] = idW;
      ids[2] = idS;
      ids[3] = idSW;
      this->Connectivity->InsertNextTypedTuple( ids );
    }
  }

  // Check whether a dual cell around SE corner exists
  if (    cursor->HasTree( dS ) && cursor->HasTree( dSE ) && cursor->HasTree( dE )
       && cursor->IsLeaf( dS )  && cursor->IsLeaf( dSE )  && cursor->IsLeaf( dE ) )
  {
    vtkIdType idS, idSE, idE;
    if (    ! mask->GetValue( idS = cursor->GetGlobalNodeIndex ( dS ) )
         && ! mask->GetValue( idSE = cursor->GetGlobalNodeIndex ( dSE ) )
         && ! mask->GetValue( idE = cursor->GetGlobalNodeIndex ( dE ) )
         && level != cursor->GetLevel( dE ) )
    {
      // If S, SE, and E neighbors are leaves, create a face if E at higher level
      ids[1] = idE;
      ids[2] = idS;
      ids[3] = idSE;
      this->Connectivity->InsertNextTypedTuple( ids );
    }
  }

  // Check whether a dual cell around NE corner exists
  if (    cursor->HasTree( dE ) && cursor->HasTree( dNE ) && cursor->HasTree( dN )
       && cursor->IsLeaf( dE )  && cursor->IsLeaf( dNE )  && cursor->IsLeaf( dN ) )
  {
    vtkIdType idE, idNE, idN;
    if (    ! mask->GetValue( idE = cursor->GetGlobalNodeIndex ( dE ) )
         && ! mask->GetValue( idNE = cursor->GetGlobalNodeIndex ( dNE ) )
         && ! mask->GetValue( idN = cursor->GetGlobalNodeIndex ( dN ) )
         && level != cursor->GetLevel( dE )
         && level != cursor->GetLevel( dNE )
         && level != cursor->GetLevel( dN ) )
    {
      // If E, NE, and N neighbors are leaves, create a face if E, NE, N at higher level
      ids[1] = idE;
      ids[2] = idN;
      ids[3] = idNE;
      this->Connectivity->InsertNextTypedTuple( ids );
    }
  }

  // Check whether a dual cell around NW corner exists
  if (    cursor->HasTree( dW ) && cursor->HasTree( dN ) && cursor->HasTree( dNW )
       && cursor->IsLeaf( dW )  && cursor->IsLeaf( dN )  && cursor->IsLeaf( dNW ) )
  {
    vtkIdType idW, idN, idNW;
    if (    ! mask->GetValue( idW = cursor->GetGlobalNodeIndex ( dW ) )
         && ! mask->GetValue( idN = cursor->GetGlobalNodeIndex ( dN ) )
         && ! mask->GetValue( idNW = cursor->GetGlobalNodeIndex ( dNW ) )
         && level != cursor->GetLevel( dNW )
         && level != cursor->GetLevel( dN ) )
    {
      // If W, N, and NW neighbors are leaves, create a face if NW and N at higher level
      ids[1] = idW;
      ids[2] = idN;
      ids[3] = idNW;
      this->Connectivity->InsertNextTypedTuple( ids );
    }
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GenerateDualCornerFromLeaf3D( vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor )
{
  // With d=3:
  //   (d-0)-faces are faces, neighbor cursors are 4, 10, 12, 14, 16, 22
  //   (d-1)-faces are edges, neighbor cursors are 1, 3, 5, 7, 9, 11, 15, 17, 19, 21, 23, 25
  //   (d-2)-faces are corners, neighbor cursors are 0, 2, 6, 8, 18, 20, 24, 26

  // Retrieve cursors
  std::vector< vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> > cursors;
  cursors.resize( 27 );
  for ( unsigned int c = 0; c < 27; ++ c )
  {
    cursors[c] = cursor->GetOrientedGeometryCursor( c );
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
    if ( ! cursors[13 - offset]->HasTree() )
    {
      // Move to negative side along axis
      pt[axis] -= shift[axis];
    }
    if ( ! cursors[13 + offset]->HasTree() )
    {
      // Move to positive side along axis
      pt[axis] += shift[axis];
    }
  } // axis

  // Retrieve global index of center cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();

  // Insert dual point at center of leaf cell
  assert ( id < this->GetNumberOfPoints() );
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

      // Compute whether corner is owned by another leaf
      if ( index != 13 )
      {
        if ( ! cursors[index]->HasTree() || ! cursors[index]->IsLeaf()
             ||
             ( cursors[index]->GetLevel() == level && index > 13 ) )
        {
          // If neighbor leaf is out of bounds or has not been
          // refined to a leaf, this leaf does not own the corner
          // A level tie is broken in favor of the largest index
          owner = false;
        } else {
          // Collect the leaf indices for the dual cell
          ids[l] = cursors[index]->GetGlobalNodeIndex();
        }
      } else { // if ( index != 13 )
        // Collect the leaf indices for the dual cell
        ids[l] = cursors[index]->GetGlobalNodeIndex();
      } // else
    } // l

    // If leaf owns the corner, create dual cell
    if ( owner )
    {
      this->Connectivity->InsertNextTypedTuple( ids );
    } // if ( owner )
  } // c
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GenerateDualCornerFromLeaf3D( vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor,
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

  // When a mask is present, corner, edge, and face shifts are possible
  bool shifted = false;

  // Check across face neighbors whether point must be adjusted
  unsigned int offset = 1;
  for ( unsigned int axis = 0; axis < 3; ++ axis, offset *= 3 )
  {
    vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorM = cursor->GetOrientedGeometryCursor( 13 - offset );
    if ( ! cursorM->HasTree() )
    {
      // Move to negative side along axis
      pt[axis] -= shift[axis];
      shifted = true;
    } else {
      vtkIdType idM = cursorM->GetGlobalNodeIndex();
      if ( cursorM->IsLeaf() && mask->GetValue( idM ) )
      {
        // Move to negative side along axis
        pt[axis] -= shift[axis];
        shifted = true;
      }
    }
    vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorP = cursor->GetOrientedGeometryCursor( 13 + offset );
    if ( ! cursorP->HasTree() )
    {
      // Move to positive side along axis
      pt[axis] += shift[axis];
      shifted = true;
    } else {
      vtkIdType idP = cursorP->GetGlobalNodeIndex();
      if ( cursorP->IsLeaf() && mask->GetValue( idP ) )
      {
        // Move to positive side along axis
        pt[axis] += shift[axis];
        shifted = true;
      }
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
             vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorE = cursor->GetOrientedGeometryCursor(static_cast<unsigned int>(index) );
             if ( ! cursorE->HasTree() )
             {
               // Move to corresponding edge
               pt[axis1] += o1 * o2 * shift[axis1];
               pt[axis2] += o1 * shift[axis2];
               shifted = true;
             } else {
               vtkIdType idE = cursorE->GetGlobalNodeIndex();
               if ( cursorE->IsLeaf() && mask->GetValue( idE ) )
               {
                 // Move to corresponding edge
                 pt[axis1] += o1 * o2 * shift[axis1];
                 pt[axis2] += o1 * shift[axis2];
                 shifted = true;
               }
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
          vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorC = cursor->GetOrientedGeometryCursor( static_cast<unsigned int>(index) );
          if ( ! cursorC->HasTree() )
          {
            // Move to corresponding corner
            pt[0] += o1 * o2 * o3 * shift[0];
            pt[1] += o1 * o2 * shift[1];
            pt[2] += o1 * shift[2];
          } else {// if cursor
            vtkIdType idC = cursorC->GetGlobalNodeIndex();
            if ( cursorC->IsLeaf() && mask->GetValue( idC ) )
            {
              // Move to corresponding corner
              pt[0] += o1 * o2 * o3 * shift[0];
              pt[1] += o1 * o2 * shift[1];
              pt[2] += o1 * shift[2];
            }
          } // if cursor
        } // o1
      } // o2
    } // o3
  } // if ( ! shifted )

  // Retrieve global index of center cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();

  // Insert dual point at center of leaf cell
  assert ( id < this->GetGlobalNodeIndexMax() + 1 );
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
    unsigned int real_l = 0;

    // Iterate over every leaf touching the corner
    for ( unsigned int l = 0; l < 8 && owner; ++ l )
    {
      // Retrieve cursor index of touching leaf
      unsigned int index = CornerNeighborCursorsTable3D[c][l];

      // Compute whether corner is owned by another leaf
      if ( index != 13 )
      {
        if ( ! cursor->HasTree( index ) || ! cursor->IsLeaf( index )
             ||
             ( cursor->GetLevel( index ) == level && index > 13 ) )
        {
          // If neighbor leaf is out of bounds or has not been
          // refined to a leaf, this leaf does not own the corner
          // A level tie is broken in favor of the largest index
          owner = false;
        } else {
          vtkIdType idglobal = cursor->GetGlobalNodeIndex( index );
          if ( ! mask->GetValue( idglobal ) ) {
            // Collect the leaf indices for the dual cell
            ids[ real_l++ ] = cursor->GetGlobalNodeIndex( index );
          } else {
            owner = false;
          }
        }
      } else { // if ( index != 13 )
        // Collect the leaf indices for the dual cell
        ids[ real_l++ ] = id;
      } // else
    } // l

    // If leaf owns the corner, create dual cell
    if ( owner )
    {
      if ( real_l != 8 )
      {
        if ( real_l == 0 )
        {
          continue;
        }
        vtkIdType last = ids[ real_l - 1 ];
        for ( ; real_l < 8; ++ real_l )
        {
          ids[ real_l ] = last;
        }
      }
      this->Connectivity->InsertNextTypedTuple( ids );
    } // if ( owner )
  } // c
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::ShiftDualCornerFromMaskedLeaf2D( vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor,
                                                        vtkBitArray* mask )
{
  // With d=2:
  //   (d-0)-faces are edges, neighbor cursors are 1, 3, 5, 7
  //   (d-1)-faces are corners, neighbor cursors are 0, 2, 6, 8
  //   (d-2)-faces do not exist

  // Retrieve (d-0)-neighbor (south/east/west/north) cursors
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorS = cursor->GetOrientedGeometryCursor( 1 );
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorW = cursor->GetOrientedGeometryCursor( 3 );
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorE = cursor->GetOrientedGeometryCursor( 5 );
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorN = cursor->GetOrientedGeometryCursor( 7 );

  // Retrieve (d-1)-neighbor (southwest/southeast/northwest/northeast) cursors
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorSW = cursor->GetOrientedGeometryCursor( 0 );
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorSE = cursor->GetOrientedGeometryCursor( 2 );
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorNW = cursor->GetOrientedGeometryCursor( 6 );
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorNE = cursor->GetOrientedGeometryCursor( 8 );

  // Retrieve global indices of non-center cursors

  // Retrieve 2D axes (east-west/south-north)
  unsigned int axisWE = this->Orientation ? 0 : 1;
  unsigned int axisSN = this->Orientation == 2 ? 1 : 2;

  // Retrieve current level to break corner ownership ties
  unsigned int level = cursor->GetLevel();

  // Check whether dual point across S edge must be adjusted
  if ( cursorS->HasTree() && cursorS->IsLeaf() && cursorS->GetLevel() < level )
  {
    vtkIdType idS = cursorS->GetGlobalNodeIndex();
    if (! mask->GetValue( idS ) )
    {
      // Dual point must be adusted
      this->PointShifted[idS] = true;
      this->PointShifts[axisSN][idS] = cursorS->GetTree()->GetScale( axisSN )
        * this->ReductionFactors[cursorS->GetLevel()];
    }
  }

  // Check whether dual point across W edge must be adjusted
  if ( cursorW->HasTree() && cursorW->IsLeaf() && cursorW->GetLevel() < level )
  {
    vtkIdType idW = cursorW->GetGlobalNodeIndex();
    if ( ! mask->GetValue( idW ) )
    {
      // Dual point must be adusted
      this->PointShifted[idW] = true;
      this->PointShifts[axisWE][idW] = cursorW->GetTree()->GetScale( axisWE )
        * this->ReductionFactors[cursorW->GetLevel()];
    }
  }

  // Check whether dual point across E face must be adjusted
  if ( cursorE->HasTree() && cursorE->IsLeaf() && cursorE->GetLevel() < level )
  {
    vtkIdType idE = cursorE->GetGlobalNodeIndex();
    if ( ! mask->GetValue( idE ) )
    {
      // Dual point must be adusted
      this->PointShifted[idE] = true;
      this->PointShifts[axisWE][idE] = - cursorE->GetTree()->GetScale( axisWE )
        * this->ReductionFactors[cursorE->GetLevel()];
    }
  }

  // Check whether dual point across N edge must be adjusted
  if ( cursorN->HasTree() && cursorN->IsLeaf() && cursorN->GetLevel() < level )
  {
    vtkIdType idN = cursorN->GetGlobalNodeIndex();
    if ( ! mask->GetValue( idN ) )
    {
      // Dual point must be adusted
      this->PointShifted[idN] = true;
      this->PointShifts[axisSN][idN] = - cursorN->GetTree()->GetScale( axisSN )
        * this->ReductionFactors[cursorN->GetLevel()];
    }
  }

  // Check whether dual point across SE corner must be adjusted
  if ( cursorSE->HasTree() && cursorSE->IsLeaf() && cursorSE->GetLevel() < level)
  {
    vtkIdType idSE = cursorSE->GetGlobalNodeIndex();
    if ( ! mask->GetValue( idSE ) && ! this->PointShifted[idSE] )
    {
      // Dual point must be adusted
      double shift[3];
      cursorSE->GetTree()->GetScale( shift );
      double factor = this->ReductionFactors[cursorSE->GetLevel()];
      this->PointShifts[axisWE][idSE] = factor * shift[axisWE];
      this->PointShifts[axisSN][idSE] = factor * shift[axisSN];
    }
  }

  // Check whether dual point across SW corner must be adjusted
  if ( cursorSW->HasTree() && cursorSW->IsLeaf() && cursorSW->GetLevel() < level )
  {
    vtkIdType idSW = cursorSW->GetGlobalNodeIndex();
    if ( ! mask->GetValue( idSW ) && ! this->PointShifted[idSW] )
    {
      // Dual point must be adusted
      double shift[3];
      cursorSW->GetTree()->GetScale( shift );
      double factor = this->ReductionFactors[cursorSW->GetLevel()];
      this->PointShifts[axisWE][idSW] = - factor * shift[axisWE];
      this->PointShifts[axisSN][idSW] = factor * shift[axisSN];
    }
  }

  // Check whether dual point across NW corner must be adjusted
  if ( cursorNW->HasTree() && cursorNW->IsLeaf() && cursorNW->GetLevel() < level )
  {
    vtkIdType idNW = cursorNW->GetGlobalNodeIndex();
    if( ! mask->GetValue( idNW ) && ! this->PointShifted[idNW] )
    {
      // Dual point must be adusted
      double shift[3];
      cursorNW->GetTree()->GetScale( shift );
      double factor = this->ReductionFactors[cursorNW->GetLevel()];
      this->PointShifts[axisWE][idNW] = factor * shift[axisWE];
      this->PointShifts[axisSN][idNW] = - factor * shift[axisSN];
    }
  }

  // Check whether dual point across NE corner must be adjusted
  if ( cursorNE->HasTree() && cursorNE->IsLeaf() && cursorNE->GetLevel() < level )
  {
    vtkIdType idNE = cursorNE->GetGlobalNodeIndex();
    if( ! mask->GetValue( idNE ) && ! this->PointShifted[idNE] )
    {
      // Dual point must be adusted
      double shift[3];
      cursorNE->GetTree()->GetScale( shift );
      double factor = this->ReductionFactors[cursorNE->GetLevel()];
      this->PointShifts[axisWE][idNE] = - factor * shift[axisWE];
      this->PointShifts[axisSN][idNE] = - factor * shift[axisSN];
    }
  }

}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::ShiftDualCornerFromMaskedLeaf3D( vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor,
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
    vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorM = cursor->GetOrientedGeometryCursor( 13 - offset );
    if ( cursorM->HasTree() && cursorM->IsLeaf() && cursorM->GetLevel() < level )
    {
      vtkIdType idM = cursorM->GetGlobalNodeIndex();
      if ( ! mask->GetValue( idM ) )
      {
        // Dual point must be adjusted
        this->PointShifted[idM] = true;
        this->PointShifts[axis][idM] = cursorM->GetTree()->GetScale( axis )
          * this->ReductionFactors[cursorM->GetLevel()];
      }
    }
    vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorP = cursor->GetOrientedGeometryCursor( 13 + offset );
    if ( cursorP->HasTree() && cursorP->IsLeaf() && cursorP->GetLevel() < level )
    {
      vtkIdType idP = cursorP->GetGlobalNodeIndex();
      if ( ! mask->GetValue( idP ) )
      {
        // Dual point must be adjusted
        this->PointShifted[idP] = true;
        this->PointShifts[axis][idP] = - cursorP->GetTree()->GetScale( axis )
          * this->ReductionFactors[cursorP->GetLevel()];
      }
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
          vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorE = cursor->GetOrientedGeometryCursor( static_cast<unsigned int>(index) );
          if ( cursorE->HasTree() && cursorE->IsLeaf() && cursorE->GetLevel() < level )
          {
            vtkIdType idE = cursorE->GetGlobalNodeIndex();
            if ( ! mask->GetValue( idE ) && ! this->PointShifted[idE] )
            {
              // Dual point must be adusted
              this->PointShifted[idE] = true;
              double shift[3];
              cursorE->GetTree()->GetScale( shift );
              double factor = this->ReductionFactors[cursorE->GetLevel()];
              this->PointShifts[axis1][idE] = - o1 * o2 * factor * shift[axis1];
              this->PointShifts[axis2][idE] = - o1 * factor * shift[axis2];
            }
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
        vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorC = cursor->GetOrientedGeometryCursor( index );
        if ( cursorC->HasTree() && cursorC->IsLeaf() && cursorC->GetLevel() < level )
        {
          vtkIdType idC = cursorC->GetGlobalNodeIndex();
          if ( ! mask->GetValue( idC ) && ! this->PointShifted[idC] )
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
void vtkHyperTreeGrid::vtkHyperTreeGridIterator::Initialize( vtkHyperTreeGrid* grid )
{
  this->Grid = grid;
  this->Iterator = grid->HyperTrees.begin();
}

//JB  return this->MaterialMaskIndex ?
//JB    this->MaterialMaskIndex->GetNumberOfTuples() :
//JB    this->GridSize[0] * this->GridSize[1] * this->GridSize[2];

//-----------------------------------------------------------------------------
vtkHyperTree* vtkHyperTreeGrid::vtkHyperTreeGridIterator::GetNextTree( vtkIdType &index )
{
  if ( this->Iterator == this->Grid->HyperTrees.end() )
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
void vtkHyperTreeGrid::SetOrientation( unsigned char o )
{
  unsigned char orientation = o > 2 ? 2 : o;
  if ( this->Orientation != ( orientation ) )
  {
    this->Orientation = orientation;
    this->Modified();
  }
}
//=============================================================================
