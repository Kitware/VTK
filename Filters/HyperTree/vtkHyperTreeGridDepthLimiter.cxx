/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridDepthLimiter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridDepthLimiter.h"

#include "vtkBitArray.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeCursor.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridCursor.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkHyperTreeGridDepthLimiter);

//-----------------------------------------------------------------------------
vtkHyperTreeGridDepthLimiter::vtkHyperTreeGridDepthLimiter()
{
  // Require root-level depth by default
  this->Depth = 0;

  // Default mask is emplty
  this->MaterialMask = nullptr;

  // Output indices begin at 0
  this->CurrentId = 0;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridDepthLimiter::~vtkHyperTreeGridDepthLimiter()
{
  if( this->MaterialMask )
  {
    this->MaterialMask->Delete();
    this->MaterialMask = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridDepthLimiter::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "Depth: " << this->Depth << endl;
  os << indent << "MaterialMask: " << this->MaterialMask << endl;
  os << indent << "CurrentId: " << this->CurrentId << endl;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridDepthLimiter::FillOutputPortInformation( int, vtkInformation* info )
{
  info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid" );
  return 1;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridDepthLimiter::ProcessTrees( vtkHyperTreeGrid* input,
                                                vtkDataObject* outputDO )
{
  // Downcast output data object to hyper tree grid
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast( outputDO );
  if ( ! output )
  {
    vtkErrorMacro( "Incorrect type of output: "
                   << outputDO->GetClassName() );
    return 0;
  }

  // Set grid parameters
  output->SetGridSize( input->GetGridSize() );
  output->SetTransposedRootIndexing( input->GetTransposedRootIndexing() );
  output->SetBranchFactor( input->GetBranchFactor() );
  output->SetDimension( input->GetDimension() );
  output->SetOrientation( input->GetOrientation() );
  output->SetXCoordinates( input->GetXCoordinates() );
  output->SetYCoordinates( input->GetYCoordinates() );
  output->SetZCoordinates( input->GetZCoordinates() );
  output->SetMaterialMaskIndex( input->GetMaterialMaskIndex() );
  output->SetHasInterface( input->GetHasInterface() );
  output->SetInterfaceNormalsName( input->GetInterfaceNormalsName() );
  output->SetInterfaceInterceptsName( input->GetInterfaceInterceptsName() );

  // Initialize output point data
  this->InData = input->GetPointData();
  this->OutData = output->GetPointData();
  this->OutData->CopyAllocate( this->InData );

  // Output indices begin at 0
  this->CurrentId = 0;

  // Create material mask bit array if one is present on input
  if( input->HasMaterialMask() )
  {
    this->MaterialMask = vtkBitArray::New();
  }

  // Retrieve material mask
  vtkBitArray* mask
    = this->MaterialMask ? input->GetMaterialMask() : 0;

  // Initialize output trees
  output->GenerateTrees();

  // Output indices begin at 0
  this->CurrentId = 0;

  // Iterate over all input and output hyper trees
  vtkIdType inIndex;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator( it );

  while ( it.GetNextTree( inIndex ) )
  {
    // Initialize new grid cursor at root of current input tree
    vtkHyperTreeGridCursor* inCursor = input->NewGridCursor( inIndex );

    // Initialize new cursor at root of current output tree
    vtkHyperTreeCursor* outCursor = output->NewCursor( inIndex, true );
    outCursor->ToRoot();

    // Limit depth recursively
    this->RecursivelyProcessTree( inCursor, outCursor, mask );

    // Clean up
    inCursor->Delete();
    outCursor->Delete();
  } // it

  // Squeeze and set output material mask if necessary
  if( this->MaterialMask )
  {
    this->MaterialMask->Squeeze();
    output->SetMaterialMask( this->MaterialMask );
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridDepthLimiter::RecursivelyProcessTree( vtkHyperTreeGridCursor* inCursor,
                                                           vtkHyperTreeCursor* outCursor,
                                                           vtkBitArray* mask )
{
  // Retrieve input grid
  vtkHyperTreeGrid* input = inCursor->GetGrid();

  // Retrieve global index of input cursor
  vtkIdType inId = inCursor->GetGlobalNodeIndex();

  // Increase index count on output: postfix is intended
  vtkIdType outId = this->CurrentId ++;

  // Retrieve output tree and set global index of output cursor
  vtkHyperTree* outTree = outCursor->GetTree();
  outTree->SetGlobalIndexFromLocal( outCursor->GetVertexId(), outId );

  // Update material mask if relevant
  if( mask )
  {
    // Check whether non-leaf at maximum depth is reached
    if ( inCursor->GetLevel() == this->Depth && ! inCursor->IsLeaf() )
    {
      // If yes, then it becomes an output leaf that must be visible
      this->MaterialMask->InsertValue( outId, 0 );
    }
    else
    {
      // Otherwise, use input mask value
      this->MaterialMask->InsertValue( outId, mask->GetValue( inId )  );
    }
  } // if ( mask )

  // Copy output cell data from that of input cell
  this->OutData->CopyData( this->InData, inId, outId );

  // Descend further into input trees only if cursor is not at leaf and depth not reached
  if ( ! inCursor->IsLeaf() && inCursor->GetLevel() < this->Depth )
  {
    // Cursor is not at leaf, subdivide output tree one level further
    outTree->SubdivideLeaf( outCursor );

    // If input cursor is neither at leaf nor at maximum depth, recurse to all children
    int numChildren = input->GetNumberOfChildren();
    for ( int child = 0; child < numChildren; ++ child )
    {
      // Create child cursor from parent in input grid
      vtkHyperTreeGridCursor* childCursor = inCursor->Clone();
      childCursor->ToChild( child );

      // Descend into child in output grid as well
      outCursor->ToChild( child );

      // Recurse
      this->RecursivelyProcessTree( childCursor, outCursor, mask );

      // Return to parent in output grid
      outCursor->ToParent();

      // Clean up
      childCursor->Delete();
      childCursor = nullptr;
    } // child
  } // if ( ! inCursor->IsLeaf() && inCursor->GetCurrentDepth() < this->Depth )
}
