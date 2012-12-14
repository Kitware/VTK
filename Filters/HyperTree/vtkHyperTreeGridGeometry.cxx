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
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkHyperTreeGridGeometry);

static int vtkSuperCursorFaceIndices[] = { -1, 1, -3, 3, -9, 9 };

//-----------------------------------------------------------------------------
vtkHyperTreeGridGeometry::vtkHyperTreeGridGeometry()
{
  this->Points = 0;
  this->Cells = 0;
  this->Input = 0;
  this->Output = 0;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridGeometry::~vtkHyperTreeGridGeometry()
{
  if ( this->Points )
    {
    this->Points->Delete();
    this->Points = 0;
    }
  if ( this->Cells )
    {
    this->Cells->Delete();
    this->Cells = 0;
    }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  if ( this->Input )
    {
    os << indent << "Input:\n";
    this->Input->PrintSelf( os, indent.GetNextIndent() );
    }
  else
    {
    os << indent << "Input: (none)\n";
    }
  if ( this->Output )
    {
    os << indent << "Output:\n";
    this->Output->PrintSelf( os, indent.GetNextIndent() );
    }
  else
    {
    os << indent << "Output: (none)\n";
    }
  if ( this->Points )
    {
    os << indent << "Points:\n";
    this->Points->PrintSelf( os, indent.GetNextIndent() );
    }
  else
    {
    os << indent << "Points: (none)\n";
    }
  if ( this->Cells )
    {
    os << indent << "Cells:\n";
    this->Cells->PrintSelf( os, indent.GetNextIndent() );
    }
  else
    {
    os << indent << "Cells: (none)\n";
    }
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridGeometry::FillInputPortInformation( int, vtkInformation *info )
{
  info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid" );
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridGeometry::RequestData( vtkInformation*,
                                           vtkInformationVector** inputVector,
                                           vtkInformationVector* outputVector )
{
  // Get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject( 0 );
  vtkInformation *outInfo = outputVector->GetInformationObject( 0 );

  // Retrieve input and output
  this->Input = vtkHyperTreeGrid::SafeDownCast( inInfo->Get( vtkDataObject::DATA_OBJECT() ) );
  this->Output= vtkPolyData::SafeDownCast( outInfo->Get( vtkDataObject::DATA_OBJECT() ) );

  // Ensure that primal grid API is used for hyper trees
  int inputDualFlagIsOn = this->Input->GetUseDualGrid();
  if ( inputDualFlagIsOn )
    {
    this->Input->SetUseDualGrid( 0 );
    }

  // Initialize output cell data
  vtkCellData* inCD = this->Input->GetCellData();
  vtkCellData* outCD = this->Output->GetCellData();
  outCD->CopyAllocate( inCD );

  // Extract geometry from hyper tree grid
  this->ProcessTrees();

  // Return duality flag of input to its original state
  if ( inputDualFlagIsOn )
    {
    this->Input->SetUseDualGrid( 1 );
    }

  // Clean up
  this->Input = 0;
  this->Output = 0;

  this->UpdateProgress ( 1. );
  return 1;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::ProcessTrees()
{
  // TODO: MTime on generation of this table.
  this->Input->GenerateSuperCursorTraversalTable();

  // Primal corner points
  this->Points = vtkPoints::New();
  this->Cells = vtkCellArray::New();

  // Iterate over all hyper trees
  unsigned int* gridSize = this->Input->GetGridSize();
  for ( unsigned int k = 0; k < gridSize[2]; ++ k )
    {
    for ( unsigned int j = 0; j < gridSize[1]; ++ j )
      {
        for ( unsigned int i = 0; i < gridSize[0]; ++ i )
        {
        // Storage for super cursors
        vtkHyperTreeGridSuperCursor superCursor;

        // Initialize center cursor
        this->Input->InitializeSuperCursor( &superCursor, i, j, k );

        // Traverse and populate dual recursively
        this->RecursiveProcessTree( &superCursor );
        } // i
      } // j
    } // k

  // Set output geometry and topology
  this->Output->SetPoints( this->Points );
  if ( this->Input->GetDimension() == 1  )
    {
    this->Output->SetLines( this->Cells );
    }
  else
    {
    this->Output->SetPolys( this->Cells );
    }
}


//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::AddFace( vtkIdType inId,
                                        double* origin, double* size,
                                        int offset, int orientation )
{
  // Initialize points
  double pt[3];
  for ( int i = 0; i < 3; ++ i )
    {
    pt[i] = origin[i];
    }
  if ( offset )
    {
    pt[orientation] += size[orientation];
    }

  // Storage for face vertices
  vtkIdType ids[4];

  // Create origin vertex
  ids[0] = this->Points->InsertNextPoint( pt );

  // Create other face vertices depending on orientation
  int axis1 = 0;
  int axis2 = 0;
  switch ( orientation )
    {
    case 0:
      axis1 = 1;
      axis2 = 2;
      break;
    case 1:
      axis1 = 0;
      axis2 = 2;
      break;
    case 2:
      axis1 = 0;
      axis2 = 1;
      break;
    }
  pt[axis1] += size[axis1];
  ids[1] = this->Points->InsertNextPoint( pt );
  pt[axis2] += size[axis2];
  ids[2] = this->Points->InsertNextPoint( pt );
  pt[axis1] = origin[axis1];
  ids[3] = this->Points->InsertNextPoint( pt );

  // Insert face
  vtkIdType outId = this->Cells->InsertNextCell( 4, ids );

  // Copy face data from that of the cell from which it comes
  this->Output->GetCellData()->CopyData( this->Input->GetCellData(), inId, outId );
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::RecursiveProcessTree( vtkHyperTreeGridSuperCursor* superCursor )
{
  // Get cursor at super cursor center
  vtkHyperTreeSimpleCursor* cursor = superCursor->GetCursor( 0 );

  // If cursor is not at leaf, recurse to all children
  if ( ! cursor->IsLeaf() )
    {
    int numChildren = this->Input->GetNumberOfChildren();
    for ( int child = 0; child < numChildren; ++ child )
      {
      vtkHyperTreeGridSuperCursor newSuperCursor;
      this->Input->InitializeSuperCursorChild( superCursor, &newSuperCursor, child );
      this->RecursiveProcessTree( &newSuperCursor );
      } // child
    return;
    } // if ( ! cursor->IsLeaf() )

  // Cell at cursor center is a leaf, retrieve its global index
  vtkIdType inId = cursor->GetGlobalLeafIndex();

  // Create the outer geometry, depending on the dimension of the grid
  switch (  this->Input->GetDimension() )
    {
    case 1:
      // In 1D the geometry is composed of edges
      vtkIdType ids[2];
      ids[0] = this->Points->InsertNextPoint( superCursor->Origin );
      double pt[3];
      pt[0] = superCursor->Origin[0] + superCursor->Size[0];
      pt[1] = superCursor->Origin[1];
      pt[2] = superCursor->Origin[2];
      ids[1] = this->Points->InsertNextPoint( pt );
      this->Cells->InsertNextCell( 2, ids );

      break; // case 1
    case 2:
      // In 2D all faces are generated
      this->AddFace( inId, superCursor->Origin, superCursor->Size, 0, 2 );

      break; // case 2
    case 3:
      // In 3D masked and unmasked cells are handles differently
      if ( this->Input->GetMaterialMask()->GetTuple1( inId ) )
        {
        // Cell is masked, check if any of the face neighbors are unmasked
        for ( int f = 0; f < 6; ++ f )
          {
          // Retrieve face neighbor cursor and break ties at same level
          cursor = superCursor->GetCursor( vtkSuperCursorFaceIndices[f] );
          if ( cursor->GetTree() 
               && cursor->IsLeaf()
               && cursor->GetLevel() < superCursor->GetCursor( 0 )->GetLevel() )
            {
            // Retrieve neighbor leaf which exists
            int id = cursor->GetGlobalLeafIndex();

            // Check if this correspond to an umasked cell
            if ( ! this->Input->GetMaterialMask()->GetTuple1( id ) )
              {
              // Neighbor cell is masked, generate boundary face
              div_t d = div( f, 2 );
              this->AddFace( id,
                             superCursor->Origin, superCursor->Size,
                             d.rem, d.quot );
              }
            }
          } // f
        return;
        } //  if ( this->Input->GetMaterialMask()->GetTuple1( inId ) )

      // Cell is not masked, handle its boundary faces as needed
      for ( int f = 0; f < 6; ++ f )
        {
        // Retrieve face neighbor cursor
        cursor = superCursor->GetCursor( vtkSuperCursorFaceIndices[f] );

        // Boundary faces, or faces shared by a masked cell, must be created
        if ( ! cursor->GetTree()
             ||
             ( cursor->IsLeaf()
               && this->Input->GetMaterialMask()->GetTuple1( cursor->GetGlobalLeafIndex() ) ) )
          {
          div_t d = div( f, 2 );
          this->AddFace( inId,
                         superCursor->Origin, superCursor->Size,
                         d.rem, d.quot );

          }
        } // f

      break; // case 3
    } // switch (  this->Input->GetDimension() )
}
