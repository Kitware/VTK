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
#include "vtkCollection.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkGenericCell.h"
#include "vtkHyperTree.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStructuredData.h"

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

void vtkHyperTreeGrid::SetMask ( vtkBitArray* _arg)
{
  vtkSetObjectBodyMacro( Mask, vtkBitArray, _arg );

  this->InitPureMask = false;
  if(this->PureMask)
  {
    this->PureMask->Delete();
    this->PureMask = nullptr;
  }
}

// Helper macros to quickly fetch a HT at a given index or iterator
#define GetHyperTreeFromOtherMacro( _obj_, _index_ ) \
  ( static_cast<vtkHyperTree*>( _obj_->HyperTrees.find( _index_ ) \
                                != _obj_->HyperTrees.end() ? \
                                _obj_->HyperTrees[ _index_ ] : 0 ) )
#define GetHyperTreeFromThisMacro( _index_ ) GetHyperTreeFromOtherMacro( this, _index_ )

//-----------------------------------------------------------------------------
vtkHyperTreeGrid::vtkHyperTreeGrid()
{
  // Default state
  this->FrozenState = false;

  // Grid topology
  this->GridSize[0] = 0;
  this->GridSize[1] = 0;
  this->GridSize[2] = 0;
  // -----------------------------------------------
  // RectilinearGrid
  // -----------------------------------------------
  this->Dimensions[0] = 0;
  this->Dimensions[1] = 0;
  this->Dimensions[2] = 0;

  int extent[6] = {0, -1, 0, -1, 0, -1};
  memcpy(this->Extent, extent, 6*sizeof(int));
  this->DataDescription = VTK_EMPTY;

  this->Information->Set( vtkDataObject::DATA_EXTENT_TYPE(), VTK_3D_EXTENT ) ;
  this->Information->Set( vtkDataObject::DATA_EXTENT(), this->Extent, 6 );

  // Primal grid geometry
  this->XCoordinates=vtkDoubleArray::New();
  this->XCoordinates->SetNumberOfTuples(1);
  this->XCoordinates->SetComponent(0, 0, 0.0);

  this->YCoordinates=vtkDoubleArray::New();
  this->YCoordinates->SetNumberOfTuples(1);
  this->YCoordinates->SetComponent(0, 0, 0.0);

  this->ZCoordinates=vtkDoubleArray::New();
  this->ZCoordinates->SetNumberOfTuples(1);
  this->ZCoordinates->SetComponent(0, 0, 0.0);
  // -----------------------------------------------

  this->TransposedRootIndexing = false;

  // Invalid default grid parameters to force actual initialization
  this->Dimension = 0;
  this->Orientation = UINT_MAX;
  this->BranchFactor = 0;
  this->NumberOfChildren = 0;

  // Masked primal leaves
  this->Mask = vtkBitArray::New();
  this->PureMask = nullptr;
  this->InitPureMask = false;

  // No interface by default
  this->HasInterface = false;

  // Interface array names
  this->InterfaceNormalsName = nullptr;
  this->InterfaceInterceptsName = nullptr;

  this->Bounds[0] = 0.0;
  this->Bounds[1] = -1.0;
  this->Bounds[2] = 0.0;
  this->Bounds[3] = -1.0;
  this->Bounds[4] = 0.0;
  this->Bounds[5] = -1.0;
  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;
  this->PointData = vtkPointData::New();
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
  if ( this->Mask )
  {
    this->Mask->Delete();
    this->Mask = nullptr;
  }

  if( this->PureMask )
  {
    this->PureMask->Delete();
    this->PureMask = nullptr;
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

  this->DeleteTrees();
  this->PointData->Delete();
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
  os << indent << "Dimensions: "
     << this->Dimensions[0] <<","
     << this->Dimensions[1] <<","
     << this->Dimensions[2] << endl;
  os << indent << "Mask:\n";
  if ( this->Mask )
  {
    this->Mask->PrintSelf( os, indent.GetNextIndent() );
  }
  if ( this->PureMask )
  {
    this->PureMask->PrintSelf( os, indent.GetNextIndent() );
  }
  os << indent << "InitPureMask: "
     << ( this->InitPureMask ? "true" : "false" ) << endl;

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

  os << indent << "PointData:\n";
  this->PointData->PrintSelf( os, indent.GetNextIndent() );
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
void vtkHyperTreeGrid::CopyStructure( vtkDataObject* ds )
{
  assert( "pre: ds_exists" && ds != nullptr );
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast( ds );
  assert( "pre: same_type" && htg != nullptr );

  // RectilinearGrid
  for (int i=0; i<3; i++)
  {
    this->Dimensions[i] = htg->Dimensions[i];
  }
  this->SetExtent(htg->GetExtent());
  this->DataDescription = htg->DataDescription;

  this->SetXCoordinates(htg->XCoordinates);
  this->SetYCoordinates(htg->YCoordinates);
  this->SetZCoordinates(htg->ZCoordinates);

  // Copy grid parameters
  this->FrozenState = htg->FrozenState;
  this->BranchFactor = htg->BranchFactor;
  this->Dimension = htg->Dimension;
  this->Orientation = htg->Orientation;
  memcpy( this->GridSize, htg->GetGridSize(), 3 * sizeof( int ) );
  this->NumberOfChildren = htg->NumberOfChildren;
  this->TransposedRootIndexing = htg->TransposedRootIndexing;
  this->InitPureMask = htg->InitPureMask;
  this->HasInterface = htg->HasInterface;
  this->SetInterfaceNormalsName(htg->InterfaceNormalsName);
  this->SetInterfaceInterceptsName(htg->InterfaceInterceptsName);

  // Shallow copy masked if needed
  this->SetMask(htg->GetMask());
  vtkSetObjectBodyMacro(PureMask, vtkBitArray, htg->GetPureMask());

  this->PointData->CopyStructure(htg->GetPointData());

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

//-----------------------------------------------------------------------------

void vtkHyperTreeGrid::GetNumberOfTreesPerDimension(unsigned int* dimOut)
{
  dimOut[0] = this->GridSize[0];
  dimOut[1] = this->GridSize[1];
  dimOut[2] = this->GridSize[2];
}

//-----------------------------------------------------------------------------
// TO BE REMOVED
void vtkHyperTreeGrid::SetGridSize( unsigned int dim[3] )
{
  this->GridSize[0] = dim[0];
  this->GridSize[1] = dim[1];
  this->GridSize[2] = dim[2];

  // Caution the extent is the range of point index which include the range value.
  // Here the extent has a different meaning
  // If we don't set the extent that way TestHyperTreeGridTernary2DMaterialBits will fail
  this->SetExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);
}
void vtkHyperTreeGrid::SetGridSize( unsigned int i, unsigned int j, unsigned int k )
{
  this->GridSize[0] = i;
  this->GridSize[1] = j;
  this->GridSize[2] = k;

  // Caution the extent is the range of point index which include the range value.
  // Here the extent has a different meaning
  // If we don't set the extent that way TestHyperTreeGridTernary2DMaterialBits will fail
  this->SetExtent(0, i - 1, 0, j - 1, 0, k - 1);
}
// TO BE REMOVED
//-----------------------------------------------------------------------------

// ============================================================================
// BEGIN - RectilinearGrid common API
// ============================================================================

//----------------------------------------------------------------------------
// Set dimensions of rectilinear grid dataset.
void vtkHyperTreeGrid::SetDimensions(int i, int j, int k)
{
  this->SetExtent(0, i-1, 0, j-1, 0, k-1);

  // Compute the number of tree in each direction
  // FIXME: this->GridSize[0] = (i - 1 == 0) ? 1 : (i - 1);
  // FIXME: this->GridSize[1] = (j - 1 == 0) ? 1 : (j - 1);
  // FIXME: this->GridSize[2] = (k - 1 == 0) ? 1 : (k - 1);
}

//----------------------------------------------------------------------------
// Set dimensions of rectilinear grid dataset.
void vtkHyperTreeGrid::SetDimensions(const int dim[3])
{
  this->SetExtent(0, dim[0]-1, 0, dim[1]-1, 0, dim[2]-1);

  // Compute the number of tree in each direction
  // FIXME: for (int i = 0; i < 3; i++)
  // FIXME: {
  // FIXME:   this->GridSize[i] = (dim[i] - 1) == 0 ? 1 : (dim[i] - 1);
  // FIXME: }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGrid::SetExtent(int extent[6])
{
  int description;

  description = vtkStructuredData::SetExtent(extent, this->Extent);
  if ( description < 0 ) //improperly specified
  {
    vtkErrorMacro (<< "Bad Extent, retaining previous values");
  }

  if (description == VTK_UNCHANGED)
  {
    return;
  }

  this->DataDescription = description;

  this->Modified();

  // Only update the Dimensions when set with [0, ..., 0, ..., 0, ...] extent
  if (extent[0] == 0 && extent[2] == 0 && extent[4] == 0)
  {
    this->Dimensions[0] = extent[1] - extent[0] + 1;
    this->Dimensions[1] = extent[3] - extent[2] + 1;
    this->Dimensions[2] = extent[5] - extent[4] + 1;
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGrid::SetExtent(int xMin, int xMax, int yMin, int yMax,
                                   int zMin, int zMax)
{
  int extent[6];

  extent[0] = xMin; extent[1] = xMax;
  extent[2] = yMin; extent[3] = yMax;
  extent[4] = zMin; extent[5] = zMax;

  this->SetExtent(extent);
}

// ============================================================================
// END - RectilinearGrid common API
// ============================================================================

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
bool vtkHyperTreeGrid::HasMask()
{
  return ( this->Mask->GetNumberOfTuples() != 0 );
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

  // vtkIdType nbTrees = 1;
  // for (int i = 0; i < 3; i++)
  // {
  //   int dim = this->Extent[2 * i + 1] - this->Extent[2 * i];
  //   if (dim > 1)
  //   {
  //     nbTrees *= dim;
  //   }
  // }
  // return nbTrees;
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
  this->Superclass::Initialize();
  // DataObject Initialize will not do PointData
  this->PointData->Initialize();
  // Delete existing trees
  this->DeleteTrees();
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
void vtkHyperTreeGrid::ShallowCopy( vtkDataObject* src )
{
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast( src );
  assert( "src_same_type" && htg );

  // Copy member variables
  this->CopyStructure( htg );

  this->PointData->ShallowCopy(htg->GetPointData());

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

  if (htg->Mask)
  {
    vtkNew<vtkBitArray> mask;
    this->SetMask(mask);
    this->Mask->DeepCopy( htg->Mask );
  }

  if( htg->PureMask )
  {
    if (!this->PureMask)
    {
      this->PureMask = vtkBitArray::New();
    }
    this->PureMask->DeepCopy( htg->PureMask );
  }

  this->PointData->DeepCopy(htg->GetPointData());

  // Rectilinear part
  vtkDoubleArray *s;
  this->SetDimensions(htg->GetDimensions());
  memcpy(this->Extent, htg->GetExtent(), 6*sizeof(int));
  this->DataDescription = htg->DataDescription;

  s = vtkDoubleArray::New();
  s->DeepCopy(htg->GetXCoordinates());
  this->SetXCoordinates(s);
  s->Delete();
  s = vtkDoubleArray::New();
  s->DeepCopy(htg->GetYCoordinates());
  this->SetYCoordinates(s);
  s->Delete();
  s = vtkDoubleArray::New();
  s->DeepCopy(htg->GetZCoordinates());
  this->SetZCoordinates(s);
  s->Delete();

  // Call superclass
  this->Superclass::DeepCopy( src );
}

//----------------------------------------------------------------------------
bool vtkHyperTreeGrid::RecursivelyInitializePureMask( vtkHyperTreeGridNonOrientedCursor* cursor, vtkDataArray* normale )
{
  // Retrieve mask value at cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();
  bool mask = this->HasMask() && this->Mask->GetValue( id  );

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
      pure |= this->RecursivelyInitializePureMask( cursor, normale );
      cursor->ToParent( );
    }
    // Set and return pure material mask with recursively computed value
    this->PureMask->SetTuple1( id, pure );
    return pure;
  }

  // Set and return pure material mask with recursively computed value
  this->PureMask->SetTuple1( id, mask );
  return mask;
}

//----------------------------------------------------------------------------
vtkBitArray* vtkHyperTreeGrid::GetPureMask()
{
  // Check whether a pure material mask was initialized
  if( ! this->InitPureMask )
  {
    if ( ! this->Mask->GetNumberOfTuples() )
    {
      // Keep track of the fact that a pure material mask now exists
      this->InitPureMask = true;
      return nullptr;
    }
    // If not, then create one
    if ( this->PureMask == nullptr )
    {
      this->PureMask = vtkBitArray::New();
    }
    this->PureMask->SetNumberOfTuples( this->Mask->GetNumberOfTuples() );

    // Iterate over hyper tree grid
    vtkIdType index;
    vtkHyperTreeGridIterator it;
    it.Initialize( this );

    vtkDataArray *normale = nullptr;
    if ( this->HasInterface )
    {
      // Interface defined
      normale = this->GetFieldData()->GetArray( this->InterfaceNormalsName );
    }

    vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
    while ( it.GetNextTree( index ) )
    {
      // Create cursor instance over current hyper tree
      this->InitializeNonOrientedCursor( cursor, index );
      // Recursively initialize pure material mask
      this->RecursivelyInitializePureMask( cursor, normale );
    }

    // Keep track of the fact that a pure material mask now exists
    this->InitPureMask = true;
  }

  // Return existing or created pure material mask
  return this->PureMask;
}

//----------------------------------------------------------------------------
unsigned long vtkHyperTreeGrid::GetActualMemorySizeBytes()
{
  size_t size = 0; // in bytes

  size += this->vtkDataObject::GetActualMemorySize() << 10;

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

  if ( this->Mask )
  {
    size += this->Mask->GetActualMemorySize() << 10;
  }

  size += this->PointData->GetActualMemorySize() << 10;

  return static_cast<unsigned long>(size);
}

//----------------------------------------------------------------------------
unsigned long vtkHyperTreeGrid::GetActualMemorySize()
{
  // in kilibytes
  return ( this->GetActualMemorySizeBytes() >> 10 );
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

//=============================================================================
// Hyper tree grid iterator
// Implemented here because it needs access to the internal classes.
//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::vtkHyperTreeGridIterator::Initialize( vtkHyperTreeGrid* grid )
{
  this->Grid = grid;
  this->Iterator = grid->HyperTrees.begin();
}

//JB  return this->MaskIndex ?
//JB    this->MaskIndex->GetNumberOfTuples() :
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

//-----------------------------------------------------------------------------
double *vtkHyperTreeGrid::GetBounds( )
{
  this->ComputeBounds();
  return this->Bounds;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetBounds( double *obds )
{
  double *bds = this->GetBounds();
  memcpy(obds, bds, 6*sizeof(double));
}

//-----------------------------------------------------------------------------
double *vtkHyperTreeGrid::GetCenter( )
{
  double *bds = this->GetBounds();
  this->Center[0] = bds[0] + (bds[1]-bds[0])/2.0;
  this->Center[1] = bds[2] + (bds[3]-bds[2])/2.0;
  this->Center[2] = bds[4] + (bds[5]-bds[4])/2.0;
  return this->Center;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGrid::GetCenter( double *octr )
{
  double *ctr = this->GetCenter();
  memcpy(octr, ctr, 3*sizeof(double));
}
