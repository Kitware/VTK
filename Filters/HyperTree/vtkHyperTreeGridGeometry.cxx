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
#include "vtkDoubleArray.h"
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
static const unsigned int EdgeIndices[3][2][4] = {
  {
    { 3,11,7,8 }, { 1,10,5,9 }
  },
  {
    { 0,9,4,8 }, { 2,10,6,11 }
  },
  {
    { 0,1,2,3 }, { 4,5,6,7 }
  }
};

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

  // Default interface values
  this->HasInterface = false;
  this->Normals = nullptr;
  this->Intercepts = nullptr;
  this->FaceIDs = vtkIdList::New();
  this->FacePoints = vtkPoints::New();
  this->FacePoints->SetNumberOfPoints( 4 );
  this->FacesA = vtkIdTypeArray::New();
  this->FacesA->SetNumberOfComponents( 2 );
  this->FacesB = vtkIdTypeArray::New();
  this->FacesB->SetNumberOfComponents( 2 );
  this->FaceScalarsA = vtkDoubleArray::New();
  this->FaceScalarsA->SetNumberOfTuples( 4 );
  this->FaceScalarsB = vtkDoubleArray::New();
  this->FaceScalarsB->SetNumberOfTuples( 4 );
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

  if ( this->FacePoints )
  {
    this->FacePoints->Delete();
    this->FacePoints = nullptr;
  }

  if ( this->FaceIDs )
  {
    this->FaceIDs->Delete();
    this->FaceIDs = nullptr;
  }

  if ( this->FacesA )
  {
    this->FacesA->Delete();
    this->FacesA = nullptr;
  }

  if ( this->FacesB )
  {
    this->FacesB->Delete();
    this->FacesB = nullptr;
  }

  if ( this->FaceScalarsA )
  {
    this->FaceScalarsA->Delete();
    this->FaceScalarsA = nullptr;
  }

  if ( this->FaceScalarsB )
  {
    this->FaceScalarsB->Delete();
    this->FaceScalarsB = nullptr;
  }

}

