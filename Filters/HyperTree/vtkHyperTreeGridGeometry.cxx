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
#include "vtkHyperTreeGridCursor.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

static const unsigned int VonNeumannCursors3D[] = { 0, 1, 2, 4, 5, 6 };
static const unsigned int VonNeumannOrientations3D[]  = { 2, 1, 0, 0, 1, 2 };
static const unsigned int VonNeumannOffsets3D[]  = { 0, 0, 0, 1, 1, 1 };

vtkStandardNewMacro(vtkHyperTreeGridGeometry);

//-----------------------------------------------------------------------------
vtkHyperTreeGridGeometry::vtkHyperTreeGridGeometry()
{
  // Create storage for corners of leaf cells
  this->Points = vtkPoints::New();

  // Create storage for untructured leaf cells
  this->Cells = vtkCellArray::New();

  // Default dimension is 0
  this->Dimension = 0;

  // Default orientation is 0
  this->Orientation = 0;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridGeometry::~vtkHyperTreeGridGeometry()
{
  if ( this->Points )
  {
    this->Points->Delete();
    this->Points = nullptr;
  }

  if ( this->Cells )
  {
    this->Cells->Delete();
    this->Cells = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::PrintSelf( ostream& os, vtkIndent indent )
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
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridGeometry::FillOutputPortInformation( int,
                                                         vtkInformation* info )
{
  info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData" );
  return 1;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridGeometry::ProcessTrees( vtkHyperTreeGrid* input,
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

  // Retrieve useful grid parameters for speed of access
  this->Dimension = input->GetDimension();
  this->Orientation = input->GetOrientation();

  // Initialize output cell data
  this->InData = input->GetPointData();
  this->OutData = output->GetCellData();
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
    // Initialize new cursor at root of current tree
    vtkHyperTreeGridCursor* cursor;
    if ( this->Dimension == 3 )
    {
      // In 3 dimensions, von Neumann neighborhood information is needed
      cursor = input->NewVonNeumannSuperCursor( index );

      // Build geometry recursively
      this->RecursivelyProcessTree( cursor, mask );
    } // if ( this->Dimension == 3 )
    else
    {
      // Otherwise, geometric properties of the cells suffice
      cursor = input->NewGeometricCursor( index );

      // Build geometry recursively
      this->RecursivelyProcessTree( cursor, mask );
    } // else

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

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::RecursivelyProcessTree( vtkHyperTreeGridCursor* cursor,
                                                       vtkBitArray* mask )
{
  // Retrieve input grid
  vtkHyperTreeGrid* input = cursor->GetGrid();

  // Create geometry output if cursor is at leaf
  if ( cursor->IsLeaf() )
  {
    // Cursor is at leaf, process it depending on its dimension
    switch ( this->Dimension )
    {
      case 1:
        this->ProcessLeaf1D( cursor );
        break;
      case 2:
        this->ProcessLeaf2D( cursor, mask );
        break;
      case 3:
        this->ProcessLeaf3D( cursor, mask );
        break;
      default:
        return;
    } // switch ( this->Dimension )
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
      this->RecursivelyProcessTree( childCursor, mask );

      // Clean up
      childCursor->Delete();
      childCursor = nullptr;
    } // child
  } // else
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::ProcessLeaf1D( vtkHyperTreeGridCursor* cursor )
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
void vtkHyperTreeGridGeometry::ProcessLeaf2D( vtkHyperTreeGridCursor* cursor,
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
void vtkHyperTreeGridGeometry::ProcessLeaf3D( vtkHyperTreeGridCursor* superCursor,
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
void vtkHyperTreeGridGeometry::AddFace( vtkIdType inId,
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
