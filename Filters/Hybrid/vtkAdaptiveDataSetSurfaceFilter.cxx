/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdaptiveDataSetSurfaceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAdaptiveDataSetSurfaceFilter.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPolyData.h"
#include "vtkBitArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridCursor.h"
#include "vtkDataSetAttributes.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"

static const unsigned int VonNeumannCursors3D[] = { 0, 1, 2, 4, 5, 6 };
static const unsigned int VonNeumannOrientations3D[]  = { 2, 1, 0, 0, 1, 2 };
static const unsigned int VonNeumannOffsets3D[]  = { 0, 0, 0, 1, 1, 1 };

vtkStandardNewMacro(vtkAdaptiveDataSetSurfaceFilter);

//-----------------------------------------------------------------------------
vtkAdaptiveDataSetSurfaceFilter::vtkAdaptiveDataSetSurfaceFilter()
{
  this->InData = nullptr;
  this->OutData = nullptr;
  this->Points = nullptr;
  this->Cells = nullptr;

  // Default dimension is 0
  this->Dimension = 0;

  // Default orientation is 0
  this->Orientation = 0;

  this->Renderer = nullptr;

  this->LevelMax = -1;

  this->ParallelProjection = false;
  this->LastRendererSize[0] = 0;
  this->LastRendererSize[1] = 0;
  this->LastCameraFocalPoint[0] = 0.0;
  this->LastCameraFocalPoint[1] = 0.0;
  this->LastCameraFocalPoint[2] = 0.0;
  this->LastCameraParallelScale = 0;

  this->Scale = 1;
}

//-----------------------------------------------------------------------------
vtkAdaptiveDataSetSurfaceFilter::~vtkAdaptiveDataSetSurfaceFilter()
{

}

//----------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  if( this->InData )
  {
    os << indent << "InData:\n";
    this->InData->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "InData: ( none )\n";
  }

  if( this->OutData )
  {
    os << indent << "OutData:\n";
    this->OutData->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "OutData: ( none )\n";
  }

  if( this->Points )
  {
    os << indent << "Points:\n";
    this->Points->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "Points: ( none )\n";
  }

  if( this->Cells )
  {
    os << indent << "Cells:\n";
    this->Cells->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "Cells: ( none )\n";
  }

  os << indent << "Dimension: " << this->Dimension << endl;
  os << indent << "Orientation: " << this->Orientation << endl;
  os << indent << "Axis1: " << this->Axis1 << endl;
  os << indent << "Axis2: " << this->Axis2 << endl;
  os << indent << "Radius: " << this->Radius << endl;
  os << indent << "LevelMax: " << this->LevelMax << endl;
  os << indent << "ParallelProjection: " << this->ParallelProjection << endl;
  os << indent << "Scale: " << this->Scale << endl;
  os << indent << "LastCameraParallelScale: " << this->LastCameraParallelScale << endl;
  os << indent << "LastRendererSize: "
     << this->LastRendererSize[0] << ", "
     << this->LastRendererSize[1] << endl;
  os << indent << "LastCameraFocalPoint: "
     << this->LastCameraFocalPoint[0] << ", "
     << this->LastCameraFocalPoint[1] << ", "
     << this->LastCameraFocalPoint[2] << endl;
}

//----------------------------------------------------------------------------
int vtkAdaptiveDataSetSurfaceFilter::RequestData( vtkInformation* request,
                                                  vtkInformationVector** inputVector,
                                                  vtkInformationVector* outputVector )
{
  // Get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get( vtkDataObject::DATA_OBJECT() ) );
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get( vtkDataObject::DATA_OBJECT() ) );

  int MeshType = input->GetDataObjectType();
  if ( MeshType != VTK_HYPER_TREE_GRID )
  {
    return this->Superclass::RequestData( request, inputVector, outputVector );
  }

  return this->DataSetExecute( input, output );
}


