/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridAxisClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridAxisClip.h"

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
#include "vtkQuadric.h"

#include <set>

vtkStandardNewMacro(vtkHyperTreeGridAxisClip);
vtkCxxSetObjectMacro(vtkHyperTreeGridAxisClip, Quadric, vtkQuadric);

//-----------------------------------------------------------------------------
vtkHyperTreeGridAxisClip::vtkHyperTreeGridAxisClip()
{
  // Defaut clipping mode is by plane
  this->ClipType = vtkHyperTreeGridAxisClip::PLANE;

  // Defaut normal axis is Z
  this->PlaneNormalAxis = 0;

  // Defaut place intercept is 0
  this->PlanePosition = 0.;

  // Default clipping box is a unit cube centered at origin
  this->Bounds[0] = -.5;
  this->Bounds[1] =  .5;
  this->Bounds[2] = -.5;
  this->Bounds[3] =  .5;
  this->Bounds[4] = -.5;
  this->Bounds[5] =  .5;

  // Default quadric is a sphere with radius 1 centered at origin
  this->Quadric = vtkQuadric::New();
  this->Quadric->SetCoefficients( 1., 1., 1.,
                                  0., 0., 0.,
                                  0., 0., 0.,
                                  -1. );

  // Defaut inside/out flag is false
  this->InsideOut = 0;

  // This filter always creates an output with a material mask
  this->MaterialMask = vtkBitArray::New();

  // Output indices begin at 0
  this->CurrentId = 0;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridAxisClip::~vtkHyperTreeGridAxisClip()
{
  if( this->MaterialMask )
  {
    this->MaterialMask->Delete();
    this->MaterialMask = nullptr;
  }

  if ( this->Quadric )
  {
    this->Quadric->UnRegister( this );
    this->Quadric = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAxisClip::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "ClipType: " << this->ClipType << endl;
  os << indent << "PlaneNormalAxis: " << this->PlaneNormalAxis << endl;
  os << indent << "PlanePosition: " << this->PlanePosition << endl;
  os << indent << "Bounds: "
     <<  this->Bounds[0] << "-" <<  this->Bounds[1] << ", "
     <<  this->Bounds[2] << "-" <<  this->Bounds[3] << ", "
     <<  this->Bounds[4] << "-" <<  this->Bounds[5] << endl;
  os << indent << "InsideOut: " << this->InsideOut << endl;
  os << indent << "MaterialMask: " << this->MaterialMask << endl;
  os << indent << "CurrentId: " << this->CurrentId << endl;

  if ( this->Quadric )
  {
    this->Quadric->PrintSelf( os, indent.GetNextIndent() );
  }
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridAxisClip::FillOutputPortInformation( int, vtkInformation *info )
{
  info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid" );
  return 1;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridAxisClip::GetMinimumBounds( double bMin[3] )
{
  bMin[0] = this->Bounds[0];
  bMin[1] = this->Bounds[2];
  bMin[2] = this->Bounds[4];
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridAxisClip::GetMaximumBounds( double bMax[3] )
{
  bMax[0] = this->Bounds[1];
  bMax[1] = this->Bounds[3];
  bMax[2] = this->Bounds[5];
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridAxisClip::SetQuadricCoefficients( double q[10] )
{
  if ( ! this->Quadric )
  {
    this->Quadric = vtkQuadric::New();
  }
  this->Quadric->SetCoefficients( q );
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridAxisClip::GetQuadricCoefficients( double q[10] )
{
  this->Quadric->GetCoefficients( q );
}

//-----------------------------------------------------------------------------
double* vtkHyperTreeGridAxisClip::GetQuadricCoefficients()
{
  return this->Quadric->GetCoefficients();
}

//----------------------------------------------------------------------------
vtkMTimeType vtkHyperTreeGridAxisClip::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();

  if ( this->Quadric )
  {
    vtkMTimeType time = this->Quadric->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }

  return mTime;
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridAxisClip::IsClipped( vtkHyperTreeGridCursor* cursor )
{
  // Check clipping status depending on clip type
  switch ( this->ClipType )
  {
    case vtkHyperTreeGridAxisClip::PLANE:
    {
      // Retrieve normal axis and intercept of plane
      int axis = this->PlaneNormalAxis;
      double inter = this->PlanePosition;

      // Retrieve geometric origin of input cursor
      double* origin = cursor->GetOrigin();

      // Decide whether cell is clipped out depending on inside/out flag
      if ( ! this->InsideOut )
      {
        // Check whether cursor is above hyperplane
        if ( origin[axis] > inter )
        {
          return true;
        }
      } // if ( ! this->InsideOut )
      else
      {
        // Retrieve geometric size of input cursor
        double* size = cursor->GetSize();

        // Check whether cursor is below hyperplane
        if ( origin[axis] + size[axis] < inter )
        {
          return true;
        }
      } // else
      break;
    } // case PLANE
    case vtkHyperTreeGridAxisClip::BOX:
    {
      // Retrieve bounds of box
      double bMin[3], bMax[3];
      this->GetMinimumBounds( bMin );
      this->GetMaximumBounds( bMax );

      // Retrieve geometric origin and size of input cursor
      double* cMin = cursor->GetOrigin();
      double* size = cursor->GetSize();

      // Compute upper bounds of input cursor
      double cMax[3];
      cMax[0] = cMin[0] + size[0];
      cMax[1] = cMin[1] + size[1];
      cMax[2] = cMin[2] + size[2];

      for ( unsigned int d = 0; d < 3; ++ d )
      {
        // By default assume the cell will be clipped out
        bool clipped = true;

        // Do not clip cell yet if its lower bound is in prescribed bounds
        if ( ( cMin[d] >= bMin[d] ) && ( cMin[d] <= bMax[d] ) )
        {
          clipped = false;
        }
        // Do not clip it either if it contains the prescribed lower bound
        else if ( ( bMin[d] >= cMin[d] ) && ( bMin[d] <= cMax[d] ) )
        {
          clipped = false;
        }

        // Do not clip cell yet if its upper bound is in prescribed bounds
        if ( ( cMax[d] >= bMin[d] ) && ( cMax[d] <= bMax[d] ) )
        {
          clipped = false;
        }
        // Do not clip it either if it contains the prescribed upper bound
        else if ( ( bMax[d] >= cMin[d] ) && ( bMax[d] <= cMax[d] ) )
        {
          clipped = false;
        }

        // Being clipped in one dimension is sufficient to be clipped for good
        if ( clipped )
        {
          return true;
        }
      } // d
      break;
    } // case BOX
    case vtkHyperTreeGridAxisClip::QUADRIC:
    {
      // Retrieve geometric origin and size of input cursor
      double* origin = cursor->GetOrigin();
      double* size = cursor->GetSize();

      // Iterate over all vertices
      double nVert = 1 << cursor->GetDimension();
      for ( int v = 0; v < nVert; ++ v )
      {
        // Transform flat index into triple
        div_t d1 = div( v, 2 );
        div_t d2 = div( d1.quot, 2 );

        // Compute vertex coordinates
        double pt[3];
        pt[0] = origin[0] + d1.rem * size[0];
        pt[1] = origin[1] + d2.rem * size[1];
        pt[2] = origin[2] + d2.quot * size[2];

        // Evaluate quadric at current vertex
        double qv = this->Quadric->EvaluateFunction( pt );
        if ( qv < 0 )
        {
          // Found negative value at this vertex, cell not clipped out
          return false;
        }
      } // v

      // All quadric values are positive at cell vertices, it is clipped out
      return true;
    } // case QUADRIC
  } //  switch ( this->ClipType )

  return false;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridAxisClip::ProcessTrees( vtkHyperTreeGrid* input,
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

  // Retrieve input dimension
  unsigned int dimension = input->GetDimension();

  // This filter works only with 3D grids
  if ( dimension == 2 &&
       static_cast<unsigned int>(this->PlaneNormalAxis) == input->GetOrientation() )
  {
    vtkErrorMacro (<< "In 2D axis clip direction cannot be normal to grid plane:"
                   << input->GetOrientation( ));
    return 0;
  }
  else if ( dimension == 1 &&
            static_cast<unsigned int>(this->PlaneNormalAxis) == input->GetOrientation() )
  {
    vtkErrorMacro (<< "In 1D axis clip direction cannot be that of grid axis:"
                   << input->GetOrientation( ));
    return 0;
  }

  // Set identical grid parameters
  output->SetDimension( dimension );
  output->SetOrientation( input->GetOrientation() );
  output->SetTransposedRootIndexing( input->GetTransposedRootIndexing() );
  output->SetBranchFactor( input->GetBranchFactor() );
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
    = input->HasMaterialMask() ? input->GetMaterialMask() : 0;

  // Storage for Cartesian indices
  unsigned int cart[3];

  // Storage for global indices of clipped out root cells
  std::set<vtkIdType> clipped;

  // Storage for material mask indices computed together with output grid
  vtkNew<vtkIdTypeArray> position;

  // First pass across tree roots: compute extent of output grid indices
  unsigned int inSize[3];
  input->GetGridSize( inSize );
  unsigned int minId[] = { 0, 0, 0 };
  unsigned int maxId[] = { 0, 0, 0 };
  unsigned int outSize[3];
  vtkIdType inIndex;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator( it );
  while ( it.GetNextTree( inIndex ) )
  {
    // Initialize new geometric cursor at root of current input tree
    vtkHyperTreeGridCursor* inCursor = input->NewGeometricCursor( inIndex );

    // Check whether root cell is intersected by plane
    if ( ! this->IsClipped( inCursor ) )
    {
      // Root is intersected by plane, compute its Cartesian coordinates
      input->GetLevelZeroCoordinatesFromIndex( inIndex, cart[0], cart[1], cart[2] );

      // Update per-coordinate grid extrema if needed
      for ( unsigned int d = 0; d < 3; ++ d )
      {
        if ( cart[d] < minId[d] )
        {
          minId[d] = cart[d];
        }
        else if ( cart[d] > maxId[d] )
        {
          maxId[d] = cart[d];
        }
      } // d
    } // if ( ! this->IsClipped( inCursor ) )
    else
    {
      // This tree root is clipped out, keep track of its global index
      clipped.insert( inIndex );
    } // else

    // Clean up
    inCursor->Delete();
  } // it

  // Set grid sizes
  outSize[0] = maxId[0] - minId[0] + 1;
  outSize[1] = maxId[1] - minId[1] + 1;
  outSize[2] = maxId[2] - minId[2] + 1;
  output->SetGridSize( outSize );

  // Compute or copy output coordinates depending on output grid sizes
  vtkDataArray* inCoords[3];
  inCoords[0] = input->GetXCoordinates();
  inCoords[1] = input->GetYCoordinates();
  inCoords[2] = input->GetZCoordinates();
  vtkDataArray* outCoords[3];
  outCoords[0] = output->GetXCoordinates();
  outCoords[1] = output->GetYCoordinates();
  outCoords[2] = output->GetZCoordinates();
  for ( unsigned int d = 0; d < 3; ++ d )
  {
    if ( inSize[d] != outSize[d] )
    {
      // Coordinate extent along d-axis is clipped
      outCoords[d]->SetNumberOfTuples( outSize[d] + 1 );
      for ( unsigned int m = 0; m <= outSize[d]; ++ m )
      {
        unsigned int n = m + minId[d];
        outCoords[d]->SetTuple1( m, inCoords[d]->GetTuple1( n ) );
      }
    } // if ( inSize[d] != outSize[d] )
    else
    {
      // Coordinate extent along d-axis is unchanged
      outCoords[d]->ShallowCopy( inCoords[d] );
    } // else
  } // d

  // Second pass across tree roots: now compute clipped grid recursively
  vtkIdType outIndex = 0;
  input->InitializeTreeIterator( it );
  while ( it.GetNextTree( inIndex ) )
  {
    // Initialize new geometric cursor at root of current input tree
    vtkHyperTreeGridCursor* inCursor = input->NewGeometricCursor( inIndex );

    // Descend only tree roots that have not already been determined to be clipped out
    if( clipped.find( inIndex ) == clipped.end() )
    {
      // Root is intersected by plane, descend into current child
      input->GetLevelZeroCoordinatesFromIndex( inIndex, cart[0], cart[1], cart[2] );

      // Get root index into output hyper tree grid
      output->GetIndexFromLevelZeroCoordinates( outIndex, cart[0], cart[1], cart[2] );

      // Initialize new cursor at root of current output tree
      vtkHyperTreeCursor* outCursor = output->NewCursor( outIndex, true );
      outCursor->ToRoot();

      // Clip tree recursively
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
void vtkHyperTreeGridAxisClip::RecursivelyProcessTree( vtkHyperTreeGridCursor* inCursor,
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

  // Flag to keep track of whether current input cell is clipped out
  bool clipped = this->IsClipped( inCursor );

  // Copy output cell data from that of input cell
  this->OutData->CopyData( this->InData, inId, outId );

  // Descend further into input trees only if cursor is neither a leaf nor clipped out
  if ( ! inCursor->IsLeaf() && ! clipped )
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

      // Child is not clipped out, descend into current child
      outCursor->ToChild( outChild );

      // Recurse
      this->RecursivelyProcessTree( childCursor, outCursor, mask );

      // Return to parent
      outCursor->ToParent();

      // Increment output children count
      ++ outChild;

      // Clean up
      childCursor->Delete();
      childCursor = 0;
    } // inChild
  } // if ( ! cursor->IsLeaf() && ! clipped )
  else if ( ! clipped && mask && mask->GetValue( inId ) )
  {
    // Handle case of not clipped but nonetheless masked leaf cells
    clipped = true;
  } // else

  // Mask output cell if necessary
  this->MaterialMask->InsertTuple1 ( outId, clipped );
}
