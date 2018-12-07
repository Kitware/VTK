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
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointData.h"

#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"

vtkStandardNewMacro(vtkHyperTreeGridDepthLimiter);

//-----------------------------------------------------------------------------
vtkHyperTreeGridDepthLimiter::vtkHyperTreeGridDepthLimiter()
{
  // Require root-level depth by default
  this->Depth = 0;

  // Default mask is emplty
  this->OutMaterialMask = nullptr;

  // Output indices begin at 0
  this->CurrentId = 0;

  // JB Pour sortir un maillage de meme type que celui en entree
  this->AppropriateOutput = true;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridDepthLimiter::~vtkHyperTreeGridDepthLimiter()
{
  if( this->OutMaterialMask )
  {
    this->OutMaterialMask->Delete();
    this->OutMaterialMask = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridDepthLimiter::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "Depth: " << this->Depth << endl;
  os << indent << "OutMaterialMask: " << this->OutMaterialMask << endl;
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
//JBDEL2  output->SetMaterialMaskIndex( input->GetMaterialMaskIndex() );
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
    this->OutMaterialMask = vtkBitArray::New();
  }

  // Retrieve material mask
  this->InMaterialMask = this->OutMaterialMask ? input->GetMaterialMask() : 0;

  // Output indices begin at 0
  this->CurrentId = 0;

  // Iterate over all input and output hyper trees
  vtkIdType inIndex;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator( it );
  vtkNew<vtkHyperTreeGridNonOrientedCursor> inCursor;
  vtkNew<vtkHyperTreeGridNonOrientedCursor> outCursor;
  while ( it.GetNextTree( inIndex ) )
  {
    // Initialize new grid cursor at root of current input tree
    input->InitializeNonOrientedCursor( inCursor, inIndex );

    // Initialize new cursor at root of current output tree
    output->InitializeNonOrientedCursor( outCursor, inIndex, true );

    // Limit depth recursively
    this->RecursivelyProcessTree( inCursor, outCursor );
  } // it

  // Squeeze and set output material mask if necessary
  if( this->OutMaterialMask )
  {
    this->OutMaterialMask->Squeeze();
    output->SetMaterialMask( this->OutMaterialMask );
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridDepthLimiter::RecursivelyProcessTree( vtkHyperTreeGridNonOrientedCursor* inCursor,
                                                           vtkHyperTreeGridNonOrientedCursor* outCursor )
{
  // Retrieve global index of input cursor
  vtkIdType inId = inCursor->GetGlobalNodeIndex();

  // Increase index count on output: postfix is intended
  vtkIdType outId = this->CurrentId ++;

  // Retrieve output tree and set global index of output cursor
  vtkHyperTree* outTree = outCursor->GetTree();
  outTree->SetGlobalIndexFromLocal( outCursor->GetVertexId(), outId );

  // Update material mask if relevant
  if( this->InMaterialMask )
  {
    // Check whether non-leaf at maximum depth is reached
    if ( inCursor->GetLevel() == this->Depth && ! inCursor->IsLeaf() )
    {
      // If yes, then it becomes an output leaf that must be visible
      this->OutMaterialMask->InsertValue( outId, 0 );
    }
    else
    {
      // Otherwise, use input mask value
      this->OutMaterialMask->InsertValue( outId, this->InMaterialMask->GetValue( inId )  );
    }
  } // if ( this->InMaterialMask )

  // Copy output cell data from that of input cell
  this->OutData->CopyData( this->InData, inId, outId );

  // Descend further into input trees only if cursor is not at leaf and depth not reached
  if ( ! inCursor->IsLeaf() && inCursor->GetLevel() < this->Depth )
  {
    // Cursor is not at leaf, subdivide output tree one level further
    outCursor->SubdivideLeaf();

    // If input cursor is neither at leaf nor at maximum depth, recurse to all children
    int numChildren = inCursor->GetNumberOfChildren();
    for ( int child = 0; child < numChildren; ++ child )
    {
      inCursor->ToChild( child );
      // Descend into child in output grid as well
      outCursor->ToChild( child );
      // Recurse
      this->RecursivelyProcessTree( inCursor, outCursor );
      // Return to parent in output grid
      inCursor->ToParent();
      outCursor->ToParent();
    } // child
  } // if ( ! inCursor->IsLeaf() && inCursor->GetCurrentDepth() < this->Depth )
}
