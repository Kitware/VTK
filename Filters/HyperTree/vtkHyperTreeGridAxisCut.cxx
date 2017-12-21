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
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeCursor.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridCursor.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkNew.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkHyperTreeGridAxisCut);

//-----------------------------------------------------------------------------
vtkHyperTreeGridAxisCut::vtkHyperTreeGridAxisCut()
{
  // Default normal axis is Z
  this->PlaneNormalAxis = 0;

  // Default place intercept is 0
  this->PlanePosition = 0.;

  // Default mask is empty
  this->MaterialMask = nullptr;

  // Output indices begin at 0
  this->CurrentId = 0;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridAxisCut::~vtkHyperTreeGridAxisCut()
{
  if( this->MaterialMask )
  {
    this->MaterialMask->Delete();
    this->MaterialMask = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAxisCut::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "PlaneNormalAxis : " << this->PlaneNormalAxis << endl;
  os << indent << "PlanePosition : " << this->PlanePosition << endl;
  os << indent << "MaterialMask: " << this->MaterialMask << endl;
  os << indent << "CurrentId: " << this->CurrentId << endl;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridAxisCut::FillOutputPortInformation( int, vtkInformation *info )
{
  info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid" );
  return 1;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridAxisCut::ProcessTrees( vtkHyperTreeGrid* input,
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

  // This filter works only with 3D grids
  if ( input->GetDimension() != 3 )
  {
    vtkErrorMacro (<< "Bad input dimension:"
                   << input->GetDimension());
    return 0;
  }
  output->SetDimension( 2 );

  // Retrieve normal axis and intercept of cut plane
  int axis = this->PlaneNormalAxis;
  double inter = this->PlanePosition;

  // Set output orientation
  output->SetOrientation( axis );

  // Set output grid sizes; must be 1 in the direction of cut plane normal
  unsigned int size[3];
  input->GetGridSize( size );
  size[axis] = 1;
  output->SetGridSize( size );

  // Duplicate coordinates along plane axes; set to zeros along normal
  vtkNew<vtkDoubleArray> zeros;
  zeros->SetNumberOfValues( 2 );
  zeros->SetValue( 0, inter );
  zeros->SetValue( 1, inter );
  switch ( axis )
  {
    case 0:
      // Cut along yz-plane
      output->SetXCoordinates( zeros );
      output->SetYCoordinates( input->GetYCoordinates() );
      output->SetZCoordinates( input->GetZCoordinates());
      break;
    case 1:
      // Cut along xz-plane
      output->SetXCoordinates( input->GetXCoordinates() );
      output->SetYCoordinates( zeros );
      output->SetZCoordinates( input->GetZCoordinates());
      break;
    case 2:
      // Cut along xz-plane
      output->SetXCoordinates( input->GetXCoordinates() );
      output->SetYCoordinates( input->GetYCoordinates() );
      output->SetZCoordinates( zeros );
      break;
    default:
      return 0;
  }

  // Other grid parameters are identical
  output->SetTransposedRootIndexing( input->GetTransposedRootIndexing() );
  output->SetBranchFactor( input->GetBranchFactor() );
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
    = this->MaterialMask ? input->GetMaterialMask() : nullptr;

  // Storage for root cell Cartesian coordinates
  unsigned int i,j,k;

  // Storage for material mask indices computed together with output grid
  vtkNew<vtkIdTypeArray> position;

  // Iterate over all input hyper trees
  vtkIdType inIndex;
  vtkIdType outIndex = 0;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator( it );
  while ( it.GetNextTree( inIndex ) )
  {
    // Initialize new geometric cursor at root of current input tree
    vtkHyperTreeGridCursor* inCursor = input->NewGeometricCursor( inIndex );

    // Retrieve geometric features of input cursor
    double* origin = inCursor->GetOrigin();
    double* _size = inCursor->GetSize();

    // Check whether root cell is intersected by plane
    if ( origin[axis] <= inter && ( origin[axis] + _size[axis] >= inter ) )
    {
      // Root is intersected by plane, descend into current child
      input->GetLevelZeroCoordinatesFromIndex( inIndex, i, j, k );

      // Get root index into output hyper tree grid, depending on cut axes
      switch ( axis )
      {
        case 0:
          output->GetIndexFromLevelZeroCoordinates( outIndex, 0, j, k );
          break;
        case 1:
          output->GetIndexFromLevelZeroCoordinates( outIndex, i, 0, k );
          break;
        case 2:
          output->GetIndexFromLevelZeroCoordinates( outIndex, i, j, 0);
          break;
        default:
          vtkErrorMacro( "Incorrect orientation of output: "
                         << axis );
          return 0;
      } // switch ( axis )

      // Initialize new cursor at root of current output tree
      vtkHyperTreeCursor* outCursor = output->NewCursor( outIndex, true );
      outCursor->ToRoot();

      // Cut tree recursively
      this->RecursivelyProcessTree( inCursor, outCursor, mask );

      // Store current output index then increment it
      position->InsertNextValue( outIndex );
      ++ outIndex;

      // Clean up
      outCursor->Delete();
    } // if origin

    // Clean up
    inCursor->Delete();
  } // it

  // Set material mask index
  output->SetMaterialMaskIndex( position );

  // Squeeze and set output material mask if necessary
  if( this->MaterialMask )
  {
    this->MaterialMask->Squeeze();
    output->SetMaterialMask( this->MaterialMask );
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAxisCut::RecursivelyProcessTree( vtkHyperTreeGridCursor* inCursor,
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
    this->MaterialMask->InsertValue( outId, mask->GetValue( inId )  );
  }

  // Copy output cell data from that of input cell
  this->OutData->CopyData( this->InData, inId, outId );

  // Descend further into input trees only if cursor is not at leaf
  if ( ! inCursor->IsLeaf() )
  {
    // Cursor is not at leaf, subdivide output tree one level further
    outTree->SubdivideLeaf( outCursor );

    // Initialize output children index
    int outChild = 0;

    // If cursor is not at leaf, recurse to all children
    int numChildren = input->GetNumberOfChildren();
    for ( int inChild = 0; inChild < numChildren; ++ inChild )
    {
      // Create child cursor from parent
      vtkHyperTreeGridCursor* childCursor = inCursor->Clone();
      childCursor->ToChild( inChild );

      // Retrieve normal axis and intercept of plane
      int axis = this->PlaneNormalAxis;
      double inter = this->PlanePosition;

      // Retrieve geometric features of input cursor
      double* origin = childCursor->GetOrigin();
      double* size = childCursor->GetSize();

      // Check whether child is intersected by plane
      if ( origin[axis] <= inter && ( origin[axis] + size[axis] >= inter ) )
      {
        // Child is intersected by plane, descend into current child
        outCursor->ToChild( outChild );

        // Recurse
        this->RecursivelyProcessTree( childCursor, outCursor, mask );

        // Return to parent
        outCursor->ToParent();

        // Increment output children count
        ++ outChild;
      }

      // Clean up
      childCursor->Delete();
      childCursor = nullptr;
    } // inChild
  } // if ( ! cursor->IsLeaf() )
}
