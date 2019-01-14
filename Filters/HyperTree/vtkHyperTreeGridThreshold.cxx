/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridThreshold.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridThreshold.h"

#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include "vtkHyperTreeGridNonOrientedCursor.h"

vtkStandardNewMacro(vtkHyperTreeGridThreshold);

//-----------------------------------------------------------------------------
vtkHyperTreeGridThreshold::vtkHyperTreeGridThreshold()
{
  // Use minimum double value by default for lower threshold bound
  this->LowerThreshold = VTK_DBL_MIN;

  // Use maximum double value by default for upper threshold bound
  this->UpperThreshold = vtkMath::Inf();

  // This filter always creates an output with a material mask
  this->OutMaterialMask = vtkBitArray::New();

  // Output indices begin at 0
  this->CurrentId = 0;

  // Process active point scalars by default
  this->SetInputArrayToProcess( 0, 0, 0,
                                vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS,
                                vtkDataSetAttributes::SCALARS );

  // Input scalars point to null by default
  this->InScalars = nullptr;

  // By default, just create a new mask
  this->JustCreateNewMask = true;

  this->AppropriateOutput = true;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridThreshold::~vtkHyperTreeGridThreshold()
{
  if( this->OutMaterialMask )
  {
    this->OutMaterialMask->Delete();
    this->OutMaterialMask = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridThreshold::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "LowerThreshold: " << this->LowerThreshold << endl;
  os << indent << "UpperThreshold: " << this->UpperThreshold << endl;
  os << indent << "OutMaterialMask: " << this->OutMaterialMask << endl;
  os << indent << "CurrentId: " << this->CurrentId << endl;

  if( this->InScalars )
  {
    os << indent << "InScalars:\n";
    this->InScalars->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "InScalars: ( none )\n";
  }
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridThreshold::FillOutputPortInformation( int, vtkInformation* info )
{
  info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid" );
  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridThreshold::ThresholdBetween( double minimum,
                                                  double maximum )
{
  this->LowerThreshold = minimum;
  this->UpperThreshold = maximum;
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridThreshold::ProcessTrees( vtkHyperTreeGrid* input,
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

  // Retrieve scalar quantity of interest
  this->InScalars = this->GetInputArrayToProcess( 0, input );
  if ( ! this->InScalars )
  {
    vtkWarningMacro(<<"No scalar data to threshold");
    return 1;
  }

  // Retrieve material mask
  this->InMaterialMask = input->HasMaterialMask() ? input->GetMaterialMask() : nullptr;

  if ( this->JustCreateNewMask )
  {
    output->ShallowCopy( input );

    this->OutMaterialMask->SetNumberOfTuples( output->GetNumberOfPoints() );

    // Iterate over all input and output hyper trees
    vtkIdType outIndex;
    vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
    output->InitializeTreeIterator( it );
    vtkNew<vtkHyperTreeGridNonOrientedCursor> outCursor;
    while ( it.GetNextTree( outIndex ) )
    {
      // Initialize new grid cursor at root of current input tree
      output->InitializeNonOrientedCursor( outCursor, outIndex );
      // Limit depth recursively
      this->RecursivelyProcessTreeWithCreateNewMask( outCursor );
    } // it
  } else {
    // Set grid parameters
    output->SetGridSize( input->GetGridSize() );
    output->SetTransposedRootIndexing( input->GetTransposedRootIndexing() );
    output->SetBranchFactor( input->GetBranchFactor() );
    output->SetDimension( input->GetDimension() );
    output->SetOrientation( input->GetOrientation() );
    output->SetXCoordinates( input->GetXCoordinates() );
    output->SetYCoordinates( input->GetYCoordinates() );
    output->SetZCoordinates( input->GetZCoordinates() );
//JBDEL2    output->SetMaterialMaskIndex( input->GetMaterialMaskIndex() );
    output->SetHasInterface( input->GetHasInterface() );
    output->SetInterfaceNormalsName( input->GetInterfaceNormalsName() );
    output->SetInterfaceInterceptsName( input->GetInterfaceInterceptsName() );

    // Initialize output point data
    this->InData = input->GetPointData();
    this->OutData = output->GetPointData();
    this->OutData->CopyAllocate( this->InData );

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
  }

  // Squeeze and set output material mask if necessary
  if( this->OutMaterialMask )
  {
    this->OutMaterialMask->Squeeze();
    output->SetMaterialMask( this->OutMaterialMask );
  }

  this->UpdateProgress( 1. );
  return 1;
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridThreshold::RecursivelyProcessTree( vtkHyperTreeGridNonOrientedCursor* inCursor,
                                                        vtkHyperTreeGridNonOrientedCursor* outCursor )
{
  // Retrieve global index of input cursor
  vtkIdType inId = inCursor->GetGlobalNodeIndex();

  // Increase index count on output: postfix is intended
  vtkIdType outId = this->CurrentId ++;

  // Copy out cell data from that of input cell
  this->OutData->CopyData( this->InData, inId, outId );

  // Retrieve output tree and set global index of output cursor
  vtkHyperTree* outTree = outCursor->GetTree();
  outTree->SetGlobalIndexFromLocal( outCursor->GetVertexId(), outId );

  // Flag to recursively decide whether a tree node should discarded
  bool discard = true;

  if ( this->InMaterialMask && this->InMaterialMask->GetValue( inId ) )
  {
    // Mask output cell if necessary
    this->OutMaterialMask->InsertTuple1 ( outId, discard );

    // Return whether current node is within range
    return discard;
  }

  // Descend further into input trees only if cursor is not at leaf
  if ( ! inCursor->IsLeaf() )
  {
    // Cursor is not at leaf, subdivide output tree one level further
    outCursor->SubdivideLeaf( );

    // If input cursor is neither at leaf nor at maximum depth, recurse to all children
    int numChildren = inCursor->GetNumberOfChildren();
    for ( int ichild = 0; ichild < numChildren; ++ ichild )
    {
      // Descend into child in intput grid as well
      inCursor->ToChild( ichild );
      // Descend into child in output grid as well
      outCursor->ToChild( ichild );
      // Recurse and keep track of whether some children are kept
      discard &= this->RecursivelyProcessTree( inCursor, outCursor );
      // Return to parent in input grid
      outCursor->ToParent();
      // Return to parent in output grid
      inCursor->ToParent();
    } // child
  } // if ( ! inCursor->IsLeaf() && inCursor->GetCurrentDepth() < this->Depth )
  else
  {
    // Input cursor is at leaf, check whether it is within range
    double value = this->InScalars->GetTuple1( inId );
    if( ! ( this->InMaterialMask && this->InMaterialMask->GetValue( inId ) )
        && value >= this->LowerThreshold && value <= this->UpperThreshold )
    {
      // Cell is not masked and is within range, keep it
      discard = false;
    }
  } // else

  // Mask output cell if necessary
  this->OutMaterialMask->InsertTuple1 ( outId, discard );

  // Return whether current node is within range
  return discard;
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridThreshold::RecursivelyProcessTreeWithCreateNewMask( vtkHyperTreeGridNonOrientedCursor* outCursor )
{
  // Retrieve global index of input cursor
  vtkIdType outId = outCursor->GetGlobalNodeIndex();

  // Flag to recursively decide whether a tree node should discarded
  bool discard = true;

  if ( this->InMaterialMask && this->InMaterialMask->GetValue( outId ) )
  {
    // Mask output cell if necessary
    this->OutMaterialMask->InsertTuple1 ( outId, discard );

    // Return whether current node is within range
    return discard;
  }

  // Descend further into input trees only if cursor is not at leaf
  if ( ! outCursor->IsLeaf() )
  {
    // If input cursor is neither at leaf nor at maximum depth, recurse to all children
    int numChildren = outCursor->GetNumberOfChildren();
    for ( int ichild = 0; ichild < numChildren; ++ ichild )
    {
      // Descend into child in output grid as well
      outCursor->ToChild( ichild );
      // Recurse and keep track of whether some children are kept
      discard &= this->RecursivelyProcessTreeWithCreateNewMask( outCursor );
      // Return to parent in output grid
      outCursor->ToParent();
    } // child
  } // if ( ! inCursor->IsLeaf() && inCursor->GetCurrentDepth() < this->Depth )
  else
  {
    // Input cursor is at leaf, check whether it is within range
    double value = this->InScalars->GetTuple1( outId );
    discard = value < this->LowerThreshold || value > this->UpperThreshold;
  } // else

  // Mask output cell if necessary
  this->OutMaterialMask->InsertTuple1 ( outId, discard );

  // Return whether current node is within range
  return discard;
}
