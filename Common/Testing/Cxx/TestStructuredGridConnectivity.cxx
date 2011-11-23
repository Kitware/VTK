/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStructuredGridConnectivity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestStructuredGridConnectivity.cxx --Test vtkStructuredGridConnectivity
//
// .SECTION Description
//  A simple test for vtkStructuredGridConnectivity which constructs a uniform
//  grid with two partitions (pieces).



// C++ includes
#include <iostream>
#include <sstream>
#include <cassert>
#include <string>
#include <vector>

// VTK includes
#include "vtkDataSet.h"
#include "vtkUniformGrid.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkMPIController.h"
#include "vtkDataObject.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkStructuredGridConnectivity.h"
#include "vtkStructuredNeighbor.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIntArray.h"
#include "vtkMeshPropertyEncoder.h"
#include "vtkMeshProperty.h"
#include "vtkPointData.h"
#include "vtkCellData.h"


//------------------------------------------------------------------------------
// Description:
// Writes the grid to a file
void WriteGrid( vtkUniformGrid *grid, std::string prefix )
{
  assert( "pre: input grid is NULL" && (grid != NULL) );

  vtkXMLImageDataWriter *writer = vtkXMLImageDataWriter::New();

  std::ostringstream oss;
  oss << prefix << "." << writer->GetDefaultFileExtension();
  writer->SetFileName( oss.str().c_str() );
  writer->SetInput( grid );
  writer->Write();

  writer->Delete();
}

//------------------------------------------------------------------------------
// Description:
// Gets the grid for a given rank
vtkUniformGrid* GetGrid( const int rank, int globalExtent[6] )
{
  // Fix spacing at 0.5
  double h[3];
  h[0] = h[1] = h[2] = 0.5;

  // Fix dimensions to 10
  int dims[3];
  dims[0] = dims[1] = dims[2] = 10;

  // Fix global origin at (0.0, 0.0, 0.0)
  double globalOrigin[3];
  globalOrigin[0] = globalOrigin[1] = globalOrigin[2] = 0.0;

  // Setup the global extent for this grid instance
  for( int i=0; i < 3; ++i )
    {
    if( i == 0 )
      {
      globalExtent[i*2]   = rank*10;
      if( globalExtent[i*2] > 0)
        globalExtent[i*2]-= rank;

      globalExtent[i*2+1] = globalExtent[i*2]+10-1;
      }
    else
      {
      globalExtent[ i*2 ]   = 0;
      globalExtent[ i*2+1 ] = 9;
      }
    }



  // Compute local origin
  double localOrigin[3];
  localOrigin[0] = globalOrigin[0] + globalExtent[0]*h[0];
  localOrigin[1] = 0.0;
  localOrigin[2] = 0.0;

  // Setup uniform grid
  vtkUniformGrid *grid = vtkUniformGrid:: New();
  grid->SetOrigin( localOrigin );
  grid->SetDimensions( dims );
  grid->SetSpacing( h );

  return( grid );
}

//------------------------------------------------------------------------------
// Description:
// Generates a multipiece of uniform grids
vtkMultiPieceDataSet* GetDataSet( )
{
  vtkMultiPieceDataSet *mpds = vtkMultiPieceDataSet::New();

  // Setup the hole extent
  int wholeExtent[6];
  wholeExtent[0] = 0;                  // IMIN
  wholeExtent[1] = 18;                 // IMAX
  wholeExtent[2] = 0;                  // JMIN
  wholeExtent[3] = 9;                  // JMAX
  wholeExtent[4] = 0;                  // KMIN
  wholeExtent[5] = 9;                  // KMAX
  mpds->SetWholeExtent( wholeExtent );

  int ext1[6];
  int ext2[6];
  vtkUniformGrid *grid1 = GetGrid( 0, ext1 );
  vtkUniformGrid *grid2 = GetGrid( 1, ext2 );

  mpds->SetNumberOfPieces( 2 );
  mpds->SetPiece( 0, grid1 );
  mpds->GetMetaData( static_cast<unsigned int>(0) )->Set(
      vtkDataObject::PIECE_EXTENT(), ext1, 6 );
  mpds->SetPiece( 1, grid2 );
  mpds->GetMetaData( 1 )->Set( vtkDataObject::PIECE_EXTENT(),ext2, 6 );
  grid1->Delete();
  grid2->Delete();

  return( mpds );
}

//------------------------------------------------------------------------------
// Description:
// Program main
int main( int argc, char **argv )
{
  vtkMultiPieceDataSet *mpds = GetDataSet( );

  vtkStructuredGridConnectivity  *gridConnectivity =
      vtkStructuredGridConnectivity::New();
  gridConnectivity->SetNumberOfGrids( mpds->GetNumberOfPieces() );
  gridConnectivity->SetWholeExtent( mpds->GetWholeExtent() );

  int ext[6];
  std::ostringstream oss;
  for( unsigned int piece=0; piece < mpds->GetNumberOfPieces(); ++piece )
    {

    vtkUniformGrid *grid = vtkUniformGrid::SafeDownCast(mpds->GetPiece(piece ));

    if( grid != NULL )
      {
      oss.str( "" );
      oss << "GRID_" << piece;
      WriteGrid( grid, oss.str() );

      mpds->GetMetaData( piece )->Get( vtkDataObject::PIECE_EXTENT(),ext );

      gridConnectivity->RegisterGrid( piece, ext );
      }

    } // END for all pieces

  std::cout << "Creating ghost nodes...\n";
  std::cout.flush();
  gridConnectivity->ComputeNeighbors();
  std::cout << "[DONE]\n";
  std::cout.flush();

  // Calculate number of nodes
  int totalNumberOfNodes = 0;
  for( unsigned int piece=0; piece < mpds->GetNumberOfPieces(); ++piece )
    {
      vtkUniformGrid *grid =
          vtkUniformGrid::SafeDownCast(mpds->GetPiece(piece ));

      if( grid != NULL )
        {
        unsigned char *nodeProperty = new unsigned char[ grid->GetNumberOfPoints() ];
        unsigned char *cellProperty = new unsigned char[ grid->GetNumberOfCells() ];

        gridConnectivity->FillMeshPropertyArrays(
            piece, nodeProperty,cellProperty);

        vtkIntArray *flags = vtkIntArray::New();
        flags->SetName( "FLAGS" );
        flags->SetNumberOfComponents( 1 );
        flags->SetNumberOfTuples( grid->GetNumberOfPoints( ) );

        vtkIdType pIdx = 0;
        for( ; pIdx < grid->GetNumberOfPoints(); ++pIdx )
          {
          unsigned char p = nodeProperty[ pIdx ];
          if( vtkMeshPropertyEncoder::IsPropertySet(p,VTKNodeProperties::SHARED) )
            {
            flags->SetValue( pIdx, VTKNodeProperties::SHARED );
            }
          else if( vtkMeshPropertyEncoder::IsPropertySet(p,VTKNodeProperties::BOUNDARY) )
            {
            flags->SetValue(pIdx, VTKNodeProperties::BOUNDARY );
            }
          else
            {
            flags->SetValue(pIdx, VTKNodeProperties::INTERNAL );
            }

          } // END for all points

        grid->GetPointData()->AddArray( flags );
        flags->Delete();

        oss.str( "" );
        oss << "BLANKEDGRID_" << piece;
        WriteGrid( grid, oss.str() );

        }

    }// END for all pieces

  std::cout << "TOTAL NUMBER OF NODES: " << totalNumberOfNodes << std::endl;
  std::cout.flush();

  gridConnectivity->Delete();
  mpds->Delete();
  return 0;
}
