/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridSource.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUniformHyperTreeGridSource.h"

#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUniformHyperTreeGrid.h"

vtkStandardNewMacro(vtkUniformHyperTreeGridSource);

//----------------------------------------------------------------------------
vtkUniformHyperTreeGridSource::vtkUniformHyperTreeGridSource()
{
}

//----------------------------------------------------------------------------
vtkUniformHyperTreeGridSource::~vtkUniformHyperTreeGridSource()
{
}

//-----------------------------------------------------------------------------
void vtkUniformHyperTreeGridSource::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//----------------------------------------------------------------------------
int vtkUniformHyperTreeGridSource::RequestData( vtkInformation*,
                                         vtkInformationVector**,
                                         vtkInformationVector* outputVector )
{
  // Retrieve the output
  vtkDataObject* outputDO = vtkDataObject::GetData( outputVector, 0 );
  vtkUniformHyperTreeGrid* output = vtkUniformHyperTreeGrid::SafeDownCast( outputDO );
  if ( ! output )
  {
    return 0;
  }

  output->Initialize();

  vtkPointData* outData = output->GetPointData();

  this->LevelBitsIndexCnt.clear();
  this->LevelBitsIndexCnt.push_back(0);

  // When using descriptor-based definition, initialize descriptor parsing
  if ( this->UseDescriptor )
  {
    // Calculate refined block size
    this->BlockSize = this->BranchFactor;
    for ( unsigned int i = 1; i < this->Dimension; ++ i )
    {
      this->BlockSize *= this->BranchFactor;
    }

    if ( ! this->DescriptorBits && ! this->InitializeFromStringDescriptor() )
    {
      return 0;
    }
    else if ( this->DescriptorBits && ! this->InitializeFromBitsDescriptor() )
    {
      return 0;
    }
  } // if this->UseDescriptor

  // Set straightforward grid parameters
  output->SetTransposedRootIndexing( this->TransposedRootIndexing );
  output->SetDimension( this->Dimension );
  output->SetOrientation( this->Orientation );
  output->SetBranchFactor( this->BranchFactor );
//JBDEL2  output->SetMaterialMaskIndex( this->LevelZeroMaterialIndex );

  //  Set parameters that depend on dimension
  switch ( this->Dimension )
  {
    case 1:
    {
      // Set 1D grid size depending on orientation
      unsigned int axis = this->Orientation;
      unsigned int gs[] = { 1, 1, 1 };
      unsigned n = this->GridSize[axis];
      gs[axis] = n;
      output->SetGridSize( gs );

/*
      // Create null coordinate array for non-existent dimensions
      ++ n;
      vtkNew<vtkDoubleArray> zeros;
      zeros->SetNumberOfValues( 2 );
      zeros->SetValue( 0, 0. );
      zeros->SetValue( 1, 0. );

      // Create coordinate array for existent dimension
      vtkNew<vtkDoubleArray> coords;
      coords->SetNumberOfValues( n );
      for ( unsigned int i = 0; i < n; ++ i )
      {
        double coord = this->Origin[axis] + this->GridScale[axis] * static_cast<double>( i );
        coords->SetValue( i, coord );
      } // i
*/
      // Assign coordinates
      switch ( axis )
      {
        case 0:
          output->SetGridScale( this->GridScale[axis], 0., 0. );
/*
          output->SetXCoordinates( coords );
          output->SetYCoordinates( zeros );
          output->SetZCoordinates( zeros );
*/
          break;
        case 1:
          output->SetGridScale( 0., this->GridScale[axis], 0. );
/*
          output->SetXCoordinates( zeros );
          output->SetYCoordinates( coords );
          output->SetZCoordinates( zeros );
*/
          break;
        case 2:
          output->SetGridScale( 0., 0., this->GridScale[axis] );
/*
          output->SetXCoordinates( zeros );
          output->SetYCoordinates( zeros );
          output->SetZCoordinates( coords );
*/
          break;
      } // switch ( axis )
/*
      zeros->SetValue( 1, 0. );
*/
    } // case 1
      break;
    case 2:
    {
      // Set grid size depending on orientation
      unsigned int n[3];
      memcpy( n, this->GridSize, 3 * sizeof( unsigned int ) );
      n[this->Orientation] = 1;
      output->SetGridSize( n );

/*
      // Create null coordinate array for non-existent dimension
      vtkNew<vtkDoubleArray> zeros;
      zeros->SetNumberOfValues( 2 );
      zeros->SetValue( 0, 0. );
      zeros->SetValue( 1, 0. );

      // Create null coordinate arrays for existent dimensions
*/
      unsigned int axis1 = ( this->Orientation + 1 ) % 3;
/*
      vtkNew<vtkDoubleArray> coords1;
      unsigned int n1 = this->GridSize[axis1] + 1;
      coords1->SetNumberOfValues( n1 );
      for ( unsigned int i = 0; i < n1; ++ i )
      {
        double coord = this->Origin[axis1] + this->GridScale[axis1] * static_cast<double>( i );
        coords1->SetValue( i, coord );
      } // i
*/
      unsigned int axis2 = ( this->Orientation + 2 ) % 3;
/*
      vtkNew<vtkDoubleArray> coords2;
      unsigned int n2 = this->GridSize[axis2] + 1;
      coords2->SetNumberOfValues( n2 );
      for ( unsigned int i = 0; i < n2; ++ i )
      {
        double coord = this->Origin[axis2] + this->GridScale[axis2] * static_cast<double>( i );
        coords2->SetValue( i, coord );
      } // i
*/

      // Assign coordinates
      switch ( this->Orientation )
      {
        case 0:
          output->SetGridScale( 0., this->GridScale[axis1], this->GridScale[axis2] );
/*
          output->SetXCoordinates( zeros );
          output->SetYCoordinates( coords1 );
          output->SetZCoordinates( coords2 );
*/
          break;
        case 1:
          output->SetGridScale( this->GridScale[axis2], 0., this->GridScale[axis1] );
/*
          output->SetXCoordinates( coords2 );
          output->SetYCoordinates( zeros );
          output->SetZCoordinates( coords1 );
*/
          break;
        case 2:
          output->SetGridScale( this->GridScale[axis1], this->GridScale[axis2], 0. );
/*
          output->SetXCoordinates( coords1 );
          output->SetYCoordinates( coords2 );
          output->SetZCoordinates( zeros );
*/
          break;
      } // switch ( this->Orientation )
    } // case 2
      break;
    case 3:
    {
      // Set grid size
      output->SetGridSize( this->GridSize );

/*
      // Create x-coordinates array
      vtkNew<vtkDoubleArray> coordsx;
      unsigned int nx = this->GridSize[0] + 1;
      coordsx->SetNumberOfValues( nx );
      for ( unsigned int i = 0; i < nx; ++ i )
      {
        double coord = this->Origin[0] + this->GridScale[0] * static_cast<double>( i );
        coordsx->SetValue( i, coord );
      } // i

      // Create y-coordinates array
      vtkNew<vtkDoubleArray> coordsy;
      unsigned int ny = this->GridSize[1] + 1;
      coordsy->SetNumberOfValues( ny );
      for ( unsigned int i = 0; i < ny; ++ i )
      {
        double coord = this->Origin[1] + this->GridScale[1] * static_cast<double>( i );
        coordsy->SetValue( i, coord );
      } // i

      // Create z-coordinates array
      vtkNew<vtkDoubleArray> coordsz;
      unsigned int nz = this->GridSize[2] + 1;
      coordsz->SetNumberOfValues( nz );
      for ( unsigned int i = 0; i < nz; ++ i )
      {
        double coord = this->Origin[2] + this->GridScale[2] * static_cast<double>( i );
        coordsz->SetValue( i, coord );
      } // i

      // Assign coordinates
      output->SetXCoordinates( coordsx );
      output->SetYCoordinates( coordsy );
      output->SetZCoordinates( coordsz );
*/
      output->SetGridScale( this->GridScale[0], this->GridScale[1], this->GridScale[2] );
      break;
    } // case 3
    default:
      vtkErrorMacro(<<"Unsupported dimension: "
                    << this->Dimension
                    << ".");
      return 0;
  } // switch ( this->Dimension )

  // Prepare array of doubles for depth values
  vtkNew<vtkDoubleArray> depthArray;
  depthArray->SetName( "Depth" );
  depthArray->SetNumberOfComponents( 1 );
  outData->SetScalars( depthArray );

  if ( this->GenerateInterfaceFields )
  {
    // Prepare arrays of triples for interface surrogates
    vtkNew<vtkDoubleArray> normalsArray;
    normalsArray->SetName( "Normals" );
    normalsArray->SetNumberOfComponents( 3 );
    outData->SetVectors( normalsArray );

    vtkNew<vtkDoubleArray> interceptsArray;
    interceptsArray->SetName( "Intercepts" );
    interceptsArray->SetNumberOfComponents( 3 );
    outData->AddArray( interceptsArray );
  }

  if ( ! this->UseDescriptor )
  {
    // Prepare array of doubles for quadric values
    vtkNew<vtkDoubleArray> quadricArray;
    quadricArray->SetName( "Quadric" );
    quadricArray->SetNumberOfComponents( 1 );
    outData->AddArray( quadricArray );
  }

  // Iterate over constituting hypertrees
  if ( ! this->ProcessTrees( nullptr, outputDO ) )
  {
    return 0;
  }

  // Squeeze output data arrays
  for ( int a = 0; a < outData->GetNumberOfArrays(); ++ a )
  {
    outData->GetArray( a )->Squeeze();
  }

  assert( "post: dataset_and_data_size_match" && output->CheckAttributes() == 0 );

  this->LevelBitsIndexCnt.clear();
  this->LevelBitsIndex.clear();

  return 1;
}

//----------------------------------------------------------------------------
int vtkUniformHyperTreeGridSource::FillOutputPortInformation( int, vtkInformation* info )
{
  info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkUniformHyperTreeGrid" );
  return 1;
}
