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
// Serial tests for structured grid connectivity

// C++ includes
#include <iostream>
#include <sstream>
#include <cassert>
#include <string>
#include <vector>

// VTK includes
#include "vtkDataSet.h"
#include "vtkUniformGrid.h"
#include "vtkMultiBlockDataSet.h"
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
#include "vtkUniformGridPartitioner.h"
#include "vtkUnsignedCharArray.h"
#include "vtkXMLMultiBlockDataWriter.h"

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
// Generates a multi-block dataset
vtkMultiBlockDataSet* GetDataSet( const int numPartitions )
{
  int wholeExtent[6];
  wholeExtent[0] = 0;
  wholeExtent[1] = 99;
  wholeExtent[2] = 0;
  wholeExtent[3] = 99;
  wholeExtent[4] = 0;
  wholeExtent[5] = 99;

  int dims[3];
  dims[0] = wholeExtent[1] - wholeExtent[0] + 1;
  dims[1] = wholeExtent[3] - wholeExtent[2] + 1;
  dims[2] = wholeExtent[5] - wholeExtent[4] + 1;

  // Generate grid for the entire domain
  vtkUniformGrid *wholeGrid = vtkUniformGrid::New();
  wholeGrid->SetOrigin( 0.0, 0.0, 0.0  );
  wholeGrid->SetSpacing( 0.5, 0.5, 0.5 );
  wholeGrid->SetDimensions( dims );

  // partition the grid, the grid partitioner will generate the whole extent and
  // node extent information.
  vtkUniformGridPartitioner *gridPartitioner = vtkUniformGridPartitioner::New();
  gridPartitioner->SetInput( wholeGrid  );
  gridPartitioner->SetNumberOfPartitions( numPartitions );
  gridPartitioner->Update();

// THIS DOES NOT COPY THE INFORMATION KEYS!
//  vtkMultiBlockDataSet *mbds = vtkMultiBlockDataSet::New();
//  mbds->ShallowCopy( gridPartitioner->GetOutput() );

  vtkMultiBlockDataSet *mbds =
      vtkMultiBlockDataSet::SafeDownCast( gridPartitioner->GetOutput() );
  mbds->SetReferenceCount( mbds->GetReferenceCount()+1 );

  wholeGrid->Delete();
  gridPartitioner->Delete();

  assert( "pre: mbds is NULL" && (mbds != NULL) );
  return( mbds );
}

//------------------------------------------------------------------------------
// Description:
// Computes the total number of nodes in the multi-block dataset.
int GetTotalNumberOfNodes( vtkMultiBlockDataSet *multiblock )
{
  assert( "multi-block grid is NULL" && (multiblock != NULL) );

  int numNodes = 0;

  for(unsigned int block=0; block < multiblock->GetNumberOfBlocks(); ++block )
    {
    vtkUniformGrid *grid =
        vtkUniformGrid::SafeDownCast( multiblock->GetBlock( block ) );

    if( grid != NULL )
      {
      vtkIdType pntIdx = 0;
      for( ; pntIdx < grid->GetNumberOfPoints(); ++pntIdx )
        {
        unsigned char nodeProperty =
            *(grid->GetPointVisibilityArray()->GetPointer( pntIdx ));
        if( !vtkMeshPropertyEncoder::IsPropertySet(
            nodeProperty,VTKNodeProperties::IGNORE ) )
          {
          ++numNodes;
          }
        } // END for all nodes
      } // END if grid != NULL

    } // END for all blocks

  return( numNodes );
}

//------------------------------------------------------------------------------
void RegisterGrids(
    vtkMultiBlockDataSet *mbds, vtkStructuredGridConnectivity *connectivity )
{
  assert( "pre: Multi-block is NULL!" && (mbds != NULL) );
  assert( "pre: connectivity is NULL!" && (connectivity != NULL) );

  for( unsigned int block=0; block < mbds->GetNumberOfBlocks(); ++block )
    {
    vtkInformation *info = mbds->GetMetaData( block );
    assert( "pre: metadata should not be NULL" && (info != NULL) );
    assert( "pre: must have piece extent!" &&
            info->Has(vtkDataObject::PIECE_EXTENT() ) );

    connectivity->RegisterGrid(block,info->Get(vtkDataObject::PIECE_EXTENT()));
    } // END for all blocks
}

//------------------------------------------------------------------------------
void FillVisibilityArrays(
    vtkMultiBlockDataSet *mbds, vtkStructuredGridConnectivity *connectivity )
{
  assert( "pre: Multi-block is NULL!" && (mbds != NULL) );
  assert( "pre: connectivity is NULL!" && (connectivity != NULL) );

  for( unsigned int block=0; block < mbds->GetNumberOfBlocks(); ++block )
    {
    vtkUniformGrid *grid = vtkUniformGrid::SafeDownCast(mbds->GetBlock(block));
    if( grid != NULL )
      {
      vtkUnsignedCharArray *nodes = vtkUnsignedCharArray::New();
      nodes->SetNumberOfValues( grid->GetNumberOfPoints() );

      vtkUnsignedCharArray *cells = vtkUnsignedCharArray::New();
      cells->SetNumberOfValues( grid->GetNumberOfCells() );

      connectivity->FillMeshPropertyArrays(
          block, nodes->GetPointer(0), cells->GetPointer(0)  );

      grid->SetPointVisibilityArray( nodes );
      nodes->Delete();
      grid->SetCellVisibilityArray( cells );
      cells->Delete();
      } // END if grid != NULL
    } // END for all blocks
}