//----------------------------------------------------------------------------
int vtkAdaptiveDataSetSurfaceFilter::DataSetExecute( vtkDataSet* inputDS,
                                                     vtkPolyData* output )
{
  // Retrieve input grid
  vtkHyperTreeGrid* input = vtkHyperTreeGrid::SafeDownCast( inputDS );
  if ( ! input )
  {
    return vtkDataSetSurfaceFilter::DataSetExecute( inputDS, output );
  }

  // Retrieve useful grid parameters for speed of access
  this->Dimension = input->GetDimension();
  this->Orientation = input->GetOrientation();

  // Initialize output cell data
  this->InData = static_cast<vtkDataSetAttributes*>( input->GetPointData() );
  this->OutData = static_cast<vtkDataSetAttributes*>( output->GetCellData() );
  this->OutData->CopyAllocate( this->InData );
  if ( this->PassThroughCellIds )
  {
    this->OriginalCellIds = vtkIdTypeArray::New();
    this->OriginalCellIds->SetName( this->GetOriginalCellIdsName() );
    this->OriginalCellIds->SetNumberOfComponents( 1 );
    this->OutData->AddArray( this->OriginalCellIds );
  }

  // Init renderer information
  if ( this->ParallelProjection && this->Renderer )
  {
    // Generate planes XY, XZ o YZ
    unsigned int* gridSize = input->GetGridSize();
    if ( gridSize[0] == 1 )
    {
      this->Axis1 = 1;
      this->Axis2 = 2;
    }
    else if ( gridSize[1] == 1 )
    {
      this->Axis1 = 0;
      this->Axis2 = 2;
    }
    else if ( gridSize[2] == 1 )
    {
      this->Axis1 = 0;
      this->Axis2 = 1;
    }

    // Compute the window size
    double windowSize = std::min(
      ( double ) this->LastRendererSize[0]/gridSize[this->Axis1],
      ( double ) this->LastRendererSize[1]/gridSize[this->Axis2] );

    // Compute the zoom of the camera
    vtkCamera* cam = this->Renderer->GetActiveCamera();
    double height = cam->GetParallelScale() * 2;
    double bounds[6];
    input->GetBounds( bounds );
    double zoom1 = ( bounds[2 * this->Axis1 + 1] - bounds[2 * this->Axis1] ) / height;
    double zoom2 = ( bounds[2 * this->Axis2 + 1] - bounds[2 * this->Axis2] ) / height;
    double zoom = std::max( zoom1, zoom2 );

    // Compute how many levels of the tree we should process
    int f = input->GetBranchFactor();
    this->LevelMax = ( log( windowSize ) + log( zoom / this->Scale ) ) / log( f ) + 1;
    if ( this->LevelMax < 0 )
    {
      this->LevelMax = 0;
    }
    double ratio = ( ( double ) this->LastRendererSize[0] ) / this->LastRendererSize[1];
    this->Radius = cam->GetParallelScale() * sqrt( 1 + ratio * ratio );
  }
  else
  {
    // Recurse all the tree
    this->LevelMax = -1;
  }

  // Extract geometry from hyper tree grid
  this->ProcessTrees( input, output );

  this->UpdateProgress( 1. );

  return 1;
}

//-----------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::ProcessTrees( vtkHyperTreeGrid* input,
                                                    vtkPolyData* output )
{
  // Create storage for corners of leaf cells
  this->Points = vtkPoints::New();

  // Create storage for untructured leaf cells
  this->Cells = vtkCellArray::New();

  // Retrieve material mask
  vtkBitArray* mask = input->HasMaterialMask() ? input->GetMaterialMask() : nullptr;

  //
  vtkUnsignedCharArray* ghost = input->GetPointGhostArray();
  if ( ghost )
  {
    this->OutData->CopyFieldOff( vtkDataSetAttributes::GhostArrayName() );
  }

  // Iterate over all hyper trees
  vtkIdType index;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator( it );
  while ( it.GetNextTree( index ) )
  {
    // Initialize new cursor at root of current tree
    vtkHyperTreeGridCursor* cursor;
    if ( this->Dimension == 3 )
    {
      // In 3 dimensions, von Neumann neighborhood information is needed
      cursor = input->NewVonNeumannSuperCursor( index );
    } // if ( this->Dimension == 3 )
    else
    {
      // Otherwise, geometric properties of the cells suffice
      cursor = input->NewGeometricCursor( index );
    } // else

    // If this is not a ghost tree
    if ( ! ghost || ! ghost->GetTuple1( cursor->GetGlobalNodeIndex() ) )
    {
      // Build geometry recursively
      this->RecursivelyProcessTree( cursor, mask, 0 );
    }

    // Clean up
    cursor->Delete();
  } // it

  // Set output geometry and topology
  output->SetPoints( this->Points );
  if ( this->Dimension == 1  )
  {
    output->SetLines( this->Cells );
  }
  else
  {
    output->SetPolys( this->Cells );
  }

  this->Points->Delete();
  this->Cells->Delete();
}

