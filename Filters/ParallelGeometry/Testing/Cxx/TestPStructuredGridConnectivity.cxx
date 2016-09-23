/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPStructuredGridConnectivity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestPStructuredGridConnectivity.cxx -- Parallel structured connectivity
//
// .SECTION Description
//  A test for parallel structured grid connectivity.

// C++ includes
#include <iostream>
#include <sstream>
#include <cassert>
#include <string>
#include <vector>

// MPI include
#include <mpi.h>

// VTK includes
#include "vtkDataSet.h"
#include "vtkUniformGrid.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMPIController.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkPStructuredGridConnectivity.h"
#include "vtkStructuredGridConnectivity.h"
#include "vtkStructuredNeighbor.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkUniformGridPartitioner.h"
#include "vtkUnsignedCharArray.h"
#include "vtkMathUtilities.h"
#include "vtkXMLPMultiBlockDataWriter.h"
#include "vtkStreamingDemandDrivenPipeline.h"

namespace
{

//------------------------------------------------------------------------------
//      G L O B A  L   D A T A
//------------------------------------------------------------------------------
vtkMultiProcessController *Controller;
int Rank;
int NumberOfProcessors;

//------------------------------------------------------------------------------
void WriteDistributedDataSet(
    const std::string &prefix, vtkMultiBlockDataSet *dataset)
{
#ifdef DEBUG_ON
  vtkXMLPMultiBlockDataWriter *writer = vtkXMLPMultiBlockDataWriter::New();
  std::ostringstream oss;
  oss << prefix << "." << writer->GetDefaultFileExtension();
  writer->SetFileName( oss.str().c_str() );
  writer->SetInputData(dataset);
  if( Controller->GetLocalProcessId() == 0 )
  {
    writer->SetWriteMetaFile(1);
  }
  writer->Update();
  writer->Delete();
#else
  (void)(prefix);
  (void)(dataset);
#endif

}

//------------------------------------------------------------------------------
void LogMessage( const std::string &msg)
{
  if( Controller->GetLocalProcessId() == 0 )
  {
    std::cout << msg << std::endl;
    std::cout.flush();
  }
}

//------------------------------------------------------------------------------
int GetTotalNumberOfNodes( vtkMultiBlockDataSet *multiblock )
{
  assert( "pre: Controller should not be NULL" && (Controller != NULL) );
  assert( "pre: multi-block grid is NULL" && (multiblock != NULL) );

  // STEP 0: Count local number of nodes
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
        if(grid->IsPointVisible(pntIdx))
        {
          ++numNodes;
        }
      } // END for all nodes
    } // END if grid != NULL

  } // END for all blocks

  // STEP 2: Synchronize processes
  Controller->Barrier();

  // STEP 3: Get a global sum
  int totalSum = 0;
  Controller->AllReduce(&numNodes,&totalSum,1,vtkCommunicator::SUM_OP);

  return( totalSum );
}

//------------------------------------------------------------------------------
// Description:
// Generates a distributed multi-block dataset, each grid is added using
// round-robin assignment.
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
  gridPartitioner->SetInputData( wholeGrid  );
  gridPartitioner->SetNumberOfPartitions( numPartitions );
  gridPartitioner->Update();
  vtkMultiBlockDataSet *partitionedGrid =
      vtkMultiBlockDataSet::SafeDownCast( gridPartitioner->GetOutput() );
  assert( "pre: partitionedGrid != NULL" && (partitionedGrid != NULL) );

  // Each process has the same number of blocks, i.e., the same structure,
  // however some block entries are NULL indicating that the data lives on
  // some other process
  vtkMultiBlockDataSet *mbds = vtkMultiBlockDataSet::New();
  mbds->SetNumberOfBlocks( numPartitions );
  mbds->GetInformation()->Set(
      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
      partitionedGrid->GetInformation()->Get(
          vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
          6 );
//  mbds->SetWholeExtent( partitionedGrid->GetWholeExtent( ) );

  // Populate blocks for this process
  unsigned int block=0;
  for( ; block < partitionedGrid->GetNumberOfBlocks(); ++block )
  {
    if( Rank == static_cast<int>( block%NumberOfProcessors ) )
    {
      // Copy the uniform grid
      vtkUniformGrid *grid = vtkUniformGrid::New();
      grid->DeepCopy( partitionedGrid->GetBlock(block) );

      mbds->SetBlock( block, grid );
      grid->Delete();

      // Copy the global extent for the blockinformation
      vtkInformation *info = partitionedGrid->GetMetaData( block );
      assert( "pre: null metadata!" && (info != NULL) );
      assert( "pre: must have a piece extent!" &&
              (info->Has(vtkDataObject::PIECE_EXTENT() ) ) );

      vtkInformation *metadata = mbds->GetMetaData( block );
      assert( "pre: null metadata!" && (metadata != NULL) );
      metadata->Set(
        vtkDataObject::PIECE_EXTENT(),
        info->Get( vtkDataObject::PIECE_EXTENT() ),
        6 );
    } // END if we own the block
    else
    {
      mbds->SetBlock( block, NULL );
    } // END else we don't own the block
  } // END for all blocks

  wholeGrid->Delete();
  gridPartitioner->Delete();

  assert( "pre: mbds is NULL" && (mbds != NULL) );
  return( mbds );
}

