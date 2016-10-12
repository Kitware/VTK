/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridGeometry.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridGeometry.h"

#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkExtentTranslator.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkHyperTreeGridGeometry);

//-----------------------------------------------------------------------------
vtkHyperTreeGridGeometry::vtkHyperTreeGridGeometry()
{
  this->Input = 0;
  this->Output = 0;

  this->InData = 0;
  this->OutData = 0;

  this->Points = 0;
  this->Cells = 0;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridGeometry::~vtkHyperTreeGridGeometry()
{
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridGeometry::FillInputPortInformation( int, vtkInformation *info )
{
  info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid" );
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridGeometry::RequestData( vtkInformation*,
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

  // Initialize output cell data
  this->InData =
    static_cast<vtkDataSetAttributes*>( this->Input->GetPointData() );
  this->OutData =
    static_cast<vtkDataSetAttributes*>( this->Output->GetCellData() );
  this->OutData->CopyAllocate( this->InData );

  // Extract geometry from hyper tree grid
  this->ProcessTrees();

  // Clean up
  this->Input = 0;
  this->Output = 0;
  this->InData = 0;
  this->OutData = 0;

  this->UpdateProgress( 1. );

  return 1;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::ProcessTrees()
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
  if ( this->Input->GetDimension() == 1  )
  {
    this->Output->SetLines( this->Cells );
  }
  else
  {
    this->Output->SetPolys( this->Cells );
  }

  this->Points->UnRegister( this );
  this->Points = 0;
  this->Cells->UnRegister( this );
  this->Cells = 0;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::RecursiveProcessTree( void* sc )
{
  // Get cursor at super cursor center
  vtkHyperTreeGrid::vtkHyperTreeGridSuperCursor* superCursor =
    static_cast<vtkHyperTreeGrid::vtkHyperTreeGridSuperCursor*>( sc );
  vtkHyperTreeGrid::vtkHyperTreeSimpleCursor* cursor0 = superCursor->GetCursor( 0 );
  if ( cursor0->IsLeaf() )
  {
    switch ( this->Input->GetDimension() )
    {
      case 1:
        ProcessLeaf1D( sc );
        break;
      case 2:
        ProcessLeaf2D( sc );
        break;
      case 3:
        ProcessLeaf3D( sc );
        break;
    }
  }
  else
  {
    // If cursor 0 is not at leaf, recurse to all children
    int numChildren = this->Input->GetNumberOfChildren();
    for ( int child = 0; child < numChildren; ++ child )
    {
      vtkHyperTreeGrid::vtkHyperTreeGridSuperCursor newSuperCursor;
      this->Input->InitializeSuperCursorChild( superCursor, &newSuperCursor, child );
      this->RecursiveProcessTree( &newSuperCursor );
    }
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::ProcessLeaf1D( void* sc )
{
  vtkHyperTreeGrid::vtkHyperTreeGridSuperCursor* superCursor =
    static_cast<vtkHyperTreeGrid::vtkHyperTreeGridSuperCursor*>( sc );

  // In 1D the geometry is composed of edges
  vtkIdType ids[2];
  ids[0] = this->Points->InsertNextPoint( superCursor->Origin );
  double pt[3];
  pt[0] = superCursor->Origin[0] + superCursor->Size[0];
  pt[1] = superCursor->Origin[1];
  pt[2] = superCursor->Origin[2];
  ids[1] = this->Points->InsertNextPoint( pt );
  this->Cells->InsertNextCell( 2, ids );
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::ProcessLeaf2D( void* sc )
{
  // Get cursor at super cursor center
  vtkHyperTreeGrid::vtkHyperTreeGridSuperCursor* superCursor =
    static_cast<vtkHyperTreeGrid::vtkHyperTreeGridSuperCursor*>( sc );
  vtkHyperTreeGrid::vtkHyperTreeSimpleCursor* cursor0 = superCursor->GetCursor( 0 );

  // Cell at cursor 0 is a leaf, retrieve its global index
  vtkIdType id0 = cursor0->GetGlobalNodeIndex();
  // In 2D all unmasked faces are generated
  if ( id0 >= 0 && ! this->Input->GetMaterialMask()->GetValue( id0 ) )
  {
    this->AddFace( id0, superCursor->Origin, superCursor->Size, 0, 2 );
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::ProcessLeaf3D( void* sc )
{
  // Get cursor at super cursor center
  vtkHyperTreeGrid::vtkHyperTreeGridSuperCursor* superCursor =
    static_cast<vtkHyperTreeGrid::vtkHyperTreeGridSuperCursor*>( sc );
  vtkHyperTreeGrid::vtkHyperTreeSimpleCursor* cursor0 = superCursor->GetCursor( 0 );

  vtkBitArray* matMask = this->Input->GetMaterialMask();

  // Cell at cursor 0 is a leaf, retrieve its global index
  vtkIdType id0 = cursor0->GetGlobalNodeIndex();

  int neighborIdx = -1;
  int masked = matMask->GetValue( id0 );
  // In 3D masked and unmasked cells are handles differently
  for ( unsigned int f = 0; f < 3; ++ f, neighborIdx *= 3 )
  {
    // For each plane, check both orientations
    for ( unsigned int o = 0; o < 2; ++ o, neighborIdx *= -1 )
    {
      // Retrieve face neighbor cursor
      vtkHyperTreeGrid::vtkHyperTreeSimpleCursor* cursor =
        superCursor->GetCursor( neighborIdx );
      vtkIdType id = cursor->GetGlobalNodeIndex();

      // Cell is masked, check if any of the face neighbors are unmasked
      if ( masked )
      {
        // Generate faces shared by an unmasked cell, break ties at same level
        if ( cursor->GetTree()
          && cursor->IsLeaf()
          && cursor->GetLevel() < cursor0->GetLevel() )
        {

          if ( id >=0 && ! matMask->GetValue( id ) )
          {
            this->AddFace( id0, superCursor->Origin, superCursor->Size, o, f );
          }
        }
      }
      else
      {
        // Boundary faces, or faces shared by a masked cell, must be created
        if ( ! cursor->GetTree()
          ||
          ( cursor->IsLeaf() && matMask->GetValue( id ) ) )
        {
          this->AddFace( id0, superCursor->Origin, superCursor->Size, o, f );
        }
      }
    } // o
  } // f
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::AddFace( vtkIdType inId,
                                        double* origin, double* size,
                                        int offset, int orientation )
{
  // Initialize point
  double pt[3];
  memcpy( pt, origin, 3 * sizeof(double) );

  if ( offset )
  {
    pt[orientation] += size[orientation];
  }

  // Storage for face vertices
  vtkIdType ids[4];

  // Create origin vertex
  ids[0] = this->Points->InsertNextPoint( pt );

  // Create other face vertices depending on orientation
  int axis1 = ( orientation == 0 ) ? 1 : 0;
  int axis2 = ( orientation == 2 ) ? 1 : 2;

  pt[axis1] += size[axis1];
  ids[1] = this->Points->InsertNextPoint( pt );
  pt[axis2] += size[axis2];
  ids[2] = this->Points->InsertNextPoint( pt );
  pt[axis1] = origin[axis1];
  ids[3] = this->Points->InsertNextPoint( pt );

  // Insert face
  vtkIdType outId = this->Cells->InsertNextCell( 4, ids );

  // Copy face data from that of the cell from which it comes
  this->OutData->CopyData( this->InData, inId, outId );
}