//----------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::RecursivelyProcessTree( vtkHyperTreeGridCursor* cursor,
                                                              vtkBitArray* mask,
                                                              int level )
{
  // Retrieve input grid
  vtkHyperTreeGrid* input = cursor->GetGrid();

  if ( this->Dimension == 3 )
  {
    // Create geometry output if cursor is at leaf
    if ( cursor->IsLeaf() )
    {
      this->ProcessLeaf3D( cursor, mask );
    } // if ( cursor->IsLeaf() )
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
        this->RecursivelyProcessTree( childCursor, mask, level+1 );

        // Clean up
        childCursor->Delete();
        childCursor = nullptr;
      } // child
    } // else
  } // if ( this->Dimension == 3 )
  else
  {
    bool insideBB = ( this->LevelMax == -1 );
    if( ! insideBB )
    {
      // Check if the current node of the tree is going to be rendered
      double half = std::max( cursor->GetSize()[this->Axis1] / 2,
                              cursor->GetSize()[this->Axis2] / 2 );
      insideBB = (
        pow( cursor->GetOrigin()[this->Axis1] + half - this->LastCameraFocalPoint[this->Axis1], 2 ) +
        pow( cursor->GetOrigin()[this->Axis2] + half - this->LastCameraFocalPoint[this->Axis2], 2 ) ) <
        pow( this->Radius + half * sqrt(2.), 2 );
    }
    if( insideBB )
    {
      // We only process those nodes than are going to be rendered
      if ( cursor->IsLeaf() || ( this->LevelMax != -1 && level >= this->LevelMax ) )
      {
        if ( this->Dimension == 2 )
        {
          this->ProcessLeaf2D( cursor, mask );
        }
        else
        {
          this->ProcessLeaf1D( cursor );
        } // else
      } // if ( cursor->IsLeaf() || ( this->LevelMax!=-1 && level >= this->LevelMax ) )
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
          this->RecursivelyProcessTree( childCursor, mask, level+1 );

          // Clean up
          childCursor->Delete();
          childCursor = nullptr;
        } // child
      } // else
    } // if( insideBB )
  } // else
}

//----------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::ProcessLeaf1D( vtkHyperTreeGridCursor* cursor )
{
  // In 1D the geometry is composed of edges, create storage for endpoint IDs
  vtkIdType id[2];

  // First endpoint is at origin of cursor
  double* origin = cursor->GetOrigin();
  id[0] = this->Points->InsertNextPoint( origin );

  // Second endpoint is at origin of cursor plus its length
  double pt[3];
  memcpy( pt, origin, 3 * sizeof( double ) );
  switch ( this->Orientation )
  {
    case 3: // 1 + 2
      pt[2] += cursor->GetSize()[2];
      break;
    case 5: // 1 + 4
      pt[1] += cursor->GetSize()[1];
      break;
    case 6: // 2 + 4
      pt[0] += cursor->GetSize()[0];
      break;
  } // switch
  id[1] = this->Points->InsertNextPoint( pt );

  // Insert edge into 1D geometry
  this->Cells->InsertNextCell( 2, id );
}

//----------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::ProcessLeaf2D( vtkHyperTreeGridCursor* cursor,
                                                     vtkBitArray* mask )

{
  // Cell at cursor center is a leaf, retrieve its global index
  vtkIdType id = cursor->GetGlobalNodeIndex();
  if ( id < 0 )
  {
    return;
  }

  // In 2D all unmasked faces are generated
  if ( ! mask  || ! mask->GetValue( id ) )
  {
    // Insert face into 2D geometry depending on orientation
    this->AddFace( id, cursor->GetOrigin(), cursor->GetSize(), 0, this->Orientation );
  }
}

