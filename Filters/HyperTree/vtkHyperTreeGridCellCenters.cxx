/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridCellCenters.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridCellCenters.h"

#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeCursor.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridCursor.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkHyperTreeGridCellCenters);

//-----------------------------------------------------------------------------
vtkHyperTreeGridCellCenters::vtkHyperTreeGridCellCenters()
{
  // Create storage for centers of leaf cells
  this->Points = vtkPoints::New();
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridCellCenters::~vtkHyperTreeGridCellCenters()
{
  if ( this->Points )
  {
    this->Points->Delete();
    this->Points = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridCellCenters::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  if( this->Points )
  {
    os << indent << "Points:\n";
    this->Points->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "Points: ( none )\n";
  }
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridCellCenters::FillOutputPortInformation( int,
                                                            vtkInformation* info )
{
  info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData" );
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridCellCenters::ProcessTrees( vtkHyperTreeGrid* input,
                                               vtkDataObject* outputDO )
{
  // Downcast output data object to polygonal data set
  vtkPolyData* output = vtkPolyData::SafeDownCast( outputDO );
  if ( ! output )
  {
    vtkErrorMacro( "Incorrect type of output: "
                   << outputDO->GetClassName() );
    return 0;
  }

  // Initialize output cell data
  this->InData = input->GetPointData();
  this->OutData = output->GetPointData();
  this->OutData->CopyAllocate( this->InData );

  // Retrieve material mask
  vtkBitArray* mask
    = input->HasMaterialMask() ? input->GetMaterialMask() : nullptr;

  // Iterate over all hyper trees
  vtkIdType index;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator( it );
  while ( it.GetNextTree( index ) )
  {
    // Initialize new geometric cursor at root of current tree
    vtkHyperTreeGridCursor* cursor = input->NewGeometricCursor( index );

    // Generate leaf cell centers recursively
    this->RecursivelyProcessTree( cursor, mask );

    // Clean up
    cursor->Delete();
  } // it

  // Set output geometry and topology if required
  output->SetPoints( this->Points );
  if ( this->VertexCells )
  {
    vtkIdType np = this->Points->GetNumberOfPoints();
    vtkCellArray* vertices = vtkCellArray::New();
    vertices->Allocate(np * 2);
    for( vtkIdType i = 0; i < np ; ++ i )
    {
      vertices->InsertNextCell( 1, &i );
    } // i
    output->SetVerts( vertices );
    vertices->Delete();
  } // this->VertexCells

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridCellCenters::RecursivelyProcessTree( vtkHyperTreeGridCursor* cursor,
                                                          vtkBitArray* mask )
{
  // Retrieve input grid
  vtkHyperTreeGrid* input = cursor->GetGrid();

  // Create cell center if cursor is at leaf
  if ( cursor->IsLeaf() )
  {
    // Cursor is at leaf, retrieve its global index
    vtkIdType id = cursor->GetGlobalNodeIndex();

    // If leaf is masked, skip it
    if ( mask && mask->GetValue( id ) )
    {
      return;
    }

    // Retrieve cell center coordinates
    double pt[3];
    cursor->GetPoint( pt );

    // Insert next point
    vtkIdType outId = this->Points->InsertNextPoint( pt );

    // Copy cell center data from leaf data, when needed
    if ( this->VertexCells )
    {
      this->OutData->CopyData( this->InData, id, outId );
    }
  }
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
      this->RecursivelyProcessTree( childCursor, mask );

      // Clean up
      childCursor->Delete();
      childCursor = nullptr;
    } // child
  } // else
}