//------------------------------------------------------------------------------
void WriteMultiBlock( vtkMultiBlockDataSet *mbds )
{
  assert( "pre: Multi-block is NULL!" && (mbds != NULL) );


  vtkXMLMultiBlockDataWriter *writer = vtkXMLMultiBlockDataWriter::New();
  assert( "pre: Cannot allocate writer" && (writer != NULL) );

  std::ostringstream oss;
  oss.str("");
  oss << "MyMultiBlock_" << mbds->GetNumberOfBlocks() << "."
      << writer->GetDefaultFileExtension();
  writer->SetFileName( oss.str().c_str() );
  writer->SetInput( mbds );
  writer->Write();

  writer->Delete();
}

//------------------------------------------------------------------------------
// Program main
int TestStructuredGridConnectivity( int argc, char *argv[] )
{
  int expected = 100*100*100;
  int rc = 0;
  int numberOfPartitions[] = { 2, 4, 8, 16, 32, 64, 128, 256 };

  for( int i=0; i < 8; ++i )
    {
    // STEP 0: Construct the dataset
    std::cout << "===\n";
    std::cout << "-- Acquire dataset with N=" << numberOfPartitions[ i ];
    std::cout << " BLOCKS...";
    std::cout.flush();
    vtkMultiBlockDataSet *mbds = GetDataSet( numberOfPartitions[ i ] );
    assert( "pre: multi-block is NULL" && (mbds != NULL) );
    std::cout << "[DONE]\n";
    std::cout.flush();
    std::cout << "NUMBLOCKS: " << mbds->GetNumberOfBlocks() << std::endl;
    std::cout.flush();
    assert( "pre: NumBlocks mismatch!" &&
            (numberOfPartitions[i] ==
                static_cast<int>(mbds->GetNumberOfBlocks()) ) );
    WriteMultiBlock( mbds );

    // STEP 1: Construct the grid connectivity
    std::cout << "-- Allocating grid connectivity data-structures...";
    std::cout.flush();
    vtkStructuredGridConnectivity *gridConnectivity=
        vtkStructuredGridConnectivity::New();
    gridConnectivity->SetNumberOfGrids( mbds->GetNumberOfBlocks() );
    gridConnectivity->SetWholeExtent( mbds->GetWholeExtent() );
    std::cout << "[DONE]\n";
    std::cout.flush();

    // STEP 2: Registers the grids
    std::cout << "-- Registering grid blocks...";
    std::cout.flush();
    RegisterGrids( mbds, gridConnectivity );
    std::cout << "[DONE]\n";
    std::cout.flush();

    // STEP 3: Compute neighbors
    std::cout << "-- Computing neighbors...";
    std::cout.flush();
    gridConnectivity->ComputeNeighbors();
    std::cout << "[DONE]\n";
    std::cout.flush();

    gridConnectivity->Print( std::cout );

    // STEP 4: Fill-in the visibility arrays!
    std::cout << "-- Fill visibility arrays...";
    std::cout.flush();
    FillVisibilityArrays( mbds, gridConnectivity  );
    std::cout << "[DONE]\n";
    std::cout.flush();

    // STEP 5: Compute total number of nodes compare to expected
    std::cout << "-- Computing the total number of nodes...";
    std::cout.flush();
    int NumNodes = GetTotalNumberOfNodes( mbds );
    std::cout << "[DONE]\n";
    std::cout.flush();

    std::cout << "NUMNODES=" << NumNodes << " EXPECTED=" << expected << "...";
    if( NumNodes != expected )
      {
      ++rc;
      std::cout << "[ERROR]\n";
      std::cout.flush();
      mbds->Delete();
      gridConnectivity->Delete();
      return( rc );
      }
    else
      {
      std::cout << "[OK]\n";
      std::cout.flush();
      }

    // STEP 6: De-allocated data-structures
    mbds->Delete();
    gridConnectivity->Delete();
    } // END for all tests

  return( rc );
}

//------------------------------------------------------------------------------
// Description:
// A simple test designed as an aid in the development of the structured grid
// connectivity functionality.
int SimpleMonolithicTest( int argc, char **argv )
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
          if(!vtkMeshPropertyEncoder::IsPropertySet(p,VTKNodeProperties::IGNORE))
            {
            ++totalNumberOfNodes;
            if(vtkMeshPropertyEncoder::IsPropertySet(p,VTKNodeProperties::BOUNDARY))
              flags->SetValue( pIdx, 2 );
            else
              flags->SetValue( pIdx, 3);
            }
          else
            {
            flags->SetValue( pIdx, 1 );
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

//------------------------------------------------------------------------------
// Program main
int main( int argc, char **argv )
{
  int rc = 0;

  if( argc > 1 )
    {
    rc = SimpleMonolithicTest( argc, argv );
    }
  else
    {
    rc = TestStructuredGridConnectivity( argc, argv );
    }

  return( rc );
}