//----------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::ProcessLeaf3D( vtkHyperTreeGridCursor* superCursor,
                                                     vtkBitArray* mask )
{
  // Cell at super cursor center is a leaf, retrieve its global index, level, and mask
  vtkIdType id = superCursor->GetGlobalNodeIndex();
  unsigned level = superCursor->GetLevel();
  int masked = mask ? mask->GetValue( id ) : 0;

  // Iterate over all cursors of Von Neumann neighborhood around center
  unsigned int nc = superCursor->GetNumberOfCursors() - 1;
  for ( unsigned int c = 0 ; c < nc; ++ c )
  {
    // Retrieve cursor to neighbor across face
    vtkHyperTreeGridCursor* cursorN = superCursor->GetCursor( VonNeumannCursors3D[c] );

    // Retrieve tree, leaf flag, and mask of neighbor cursor
    vtkHyperTree* treeN = cursorN->GetTree();
    bool leafN = cursorN->IsLeaf();
    vtkIdType idN = cursorN->GetGlobalNodeIndex();
    int maskedN  = mask ? mask->GetValue( idN ) : 0;

    // In 3D masked and unmasked cells are handled differently:
    // . If cell is unmasked, and face neighbor is a masked leaf, or no such neighbor
    //   exists, then generate face.
    // . If cell is masked, and face neighbor exists and is an unmasked leaf, then
    //   generate face, breaking ties at same level. This ensures that faces between
    //   unmasked and masked cells will be generated once and only once.
    if ( ( ! masked && ( ! treeN || ( leafN && maskedN ) ) )
         ||
         ( masked && treeN && leafN && cursorN->GetLevel() < level && ! maskedN ) )
    {
      // Generate face with corresponding normal and offset
      this->AddFace( id, superCursor->GetOrigin(), superCursor->GetSize(),
                     VonNeumannOffsets3D[c], VonNeumannOrientations3D[c] );
    }
  } // c
}

//----------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::AddFace( vtkIdType inId,
                                               double* origin,
                                               double* size,
                                               int offset,
                                               unsigned int orientation )
{
  // Storage for point coordinates
  double pt[] = { 0., 0., 0. };

  // Storage for face vertex IDs
  vtkIdType ids[4];

  // First cell vertex is always at origin of cursor
  memcpy( pt, origin, 3 * sizeof( double ) );
  if ( offset )
  {
    // Offset point coordinate as needed
    pt[orientation] += size[orientation];
  }
  ids[0] = this->Points->InsertNextPoint( pt );

  // Create other face vertices depending on orientation
  unsigned int axis1 = orientation ? 0 : 1;
  unsigned int axis2 = orientation == 2 ? 1 : 2;
  pt[axis1] += size[axis1];
  ids[1] = this->Points->InsertNextPoint( pt );
  pt[axis2] += size[axis2];
  ids[2] = this->Points->InsertNextPoint( pt );
  pt[axis1] = origin[axis1];
  ids[3] = this->Points->InsertNextPoint( pt );

  // Insert next face
  vtkIdType outId = this->Cells->InsertNextCell( 4, ids );

  // Copy face data from that of the cell from which it comes
  this->OutData->CopyData( this->InData, inId, outId );
}

//----------------------------------------------------------------------------
void vtkAdaptiveDataSetSurfaceFilter::SetRenderer( vtkRenderer* ren )
{
  if ( ren != this->Renderer )
  {
    this->Renderer = ren;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkMTimeType vtkAdaptiveDataSetSurfaceFilter::GetMTime()
{
  // Check for minimal changes
  if ( this->Renderer )
  {
    vtkCamera* cam = this->Renderer->GetActiveCamera();
    if ( cam )
    {
      // Check & Update parallel projection
      bool para = (cam->GetParallelProjection()!=0);

      if ( this->ParallelProjection != para )
      {
        this->ParallelProjection = para;
        this->Modified();
      }

      // Check & Update renderer size
      int* sz = this->Renderer->GetSize();
      if (   this->LastRendererSize[0] != sz[0]
                || this->LastRendererSize[1] != sz[1] )
      {
        this->LastRendererSize[0] = sz[0];
        this->LastRendererSize[1] = sz[1];
        this->Modified();
      }

      // Check & Update camera focal point
      double* fp = cam->GetFocalPoint();
      if ( this->LastCameraFocalPoint[0] != fp[0]
        || this->LastCameraFocalPoint[1] != fp[1]
        || this->LastCameraFocalPoint[2] != fp[2] )
      {
        this->LastCameraFocalPoint[0] = fp[0];
        this->LastCameraFocalPoint[1] = fp[1];
        this->LastCameraFocalPoint[2] = fp[2];
        this->Modified();
      }

      // Check & Update camera scale
      double scale = cam->GetParallelScale();
      if( this->LastCameraParallelScale != scale)
      {
        this->LastCameraParallelScale = scale;
        this->Modified();
      }
    } // if ( cam )
  } // if ( this->Renderer )
  return this->Superclass::GetMTime();
}