//------------------------------------------------------------------------------
void RegisterGrids(
    vtkMultiBlockDataSet *mbds, vtkPStructuredGridConnectivity *connectivity )
{
  assert( "pre: Multi-block is NULL!" && (mbds != NULL) );
  assert( "pre: connectivity is NULL!" && (connectivity != NULL) );

  for( unsigned int block=0; block < mbds->GetNumberOfBlocks(); ++block )
  {
    vtkUniformGrid *grid = vtkUniformGrid::SafeDownCast(mbds->GetBlock(block));
    if( grid != NULL )
    {
      vtkInformation *info = mbds->GetMetaData( block );
      assert( "pre: metadata should not be NULL" && (info != NULL) );
      assert( "pre: must have piece extent!" &&
              info->Has(vtkDataObject::PIECE_EXTENT() ) );
      connectivity->RegisterGrid(
          block,info->Get(vtkDataObject::PIECE_EXTENT()),
          grid->GetPointGhostArray(),
          grid->GetCellGhostArray(),
          grid->GetPointData(),
          grid->GetCellData(),
          NULL );
    } // END if block belongs to this process
  } // END for all blocks
}

//------------------------------------------------------------------------------
// Tests StructuredGridConnectivity on a distributed data-set
int TestPStructuredGridConnectivity( const int factor )
{
  assert( "pre: MPI Controller is NULL!" && (Controller != NULL) );

  int expected = 100*100*100;

  // STEP 0: Calculation number of partitions as factor of the number of
  // processes.
  assert( "pre: factor >= 1" && (factor >= 1) );
  int numPartitions = factor * NumberOfProcessors;

  // STEP 1: Acquire the distributed structured grid for this process.
  // Each process has the same number of blocks, but not all entries are
  // poplulated. A NULL entry indicates that the block belongs to a different
  // process.
  vtkMultiBlockDataSet *mbds = GetDataSet( numPartitions );
  Controller->Barrier();
  assert( "pre: mbds != NULL" && (mbds != NULL) );
  assert( "pre: numBlocks mismatch" &&
           (static_cast<int>(mbds->GetNumberOfBlocks())==numPartitions) );

  // STEP 2: Setup the grid connectivity
  vtkPStructuredGridConnectivity *gridConnectivity =
      vtkPStructuredGridConnectivity::New();
  gridConnectivity->SetController( Controller );
  gridConnectivity->SetNumberOfGrids( mbds->GetNumberOfBlocks() );
  gridConnectivity->SetWholeExtent(
      mbds->GetInformation()->Get(
          vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  gridConnectivity->Initialize();

  // STEP 3: Register the grids
  RegisterGrids( mbds, gridConnectivity );
  Controller->Barrier();

  // STEP 4: Compute neighbors
  gridConnectivity->ComputeNeighbors();
  Controller->Barrier();

  // STEP 6: Total global count of the nodes
  int count = GetTotalNumberOfNodes( mbds );
  Controller->Barrier();

  // STEP 7: Deallocate
  mbds->Delete();
  gridConnectivity->Delete();

  // STEP 8: return success or failure
  if( count != expected )
    return 1;
  return 0;
}

//------------------------------------------------------------------------------
// Assuming a 100x100x100 domain and a field given by F=X+Y+Z at each
// node, the method computes the average.
double CalculateExpectedAverage()
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

  double pnt[3];
  double sum = 0.0;
  for( vtkIdType pntIdx=0; pntIdx < wholeGrid->GetNumberOfPoints(); ++pntIdx )
  {
    wholeGrid->GetPoint( pntIdx, pnt );
    sum += pnt[0];
    sum += pnt[1];
    sum += pnt[2];
  } // END for all points

  double N = static_cast< double >( wholeGrid->GetNumberOfPoints() );
  wholeGrid->Delete();
  return( sum/N );
}

//------------------------------------------------------------------------------
double GetXYZSumForGrid( vtkUniformGrid *grid )
{
  assert( "pre: input grid is NULL!" && (grid != NULL) );

  double pnt[3];
  double sum = 0.0;
  for( vtkIdType pntIdx=0; pntIdx < grid->GetNumberOfPoints(); ++pntIdx )
  {
    if(grid->IsPointVisible(pntIdx))
    {
      grid->GetPoint( pntIdx, pnt );
      sum += pnt[0];
      sum += pnt[1];
      sum += pnt[2];
    }
  } // END for all points
  return( sum );
}

//------------------------------------------------------------------------------
// Tests computing the average serially vs. in paraller using a factor*N
// partitions where N is the total number of processes. An artificialy field
// F=X+Y+Z is imposed on each node.
int TestAverage( const int factor )
{
  // STEP 0: Calculate expected value
  double expected = CalculateExpectedAverage();

  // STEP 1: Calculation number of partitions as factor of the number of
  // processes.
  assert( "pre: factor >= 1" && (factor >= 1) );
  int numPartitions = factor * NumberOfProcessors;

  // STEP 2: Acquire the distributed structured grid for this process.
  // Each process has the same number of blocks, but not all entries are
  // poplulated. A NULL entry indicates that the block belongs to a different
  // process.
  vtkMultiBlockDataSet *mbds = GetDataSet( numPartitions );
  assert( "pre: mbds != NULL" && (mbds != NULL) );
  assert( "pre: numBlocks mismatch" &&
           (static_cast<int>(mbds->GetNumberOfBlocks())==numPartitions) );

  // STEP 2: Setup the grid connectivity
  vtkPStructuredGridConnectivity *gridConnectivity =
      vtkPStructuredGridConnectivity::New();
  gridConnectivity->SetController( Controller );
  gridConnectivity->SetNumberOfGrids( mbds->GetNumberOfBlocks() );
  gridConnectivity->SetWholeExtent(
      mbds->GetInformation()->Get(
          vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  gridConnectivity->Initialize();

  // STEP 3: Register the grids
  RegisterGrids( mbds, gridConnectivity );
  Controller->Barrier();

  // STEP 4: Compute neighbors
  gridConnectivity->ComputeNeighbors();
  Controller->Barrier();

  // STEP 6: Total global count of the nodes
  int count = GetTotalNumberOfNodes( mbds );
  Controller->Barrier();

  // STEP 7: Compute partial local sum
  double partialSum     = 0.0;
  unsigned int blockIdx = 0;
  for( ; blockIdx < mbds->GetNumberOfBlocks(); ++blockIdx )
  {
    vtkUniformGrid *blockPtr =
        vtkUniformGrid::SafeDownCast( mbds->GetBlock( blockIdx ) );
    if( blockPtr != NULL )
    {
      partialSum += GetXYZSumForGrid( blockPtr );
    } // END if
  } // END for all blocks

  // STEP 8: All reduce to the global sum
  double globalSum = 0.0;
  Controller->AllReduce(&partialSum,&globalSum,1,vtkCommunicator::SUM_OP);

  // STEP 9: Compute average
  double average = globalSum / static_cast< double >( count );

  // STEP 7: Deallocate
  mbds->Delete();
  gridConnectivity->Delete();

  // STEP 8: return success or failure
  if( vtkMathUtilities::FuzzyCompare(average,expected) )
  {
    if( Rank == 0 )
    {
      std::cout << "Computed: " << average << " Expected: " << expected << "\n";
      std::cout.flush();
    }
    return 0;
  }

  if( Rank == 0 )
  {
    std::cout << "Global sum: "      << globalSum << std::endl;
    std::cout << "Number of Nodes: " << count     << std::endl;
    std::cout << "Computed: " << average << " Expected: " << expected << "\n";
    std::cout.flush();
  }

  return 1;
}

//------------------------------------------------------------------------------
int TestGhostLayerCreation( int factor, int ng )
{
  // STEP 1: Calculation number of partitions as factor of the number of
  // processes.
  assert( "pre: factor >= 1" && (factor >= 1) );
  int numPartitions = factor * NumberOfProcessors;

  // STEP 2: Acquire the distributed structured grid for this process.
  // Each process has the same number of blocks, but not all entries are
  // poplulated. A NULL entry indicates that the block belongs to a different
  // process.
  vtkMultiBlockDataSet *mbds = GetDataSet( numPartitions );
  WriteDistributedDataSet( "PINITIAL", mbds );
  assert( "pre: mbds != NULL" && (mbds != NULL) );
  assert( "pre: numBlocks mismatch" &&
           (static_cast<int>(mbds->GetNumberOfBlocks())==numPartitions) );

  // STEP 2: Setup the grid connectivity
  vtkPStructuredGridConnectivity *gridConnectivity =
      vtkPStructuredGridConnectivity::New();
  gridConnectivity->SetController( Controller );
  gridConnectivity->SetNumberOfGrids( mbds->GetNumberOfBlocks() );
  gridConnectivity->SetWholeExtent(
      mbds->GetInformation()->Get(
          vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  gridConnectivity->Initialize();

  // STEP 3: Register the grids
  RegisterGrids( mbds, gridConnectivity );
  Controller->Barrier();

  // STEP 4: Compute neighbors
  gridConnectivity->ComputeNeighbors();
  Controller->Barrier();

  // STEP 5: Create ghost layers
  gridConnectivity->CreateGhostLayers( ng );
  Controller->Barrier();

  mbds->Delete();
  gridConnectivity->Delete();
  return 0;
}

}

//------------------------------------------------------------------------------
// Program main
int TestPStructuredGridConnectivity( int argc, char *argv[] )
{
  int rc       = 0;

  // STEP 0: Initialize
  Controller = vtkMPIController::New();
  Controller->Initialize( &argc, &argv, 0 );
  assert("pre: Controller should not be NULL" && (Controller != NULL) );
  vtkMultiProcessController::SetGlobalController( Controller );
  LogMessage( "Finished MPI Initialization!" );

  LogMessage( "Getting Rank ID and NumberOfProcessors..." );
  Rank               = Controller->GetLocalProcessId();
  NumberOfProcessors = Controller->GetNumberOfProcesses();
  assert( "pre: NumberOfProcessors >= 1" && (NumberOfProcessors >= 1) );
  assert( "pre: Rank is out-of-bounds" && (Rank >= 0) );

  // STEP 1: Run test where the number of partitions is equal to the number of
  // processes
  Controller->Barrier();
  LogMessage( "Testing with same number of partitions as processes..." );
  rc += TestPStructuredGridConnectivity( 1 );
  Controller->Barrier();

  // STEP 2: Run test where the number of partitions is double the number of
  // processes
  Controller->Barrier();
  LogMessage("Testing with double the number of partitions as processes...");
  rc += TestPStructuredGridConnectivity( 2 );
  Controller->Barrier();

  // STEP 4: Compute average
  LogMessage("Calculating average with same number of partitions as processes");
  rc += TestAverage( 1 );
  Controller->Barrier();

  LogMessage("Calculating average with double the number of partitions");
  rc += TestAverage( 2 );
  Controller->Barrier();

  LogMessage( "Creating ghost-layers" );
  rc += TestGhostLayerCreation( 1, 1 );

  // STEP 3: Deallocate controller and exit
  LogMessage( "Finalizing..." );
  Controller->Finalize();
  Controller->Delete();

  if( rc != 0 )
  {
    std::cout << "Test Failed!\n";
    rc = 0;
  }
  return( rc );
}
