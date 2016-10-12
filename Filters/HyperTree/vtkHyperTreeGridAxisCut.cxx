/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridAxisCut.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridAxisCut.h"

#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkHyperTreeGridAxisCut);

//-----------------------------------------------------------------------------
vtkHyperTreeGridAxisCut::vtkHyperTreeGridAxisCut()
{
  this->PlaneNormalAxis = 0;
  this->PlanePosition = 0.;

  this->Input = 0;
  this->Output = 0;

  this->InData = 0;
  this->OutData = 0;

  this->Points = 0;
  this->Cells = 0;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridAxisCut::~vtkHyperTreeGridAxisCut()
{
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAxisCut::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  if( this->Input )
  {
    os << indent << "Input:\n";
    this->Input->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "Input: ( none )\n";
  }

  if( this->Output )
  {
    os << indent << "Output:\n";
    this->Output->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "Output: ( none )\n";
  }

  os << indent << "Plane Normal Axis : " << this->PlaneNormalAxis << endl;
  os << indent << "Plane Position : " << this->PlanePosition << endl;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridAxisCut::FillInputPortInformation( int, vtkInformation *info )
{
  info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid" );
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridAxisCut::RequestData( vtkInformation*,
                                          vtkInformationVector** inputVector,
                                          vtkInformationVector* outputVector )
{
  // Get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject( 0 );
  vtkInformation *outInfo = outputVector->GetInformationObject( 0 );

  // Retrieve input and output
  this->Input =
    vtkHyperTreeGrid::SafeDownCast( inInfo->Get( vtkDataObject::DATA_OBJECT() ) );
  this->Output =
    vtkPolyData::SafeDownCast( outInfo->Get( vtkDataObject::DATA_OBJECT() ) );

  // This filter is only for 2D slices of 3D grids
  if ( this->Input->GetDimension() != 3 )
  {
    vtkErrorMacro( "Axis cut only works with 3D trees." );
    return 0;
  }

  // Initialize output cell data
  this->InData =
    static_cast<vtkDataSetAttributes*>( this->Input->GetPointData() );
  this->OutData =
    static_cast<vtkDataSetAttributes*>( this->Output->GetCellData() );
  this->OutData->CopyAllocate( this->InData );

  // Cut through hyper tree grid
  this->ProcessTrees();

  // Clean up
  this->Input = 0;
  this->Output = 0;
  this->InData = 0;
  this->OutData = 0;

  this->UpdateProgress ( 1. );

  return 1;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridAxisCut::ProcessTrees()
{
  // TODO: MTime on generation of this table.
  this->Input->GenerateSuperCursorTraversalTable();

  // Primal corner points
  this->Points = vtkPoints::New();
  this->Cells = vtkCellArray::New();

  // Iterate over all hyper trees
  vtkIdType index;
  vtkHyperTreeGrid::vtkHyperTreeIterator it;
  this->Input->InitializeTreeIterator( it );
  while ( it.GetNextTree( index ) )
  {
    // Storage for super cursors
    vtkHyperTreeGrid::vtkHyperTreeGridSuperCursor superCursor;

    // Initialize center cursor
    this->Input->InitializeSuperCursor( &superCursor, index );

    // Traverse and populate dual recursively
    this->RecursiveProcessTree( &superCursor );
  } // it

  // Set output geometry and topology
  this->Output->SetPoints( this->Points );
  this->Output->SetPolys( this->Cells );

  this->Points->UnRegister( this );
  this->Points = 0;
  this->Cells->UnRegister( this );
  this->Cells = 0;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAxisCut::AddFace( vtkIdType inId, double* origin,
                                       double* size, double offset0,
                                       int axis0, int axis1, int axis2 )
{
  // Generate 4 points
  double pt[3];
  memcpy( pt, origin, 3 * sizeof(double) );
  pt[axis0] += size[axis0] * offset0;

  // Storage for cell IDs
  vtkIdType ids[4];
  ids[0] = this->Points->InsertNextPoint( pt );
  pt[axis1] += size[axis1];
  ids[1] = this->Points->InsertNextPoint( pt );
  pt[axis2] += size[axis2];
  ids[2] = this->Points->InsertNextPoint( pt );
  pt[axis1] = origin[axis1];
  ids[3] = this->Points->InsertNextPoint( pt );

  vtkIdType outId = this->Cells->InsertNextCell( 4, ids );
  this->OutData->CopyData( this->InData, inId, outId );
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAxisCut::RecursiveProcessTree( void* sc )
{
  vtkHyperTreeGrid::vtkHyperTreeGridSuperCursor* superCursor =
    static_cast<vtkHyperTreeGrid::vtkHyperTreeGridSuperCursor*>( sc );

  // Get cursor at super cursor center
  vtkHyperTreeGrid::vtkHyperTreeSimpleCursor* cursor0 = superCursor->GetCursor( 0 );

  if ( cursor0->IsLeaf() )
  {
    // Cursor is a leaf
    ProcessLeaf3D( sc );
  }
  else
  {
    // If cursor is not at leaf, recurse to all children
    int numChildren = this->Input->GetNumberOfChildren();
    for ( int child = 0; child < numChildren; ++ child )
    {
      vtkHyperTreeGrid::vtkHyperTreeGridSuperCursor newSuperCursor;
      this->Input->InitializeSuperCursorChild( superCursor,&newSuperCursor, child );
      this->RecursiveProcessTree( &newSuperCursor );
    }
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAxisCut::ProcessLeaf3D( void* sc )
{
  // Get cursor at super cursor center
  vtkHyperTreeGrid::vtkHyperTreeGridSuperCursor* superCursor =
    static_cast<vtkHyperTreeGrid::vtkHyperTreeGridSuperCursor*>( sc );
  vtkHyperTreeGrid::vtkHyperTreeSimpleCursor* cursor0 = superCursor->GetCursor( 0 );

  // Cursor is a leaf, retrieve its global index
  vtkIdType inId = cursor0->GetGlobalNodeIndex();

  // If leaf is masked, skip it
  if ( this->Input->GetMaterialMask()->GetValue( inId ) )
  {
    return;
  }

  // Terminate if the node does not touch the plane.
  if ( superCursor->Origin[this->PlaneNormalAxis] > this->PlanePosition ||
    ( superCursor->Origin[this->PlaneNormalAxis] +
    superCursor->Size[this->PlaneNormalAxis] < this->PlanePosition ) )
  {
    return;
  }

  // Create rectangles at plane/grid intersection
  double k = ( this->PlanePosition - superCursor->Origin[this->PlaneNormalAxis] ) /
    superCursor->Size[this->PlaneNormalAxis];
  int axis1, axis2;
  switch ( this->PlaneNormalAxis )
  {
    case 0:
      axis1 = 1;
      axis2 = 2;
      break;
    case 1:
      axis1 = 0;
      axis2 = 2;
      break;
    case 2:
      axis1 = 0;
      axis2 = 1;
      break;
    default:
      vtkErrorMacro( "Bad Axis." );
      return;
  }

  this->AddFace( inId, superCursor->Origin, superCursor->Size, k,
    this->PlaneNormalAxis, axis1, axis2 );
}
