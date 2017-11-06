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
#include "vtkHyperTreeGridCursor.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkHyperTreeGridThreshold);

//-----------------------------------------------------------------------------
vtkHyperTreeGridThreshold::vtkHyperTreeGridThreshold()
{
  // Use minimum double value by default for lower threshold bound
  this->LowerThreshold = VTK_DBL_MIN;

  // Use maximum double value by default for upper threshold bound
  this->UpperThreshold = vtkMath::Inf();

  // This filter always creates an output with a material mask
  this->MaterialMask = vtkBitArray::New();

  // Output indices begin at 0
  this->CurrentId = 0;

  // Process active point scalars by default
  this->SetInputArrayToProcess( 0, 0, 0,
                                vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS,
                                vtkDataSetAttributes::SCALARS );

  // Input scalars point to null by default
  this->InScalars = nullptr;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridThreshold::~vtkHyperTreeGridThreshold()
{
  if( this->MaterialMask )
  {
    this->MaterialMask->Delete();
    this->MaterialMask = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridThreshold::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "LowerThreshold: " << this->LowerThreshold << endl;
  os << indent << "UpperThreshold: " << this->UpperThreshold << endl;
  os << indent << "MaterialMask: " << this->MaterialMask << endl;
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

  // Retrieve material mask
  vtkBitArray* mask
    = input->HasMaterialMask() ? input->GetMaterialMask() : nullptr;

  // Initialize output trees
  output->GenerateTrees();

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
bool vtkHyperTreeGridThreshold::RecursivelyProcessTree( vtkHyperTreeGridCursor* inCursor,
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

  // Flag to recursively decide whether a tree node should discarded
  bool discard = true;

  // Copy out cell data from that of input cell
  this->OutData->CopyData( this->InData, inId, outId );

  // Descend further into input trees only if cursor is not at leaf
  if ( ! inCursor->IsLeaf() )
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

      // Recurse and keep track of whether some children are kept
      discard &= this->RecursivelyProcessTree( childCursor, outCursor, mask );

      // Return to parent in output grid
      outCursor->ToParent();

      // Clean up
      childCursor->Delete();
      childCursor = nullptr;
    } // child
  } // if ( ! inCursor->IsLeaf() && inCursor->GetCurrentDepth() < this->Depth )
  else
  {
    // Input cursor is at leaf, check whether it is within range
    double value = this->InScalars->GetTuple1( inId );
    if( ! ( mask && mask->GetValue( inId ) )
        && value >= this->LowerThreshold && value <= this->UpperThreshold )
    {
      // Cell is not masked and is within range, keep it
      discard = false;
    }
  } // else

  // Mask output cell if necessary
  this->MaterialMask->InsertTuple1 ( outId, discard );

  // Return whether current node is within range
  return discard;
}