//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "Dimension: " << this->Dimension << endl;
  os << indent << "Orientation: " << this->Orientation << endl;
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
  os << indent << "HasInterface: " << this->HasInterface << endl;
  if( this->Normals )
  {
    os << indent << ":\n";
    this->Normals->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "Normals: ( none )\n";
  }
  if( this->Intercepts )
  {
    os << indent << ":\n";
    this->Intercepts->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "Intercepts: ( none )\n";
  }
  if( this->FacePoints )
  {
    os << indent << ":\n";
    this->FacePoints->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "FacePoints: ( none )\n";
  }
  if( this->FaceIDs )
  {
    os << indent << ":\n";
    this->FaceIDs->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "FaceIDs: ( none )\n";
  }
  os << indent << "EdgesA:";
  for ( unsigned int i = 0; i < 12; ++ i )
  {
    os << " " << EdgesA[i];
  }
  os << endl;
  os << indent << "EdgesB:";
  for ( unsigned int i = 0; i < 12; ++ i )
  {
    os << " " << EdgesB[i];
  }
  os << endl;
  if( this->FacesA )
  {
    os << indent << ":\n";
    this->FacesA->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "FacesA: ( none )\n";
  }
  if( this->FacesB )
  {
    os << indent << ":\n";
    this->FacesB->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "FacesB: ( none )\n";
  }
  if( this->FaceScalarsA )
  {
    os << indent << ":\n";
    this->FaceScalarsA->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "FaceScalarsA: ( none )\n";
  }
  if( this->FaceScalarsB )
  {
    os << indent << ":\n";
    this->FaceScalarsB->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "FaceScalarsB: ( none )\n";
  }
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

  // Retrieve interface data when relevant
  this->HasInterface = input->GetHasInterface();
  if ( this->HasInterface )
  {
    this->Normals
      = vtkDoubleArray::SafeDownCast( this->InData->GetArray( input->GetInterfaceNormalsName() ) );
    this->Intercepts
      = vtkDoubleArray::SafeDownCast( this->InData->GetArray( input->GetInterfaceInterceptsName() ) );
  } // this->HasInterface

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

  // Reset interface variables if needed
  if ( this->HasInterface )
  {
    size_t int12sz = 12 * sizeof( vtkIdType );
    memset( this->EdgesA, -1,  int12sz );
    memset( this->EdgesB, -1,  int12sz );
    this->FacesA->Reset();
    this->FacesB->Reset();
  } // if ( this->HasInterface )

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

  // Default cell type is pure
  double type = 2.;

  // Reset interface variables if needed
  if ( this->HasInterface )
  {
    size_t int12sz = 12 * sizeof( vtkIdType );
    memset( this->EdgesA, -1,  int12sz );
    memset( this->EdgesB, -1,  int12sz );
    this->FacesA->Reset();
    this->FacesB->Reset();

    // Retrieve intercept type
    type = this->Intercepts->GetComponent( id, 2 );
  } // if ( this->HasInterface )

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

    // In 3D masked and unmasked cells are handled differently
    if ( masked )
    {
      // If cell is masked, and face neighbor exists and is an unmasked leaf,
      // generate face, breaking ties at same level. This ensures that faces
      // between unmasked and masked cells will be generated once and only once.
      if ( treeN && leafN && cursorN->GetLevel() < level && ! maskedN )
      {
        // Generate face with corresponding normal and offset
        this->AddFace( id, superCursor->GetOrigin(), superCursor->GetSize(),
                       VonNeumannOffsets3D[c], VonNeumannOrientations3D[c] );
      }
    } // if ( masked )
    else
    {
      // If cell is unmasked, and face neighbor is a masked leaf, or no
      // such neighbor exists, generate face except in some interface cases.
      bool addFace = ! treeN || ( leafN && maskedN );
      bool create = true;
      if ( ! addFace && type < 2. )
      {
        // Mixed cells must be handled but not created
        create  = false;
        addFace = true;
      }
      if ( ( ! addFace || ! create ) && this->HasInterface && leafN )
      {
        // Face must be created if neighbor is a mixed cell
        create  = this->Intercepts->GetComponent( idN, 2 ) < 2.;
        addFace |= create;
      }
      if ( addFace )
      {
        // Generate or handle face with corresponding normal and offset
        this->AddFace( id, superCursor->GetOrigin(), superCursor->GetSize(),
                       VonNeumannOffsets3D[c], VonNeumannOrientations3D[c],
                       create );
      } // if ( addFace )
    } // else
  } // c

  // Handle interfaces separately
  if ( this->HasInterface )
  {
    // Create face A when its edges are present
    vtkIdType nA = this->FacesA->GetNumberOfTuples();
    if ( nA > 0 )
    {
      this->FaceIDs->Reset();
      vtkIdType i0 = 0;
      vtkIdType edge0[2];
      this->FacesA->GetTypedTuple( i0, edge0 );
      this->FaceIDs->InsertNextId( this->EdgesA[edge0[1]] );
      while ( edge0[0] != edge0[1] )
      {
        // Iterate over edges of face A
        for ( vtkIdType i = 0; i < nA; ++ i )
        {
          // Seek next edge then break out from loop
          vtkIdType edge[2];
          this->FacesA->GetTypedTuple( i, edge );
          if( i0 != i )
          {
            if ( edge[0] == edge0[1] )
            {
              edge0[1] = edge[1];
              i0 = i;
              break;
            }
            if ( edge[1] == edge0[1] )
            {
              edge0[1] = edge[0];
              i0 = i;
              break;
            }
          } // if ( i != i0 )
        } // nA
        this->FaceIDs->InsertNextId( this->EdgesA[edge0[1]] );
      } // while ( edge0[0] != edge0[1] )

      // Create new face
      vtkIdType outId = this->Cells->InsertNextCell( this->FaceIDs );

      // Copy face data from that of the cell from which it comes
      this->OutData->CopyData( this->InData, id, outId );
    } // if ( nA > 0 )

    // Create face B when its vertices are present
    vtkIdType nB = this->FacesB->GetNumberOfTuples();
    if ( nB > 0 )
    {
      this->FaceIDs->Reset();
      int i0 = 0;
      vtkIdType edge0[2];
      this->FacesB->GetTypedTuple( i0, edge0 );
      this->FaceIDs->InsertNextId( this->EdgesB[edge0[1]] );
      while ( edge0[0] != edge0[1] )
      {
        // Iterate over faces B
        for ( vtkIdType i = 0; i < nB; ++ i )
        {
          // Seek next edge then break out from loop
          vtkIdType edge[2];
          this->FacesB->GetTypedTuple( i, edge );
          if ( i0 != i )
          {
            if ( edge[0] == edge0[1] )
            {
              edge0[1] = edge[1];
              i0 = i;
              break;
            }
            if ( edge[1] == edge0[1] )
            {
              edge0[1] = edge[0];
              i0 = i;
              break;
            }
          } // if ( i0 != i )
        } // nB
        this->FaceIDs->InsertNextId(this->EdgesB[edge0[1]]);
      } // while ( edge0[0] != edge0[1] )

      // Create new face
      vtkIdType outId = this->Cells->InsertNextCell( this->FaceIDs );

      // Copy face data from that of the cell from which it comes
      this->OutData->CopyData( this->InData, id, outId );
    } // if ( nB > 0 )
  } // if ( this->HasInterface )
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::AddFace( vtkIdType inId,
                                        double* origin,
                                        double* size,
                                        int offset,
                                        unsigned int orientation,
                                        bool create )
{
  // First cell vertex is always at origin of cursor
  double pt[3];
  memcpy( pt, origin, 3 * sizeof( double ) );
  if ( offset )
  {
    // Offset point coordinate as needed
    pt[orientation] += size[orientation];
  }

  // Storage for face vertex IDs
  vtkIdType ids[4];
  unsigned int nPts = 4;

  // Keep track of face axes
  unsigned int axis1 = orientation ? 0 : 1;
  unsigned int axis2 = orientation == 2 ? 1 : 2;

  if ( this->HasInterface )
  {
    // Retrieve intercept tuple and type
    double* inter = this->Intercepts->GetTuple( inId );
    double type = inter[2];

    // Distinguish cases depending on intercept type
    if ( type < 2 )
    {
      // Create interface intersection points
      this->FacePoints->SetPoint( 0, pt );
      pt[axis1] += size[axis1];
      this->FacePoints->SetPoint( 1, pt );
      pt[axis2] += size[axis2];
      this->FacePoints->SetPoint( 2, pt );
      pt[axis1] = origin[axis1];
      this->FacePoints->SetPoint( 3, pt );

      // Create interface intersection faces
      double coordsA[3];
      double* normal = this->Normals->GetTuple( inId );
      for ( vtkIdType pId = 0; pId < 4; ++ pId )
      {
        // Retrieve vertex coordinates
        this->FacePoints->GetPoint( pId, coordsA );

        // Set face scalars
        if ( type != 1. )
        {
          double val = inter[0]
            + normal[0] * coordsA[0]
            + normal[1] * coordsA[1]
            + normal[2] * coordsA[2];
          this->FaceScalarsA->SetTuple1( pId, val );
        } // if ( type != 1. )
        if ( type != -1. )
        {
          double val = inter[1]
            + normal[0] * coordsA[0]
            + normal[1] * coordsA[1]
            + normal[2] * coordsA[2];
          this->FaceScalarsB->SetTuple1( pId, val );
        } // if ( type != -1. )
      } // p

      // Storage for points
      double coordsB[3];
      double coordsC[3];
      nPts = 0;

      // Distinguish between relevant types
      if ( type == 1 )
      {
        // Take negative values in B
        vtkIdType pair[2];
        unsigned int indPair = 0;

        // Loop over face vertices
        for( int p = 0; p < 4; ++ p )
        {
          // Retrieve vertex coordinates
          this->FacePoints->GetPoint( p, coordsA );

          // Retrieve vertex scalars
          double A = this->FaceScalarsB->GetTuple1( p );
          double B = this->FaceScalarsB->GetTuple1( ( p + 1 ) % 4 );

          // Add point when necessary
          if ( create && A <= 0. )
          {
            ids[nPts] = this->Points->InsertNextPoint( coordsA );
            ++ nPts;
          }
          if ( A * B < 0 )
          {
            unsigned int i = EdgeIndices[orientation][offset][p];
            if ( this->EdgesB[i] == -1 )
            {
              //Compute barycenter of A and B
              this->FacePoints->GetPoint( ( p + 1 ) % 4, coordsB );
              for( int j = 0; j < 3 ;++ j )
              {
                coordsC[j] = ( B * coordsA[j] - A * coordsB[j] ) / ( B - A );
              }
              this->EdgesB[i] = this->Points->InsertNextPoint( coordsC );
            } // if ( this->EdgesB[i] == -1 )

            // Update points
            ids[nPts] = this->EdgesB[i];
            ++ nPts;
            if( indPair )
            {
              pair[1] = i;
            }
            else
            {
              pair[0] = i;
            };
            ++ indPair;
          } // if ( A * B < 0 )
        } // p

        // Insert pair only if it makes sense
        if ( indPair == 2 )
        {
          this->FacesB->InsertNextTypedTuple( pair );
        }
      } // if (type = 1 )

      else if( ! type )
      {
        // Take positive values in A
        vtkIdType pairA[2];
        unsigned int indPairA = 0;

        // Take negative values in B
        vtkIdType pairB[2];
        unsigned int indPairB = 0;

        // Loop over face vertices
        for( int p = 0; p < 4; ++ p )
        {
          // Retrieve vertex coordinates
          this->FacePoints->GetPoint( p, coordsA );

          // Retrieve vertex scalars
          double A1 = this->FaceScalarsA->GetTuple1( p );
          double B1 = this->FaceScalarsA->GetTuple1( ( p + 1 ) % 4 );
          double A2 = this->FaceScalarsB->GetTuple1( p );
          double B2 = this->FaceScalarsB->GetTuple1( ( p + 1 ) % 4 );

          // Add point when necessary
          if ( create && A1 >= 0. && A2 <= 0. )
          {
            ids[nPts] = this->Points->InsertNextPoint( coordsA );
            ++ nPts;
          }
          if ( A1 < 0. && A1 * B1 < 0. )
          {
            unsigned int i = EdgeIndices[orientation][offset][p];
            if ( this->EdgesA[i] == -1 )
            {
              //Compute barycenter of A and B
              this->FacePoints->GetPoint( ( p + 1 ) % 4, coordsB );
              for( int j = 0; j < 3 ;++ j )
              {
                coordsC[j] = ( B1 * coordsA[j] - A1 * coordsB[j] ) / ( B1 - A1 );
              }
              this->EdgesA[i] = this->Points->InsertNextPoint( coordsC );
            } // if ( this->EdgesB[i] == -1 )

            // Update points
            ids[nPts] = this->EdgesA[i];
            ++ nPts;
            if( indPairA )
            {
              pairA[1] = i;
            }
            else
            {
              pairA[0] = i;
            };
            ++ indPairA;
          } // if ( A1 < 0. && A1 * B1 < 0. )
          if ( A2* B2 < 0. )
          {
            unsigned int i = EdgeIndices[orientation][offset][p];
            if ( this->EdgesA[i] == -1 )
            {
              //Compute barycenter of A and B
              this->FacePoints->GetPoint( ( p + 1 ) % 4, coordsB );
              for( int j = 0; j < 3 ;++ j )
              {
                coordsC[j] = ( B2 * coordsA[j] - A2 * coordsB[j] ) / ( B2 - A2 );
              }
              this->EdgesB[i] = this->Points->InsertNextPoint( coordsC );
            } // if ( this->EdgesA[i] == -1 )

            // Update points
            ids[nPts] = this->EdgesB[i];
            ++ nPts;
            if( indPairB )
            {
              pairB[1] = i;
            }
            else
            {
              pairB[0] = i;
            };
            ++ indPairB;
          } // if ( A2 * B2 < 0. )
          if ( A1 > 0. && A1 * B1 < 0. )
          {
            unsigned int i = EdgeIndices[orientation][offset][p];
            if ( this->EdgesA[i] == -1 )
            {
              //Compute barycenter of A and B
              this->FacePoints->GetPoint( ( p + 1 ) % 4, coordsB );
              for( int j = 0; j < 3 ;++ j )
              {
                coordsC[j] = ( B1 * coordsA[j] - A1 * coordsB[j] ) / ( B1 - A1 );
              }
              this->EdgesA[i] = this->Points->InsertNextPoint( coordsC );
            } // if ( this->EdgesA[i] == -1 )

            // Update points
            ids[nPts] = this->EdgesA[i];
            ++ nPts;
            if( indPairA )
            {
              pairA[1] = i;
            }
            else
            {
              pairA[0] = i;
            };
            ++ indPairA;
          } // if ( A1 > 0. && A1 * B1 < 0. )
        } // p

        // Insert pairs only if it makes sense
        if ( indPairA == 2 )
        {
          this->FacesA->InsertNextTypedTuple( pairA );
        }
        if ( indPairB == 2 )
        {
          this->FacesB->InsertNextTypedTuple( pairB );
        }
      } // else if( ! type )
      else if ( type == -1. )
      {
        // Take positive values in A
        vtkIdType pair[] = { -1, -1 };
        unsigned int indPair = 0;

        // Loop over face vertices
        for( int p = 0; p < 4; ++ p )
        {
          // Retrieve vertex coordinates
          this->FacePoints->GetPoint( p, coordsA );

          // Retrieve vertex scalars
          double A = this->FaceScalarsA->GetTuple1( p );
          double B = this->FaceScalarsA->GetTuple1( ( p + 1 ) % 4 );

          // Add point when necessary
          if ( create && A >= 0. )
          {
            ids[nPts] = this->Points->InsertNextPoint( coordsA );
            ++ nPts;
          }
          if ( A * B < 0. )
          {
            unsigned int i = EdgeIndices[orientation][offset][p];
            if ( this->EdgesA[i] == -1 )
            {
              //Compute barycenter of A and B
              this->FacePoints->GetPoint( ( p + 1 ) % 4, coordsB );
              for( int j = 0; j < 3 ;++ j )
              {
                coordsC[j] = ( B * coordsA[j] - A * coordsB[j] ) / ( B - A );
              }
              this->EdgesA[i] = this->Points->InsertNextPoint( coordsC );
            } // if ( this->EdgesB[i] == -1 )

            // Update points
            ids[nPts] = this->EdgesA[i];
            ++ nPts;
            if( indPair )
            {
              pair[1] = i;
            }
            else
            {
              pair[0] = i;
            };
            ++ indPair;
          } // if ( A * B < 0. )
        } // p

        // Insert pair only if it makes sense
        if ( indPair == 2 )
        {
          this->FacesA->InsertNextTypedTuple( pair );
        }
      } // else if ( type == -1. )
    } // if ( type < 2 )
    else
    {
      // Create quadrangle vertices depending on orientation
      ids[0] = this->Points->InsertNextPoint( pt );
      pt[axis1] += size[axis1];
      ids[1] = this->Points->InsertNextPoint( pt );
      pt[axis2] += size[axis2];
      ids[2] = this->Points->InsertNextPoint( pt );
      pt[axis1] = origin[axis1];
      ids[3] = this->Points->InsertNextPoint( pt );
    } // else
  } // if ( this->HasInterface )
  else
  {
    // Create quadrangle vertices depending on orientation
    ids[0] = this->Points->InsertNextPoint( pt );
    pt[axis1] += size[axis1];
    ids[1] = this->Points->InsertNextPoint( pt );
    pt[axis2] += size[axis2];
    ids[2] = this->Points->InsertNextPoint( pt );
    pt[axis1] = origin[axis1];
    ids[3] = this->Points->InsertNextPoint( pt );
  } // else

  // Insert next face if needed
  if ( create )
  {
    // Create cell and corresponding ID
    vtkIdType outId = this->Cells->InsertNextCell( nPts, ids );

    // Copy face data from that of the cell from which it comes
    this->OutData->CopyData( this->InData, inId, outId );
  } // if ( create )
}
