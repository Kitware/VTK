/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridToUnstructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridToUnstructuredGrid.h"

#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkHyperTreeGridToUnstructuredGrid);

//-----------------------------------------------------------------------------
vtkHyperTreeGridToUnstructuredGrid::vtkHyperTreeGridToUnstructuredGrid()
{
  this->Input = 0;
  this->Output = 0;

  this->InData = 0;
  this->OutData = 0;

  this->Points = 0;
  this->Cells = 0;

  this->Dimension = 0;
  this->CellSize = 0;

  this->Coefficients = 0;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridToUnstructuredGrid::~vtkHyperTreeGridToUnstructuredGrid()
{
  delete [] this->Coefficients;
  this->Coefficients = 0;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridToUnstructuredGrid::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  if( this->Input )
  {
    os << indent << "Input:\n";
    this->Input->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "Input: ( none )\n";
  }

  if( this->Output )
  {
    os << indent << "Output:\n";
    this->Output->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "Output: ( none )\n";
  }

  os << indent << "Dimension : " << this->Dimension << endl;
  os << indent << "CellSize : " << this->CellSize << endl;

  if ( this->Coefficients )
  {
    os << indent << "Coefficients : " << endl;
    for ( unsigned int i = 0; i < this->CellSize; ++ i )
    {
      os << indent;
      for ( unsigned int j = 0; j < this->Dimension; ++ j )
      {
        os << " " << this->Coefficients[i * this->Dimension + j];
      }
      os << endl;
    }
  }
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridToUnstructuredGrid::FillInputPortInformation( int, vtkInformation *info )
{
  info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid" );
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridToUnstructuredGrid::RequestData( vtkInformation*,
                                          vtkInformationVector** inputVector,
                                          vtkInformationVector* outputVector )
{
  // Get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject( 0 );
  vtkInformation *outInfo = outputVector->GetInformationObject( 0 );

  // Retrieve input and output
  this->Input =
    vtkHyperTreeGrid::SafeDownCast( inInfo->Get( vtkDataObject::DATA_OBJECT() ) );
  this->Output =
    vtkUnstructuredGrid::SafeDownCast( outInfo->Get( vtkDataObject::DATA_OBJECT() ) );

  delete [] this->Coefficients;
  this->Coefficients = 0;

  // Set instance variables needed for this conversion
  this->Dimension = this->Input->GetDimension();
  switch ( this->Dimension )
  {
    case 1:
      this->CellSize = 2;
      this->Coefficients = new unsigned int[2];
      this->Coefficients[0] = 0;
      this->Coefficients[1] = 1;
      break;
    case 2 :
      this->CellSize = 4;
      this->Coefficients = new unsigned int[8];
      for ( unsigned int i = 0; i < 4; ++ i )
      {
        div_t d = div( i, 2 );
        this->Coefficients[2 * i] = d.rem;
        this->Coefficients[2 * i + 1] = d.quot;
      }
      break;
    case 3 :
      this->CellSize = 8;
      this->Coefficients = new unsigned int[24];
      for ( unsigned int i = 0; i < 8; ++ i )
      {
        div_t d1 = div( i, 2 );
        div_t d2 = div( d1.quot, 2 );
        this->Coefficients[3 * i] = d1.rem;
        this->Coefficients[3 * i + 1] = d2.quot;
        this->Coefficients[3 * i + 2] = d2.rem;
      }
      break;
    default:
      vtkErrorMacro( "Incorrect tree dimension: "
                     << this->Dimension
                     << "." );
      return 0;
  }

  // Initialize output cell data
  this->InData =
    static_cast<vtkDataSetAttributes*>( this->Input->GetPointData() );
  this->OutData =
    static_cast<vtkDataSetAttributes*>( this->Output->GetCellData() );
  this->OutData->CopyAllocate( this->InData );

  // Convert hyper tree grid to unstructured grid
  this->ProcessTrees();

  // Clean up
  this->Input = 0;
  this->Output = 0;
  this->InData = 0;
  this->OutData = 0;

  this->UpdateProgress ( 1. );

  return 1;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridToUnstructuredGrid::ProcessTrees()
{
  // TODO: MTime on generation of this table.
  this->Input->GenerateSuperCursorTraversalTable();

  // Primal corner points
  this->Points = vtkPoints::New();
  this->Cells = vtkCellArray::New();

  // Iterate over all hyper trees
  vtkIdType index;
  vtkHyperTreeGrid::vtkHyperTreeIterator it;
  it.Initialize( this->Input );
  while ( it.GetNextTree( index ) )
  {
    // Storage for super cursors
    vtkHyperTreeGrid::vtkHyperTreeGridSuperCursor superCursor;

    // Initialize center cursor
    this->Input->InitializeSuperCursor( &superCursor, index );

    // Traverse and populate dual recursively
    this->RecursiveProcessTree( &superCursor );
  }

  // Set output geometry and topology
  this->Output->SetPoints( this->Points );
  switch ( this->CellSize )
  {
    case 2:
      this->Output->SetCells( VTK_LINE, this->Cells );
      break;
    case 4:
      this->Output->SetCells( VTK_QUAD, this->Cells );
      break;
    case 8:
      this->Output->SetCells( VTK_VOXEL, this->Cells );
      break;
    default:
      break;
  }

  this->Points->UnRegister( this );
  this->Points = 0;
  this->Cells->UnRegister( this );
  this->Cells = 0;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridToUnstructuredGrid::AddCell( vtkIdType inId,
                                                  double* origin,
                                                  double* size )
{
  // Generate 2^d points
  double pt[3];
  memcpy( pt, origin, 3 * sizeof(double) );

  // Storage for cell IDs
  vtkIdType ids[8];
  ids[0] = this->Points->InsertNextPoint( pt );

  for ( unsigned int i = 1; i < this->CellSize; ++ i )
  {
    for ( unsigned int j = 0; j < this->Dimension; ++ j )
    {
      pt[j] = origin[j] + this->Coefficients[i * this->Dimension + j] * size[j];
    }
    ids[i] = this->Points->InsertNextPoint( pt );
  }

  vtkIdType outId = this->Cells->InsertNextCell( this->CellSize, ids );
  this->OutData->CopyData( this->InData, inId, outId );
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridToUnstructuredGrid::RecursiveProcessTree( void* sc )
{
  // Get cursor at super cursor center
  vtkHyperTreeGrid::vtkHyperTreeGridSuperCursor* superCursor =
    static_cast<vtkHyperTreeGrid::vtkHyperTreeGridSuperCursor*>( sc );
  vtkHyperTreeGrid::vtkHyperTreeSimpleCursor* cursor = superCursor->GetCursor( 0 );

  if ( cursor->IsLeaf() )
  {
    // Cursor is a leaf, retrieve its global index
    vtkIdType inId = cursor->GetGlobalNodeIndex();
    // If leaf is masked, skip it
    if ( ! this->Input->GetMaterialMask()->GetValue( inId ) )
    {
      // Create cell
      this->AddCell( inId, superCursor->Origin, superCursor->Size );
    }
  }
  else
  {
     // If cursor is not at leaf, recurse to all children
    int numChildren = this->Input->GetNumberOfChildren();
    for ( int child = 0; child < numChildren; ++ child )
    {
      vtkHyperTreeGrid::vtkHyperTreeGridSuperCursor newSuperCursor;
      this->Input->InitializeSuperCursorChild( superCursor,&newSuperCursor, child );
      this->RecursiveProcessTree( &newSuperCursor );
    }
  }
}
