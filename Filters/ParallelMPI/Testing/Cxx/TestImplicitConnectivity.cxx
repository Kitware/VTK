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
// .NAME TestImplicitConnectivity.cxx -- Parallel implicit connectivity test
//
// .SECTION Description
//  A test for parallel structured grid connectivity.

// C++ includes
#include <iostream>
#include <sstream>
#include <cassert>
#include <string>
#include <vector>

// VTK includes
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkDoubleArray.h"
#include "vtkImageToStructuredGrid.h"
#include "vtkInformation.h"
#include "vtkMPIController.h"
#include "vtkMPIUtilities.h"
#include "vtkMathUtilities.h"
#include "vtkMathUtilities.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridPartitioner.h"
#include "vtkRectilinearGridWriter.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridPartitioner.h"
#include "vtkStructuredImplicitConnectivity.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkXMLPMultiBlockDataWriter.h"


#define DEBUG_ON

//------------------------------------------------------------------------------
//      G L O B A  L   D A T A
//------------------------------------------------------------------------------
vtkMultiProcessController *Controller;
int Rank;
int NumberOfProcessors;

//------------------------------------------------------------------------------
void WriteDistributedDataSet(
    std::string prefix, vtkMultiBlockDataSet *dataset)
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
void AddNodeCenteredXYZField( vtkMultiBlockDataSet *mbds )
{
  assert("pre: Multi-block is NULL!" && (mbds != NULL) );

  for( unsigned int block=0; block < mbds->GetNumberOfBlocks(); ++block )
    {
    vtkDataSet* grid = vtkDataSet::SafeDownCast(mbds->GetBlock(block));

    if( grid == NULL )
      {
      continue;
      }

    vtkDoubleArray *nodeXYZArray = vtkDoubleArray::New();
    nodeXYZArray->SetName( "NODE-XYZ" );
    nodeXYZArray->SetNumberOfComponents( 3 );
    nodeXYZArray->SetNumberOfTuples( grid->GetNumberOfPoints() );

    double xyz[3];
    for( vtkIdType pntIdx=0; pntIdx < grid->GetNumberOfPoints(); ++pntIdx )
      {
      grid->GetPoint( pntIdx, xyz );
      nodeXYZArray->SetComponent( pntIdx, 0, xyz[0] );
      nodeXYZArray->SetComponent( pntIdx, 1, xyz[1] );
      nodeXYZArray->SetComponent( pntIdx, 2, xyz[2] );
      } // END for all points

    grid->GetPointData()->AddArray( nodeXYZArray );
    nodeXYZArray->Delete();
    } // END for all blocks

}

//------------------------------------------------------------------------------
// Description:
// Generates a distributed multi-block dataset, each grid is added using
// round-robin assignment.
vtkMultiBlockDataSet* GetDataSet(
    const int numPartitions,
    double origin[3],
    double h[3],
    int wholeExtent[6])
{

  int dims[3];
  int desc = vtkStructuredData::GetDataDescriptionFromExtent(wholeExtent);
  vtkStructuredData::GetDimensionsFromExtent(wholeExtent,dims,desc);

  // Generate grid for the entire domain
  vtkUniformGrid *wholeGrid = vtkUniformGrid::New();
  wholeGrid->SetOrigin( origin[0], origin[1], origin[2]  );
  wholeGrid->SetSpacing( h[0], h[1], h[2] );
  wholeGrid->SetDimensions( dims );

  // Convert to structured grid
  vtkImageToStructuredGrid* img2sgrid = vtkImageToStructuredGrid::New();
  img2sgrid->SetInputData( wholeGrid );
  img2sgrid->Update();
  vtkStructuredGrid* wholeStructuredGrid = vtkStructuredGrid::New();
  wholeStructuredGrid->DeepCopy( img2sgrid->GetOutput() );
  img2sgrid->Delete();
  wholeGrid->Delete();

  // partition the grid, the grid partitioner will generate the whole extent and
  // node extent information.
  vtkStructuredGridPartitioner *gridPartitioner =
      vtkStructuredGridPartitioner::New();
  gridPartitioner->SetInputData( wholeStructuredGrid  );
  gridPartitioner->SetNumberOfPartitions( numPartitions );
  gridPartitioner->SetNumberOfGhostLayers(0);
  gridPartitioner->DuplicateNodesOff(); /* to create a gap */
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

  // Populate blocks for this process
  unsigned int block=0;
  for( ; block < partitionedGrid->GetNumberOfBlocks(); ++block )
    {
    if( Rank == static_cast<int>( block%NumberOfProcessors ) )
      {
      // Copy the structured grid
      vtkStructuredGrid *grid = vtkStructuredGrid::New();
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

  wholeStructuredGrid->Delete();
  gridPartitioner->Delete();

  assert( "pre: mbds is NULL" && (mbds != NULL) );

  AddNodeCenteredXYZField( mbds );
  Controller->Barrier();

  return( mbds );
}

//------------------------------------------------------------------------------
double exponential_distribution(const int i, const double beta)
{
  double xi=0.0;
  xi = ( ( exp( i*beta ) - 1 ) /( exp( beta ) - 1 ) );
  return( xi );
}

//------------------------------------------------------------------------------
void GenerateRectGrid( vtkRectilinearGrid* grid, int ext[6], double origin[3])
{
  grid->Initialize();
  grid->SetExtent(ext);

  vtkDataArray* coords[3];

  int dims[3];
  int dataDesc = vtkStructuredData::GetDataDescriptionFromExtent(ext);
  vtkStructuredData::GetDimensionsFromExtent(ext,dims,dataDesc);

  // compute & populate coordinate vectors
  double beta = 0.01; /* controls the intensity of the stretching */
  for(int i=0; i < 3; ++i)
    {
    coords[i] = vtkDataArray::CreateDataArray(VTK_DOUBLE);
    if( dims[i] == 0 )
      {
      continue;
      }
    coords[i]->SetNumberOfTuples(dims[i]);

    double prev = origin[i];
    for(int j=0; j < dims[i]; ++j)
      {
      double val = prev + ( (j==0)? 0.0 : exponential_distribution(j,beta) );
      coords[ i ]->SetTuple( j, &val );
      prev = val;
      } // END for all points along this dimension
     } // END for all dimensions

   grid->SetXCoordinates( coords[0] );
   grid->SetYCoordinates( coords[1] );
   grid->SetZCoordinates( coords[2] );
   coords[0]->Delete();
   coords[1]->Delete();
   coords[2]->Delete();
}

//------------------------------------------------------------------------------
// Description:
// Generates a distributed multi-block dataset, each grid is added using
// round-robin assignment.
vtkMultiBlockDataSet* GetRectGridDataSet(
    const int numPartitions,
    double origin[3],
    int wholeExtent[6])
{
  int dims[3];
  int desc = vtkStructuredData::GetDataDescriptionFromExtent(wholeExtent);
  vtkStructuredData::GetDimensionsFromExtent(wholeExtent,dims,desc);

  vtkRectilinearGrid* wholeGrid = vtkRectilinearGrid::New();
  GenerateRectGrid(wholeGrid,wholeExtent,origin);
//#ifdef DEBUG_ON
//  if( Controller->GetLocalProcessId() == 0 )
//    {
//    vtkRectilinearGridWriter* writer = vtkRectilinearGridWriter::New();
//    writer->SetFileName("RectilinearGrid.vtk");
//    writer->SetInputData( wholeGrid );
//    writer->Write();
//    writer->Delete();
//    }
//#endif
  vtkRectilinearGridPartitioner *gridPartitioner =
        vtkRectilinearGridPartitioner::New();
    gridPartitioner->SetInputData( wholeGrid  );
    gridPartitioner->SetNumberOfPartitions( numPartitions );
    gridPartitioner->SetNumberOfGhostLayers(0);
    gridPartitioner->DuplicateNodesOff(); /* to create a gap */
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

    // Populate blocks for this process
    unsigned int block=0;
    for( ; block < partitionedGrid->GetNumberOfBlocks(); ++block )
     {
     if( Rank == static_cast<int>( block%NumberOfProcessors ) )
       {
       // Copy the structured grid
       vtkRectilinearGrid *grid = vtkRectilinearGrid::New();
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

  AddNodeCenteredXYZField( mbds );
  Controller->Barrier();

  return mbds;
}

//------------------------------------------------------------------------------
void RegisterRectGrid(vtkMultiBlockDataSet* mbds,
                      vtkStructuredImplicitConnectivity* connectivity)
{
  assert( "pre: Multi-block is NULL!" && (mbds != NULL) );
  assert( "pre: connectivity is NULL!" && (connectivity != NULL) );

  for(unsigned int block=0; block < mbds->GetNumberOfBlocks(); ++block)
    {
    vtkRectilinearGrid* grid =
        vtkRectilinearGrid::SafeDownCast(mbds->GetBlock(block));

    if( grid != NULL )
      {
      vtkInformation* info = mbds->GetMetaData( block );
      assert( "pre: metadata should not be NULL" && (info != NULL) );
      assert( "pre: must have piece extent!" &&
               info->Has(vtkDataObject::PIECE_EXTENT() ) );

      connectivity->RegisterRectilinearGrid(
              block,
              info->Get(vtkDataObject::PIECE_EXTENT()),
              grid->GetXCoordinates(),
              grid->GetYCoordinates(),
              grid->GetZCoordinates(),
              grid->GetPointData()
              );
      } // END if block belongs to this process
    } // END for all blocks
}

//------------------------------------------------------------------------------
void RegisterGrid(
    vtkMultiBlockDataSet *mbds,
    vtkStructuredImplicitConnectivity* connectivity )
{
  assert( "pre: Multi-block is NULL!" && (mbds != NULL) );
  assert( "pre: connectivity is NULL!" && (connectivity != NULL) );


  for( unsigned int block=0; block < mbds->GetNumberOfBlocks(); ++block )
    {
    vtkStructuredGrid *grid =
        vtkStructuredGrid::SafeDownCast(mbds->GetBlock(block));
    if( grid != NULL )
      {
      vtkInformation *info = mbds->GetMetaData( block );
      assert( "pre: metadata should not be NULL" && (info != NULL) );
      assert( "pre: must have piece extent!" &&
              info->Has(vtkDataObject::PIECE_EXTENT() ) );
      connectivity->RegisterGrid(
          block,
          info->Get(vtkDataObject::PIECE_EXTENT()),
          grid->GetPoints(),
          grid->GetPointData()
          );
      } // END if block belongs to this process
    } // END for all blocks
}

//------------------------------------------------------------------------------
int CheckGrid(vtkDataSet* grid)
{
  assert("pre: grid should not be NULL!" && (grid != NULL) );
  int rc = 0;

  vtkPointData* PD = grid->GetPointData();
  assert("pre: PD should not be NULL!" && (PD != NULL) );

  if( !PD->HasArray("NODE-XYZ") )
    {
    std::cerr << "ERROR: NODE-XYZ array does not exist!\n";
    return 1;
    }

  vtkDoubleArray* array=vtkDoubleArray::SafeDownCast(PD->GetArray("NODE-XYZ"));
  if( array==NULL )
    {
    std::cerr << "ERROR: null vtkDataArray!\n";
    return 1;
    }

  if( PD->GetNumberOfTuples() != grid->GetNumberOfPoints() )
    {
    std::cerr << "ERROR: PointData numTuples != num grid points!\n";
    return 1;
    }

  double pnt[3];
  double* dataPtr = static_cast<double*>(array->GetVoidPointer(0));

  vtkIdType N = grid->GetNumberOfPoints();
  for(vtkIdType idx=0; idx < N; ++idx)
    {
    grid->GetPoint(idx,pnt);

    if( !vtkMathUtilities::NearlyEqual(pnt[0],dataPtr[idx*3],1.e-9)   ||
        !vtkMathUtilities::NearlyEqual(pnt[1],dataPtr[idx*3+1],1.e-9) ||
        !vtkMathUtilities::NearlyEqual(pnt[2],dataPtr[idx*3+2],1.e-9) )
      {
      ++rc;
      } // END if rc

    } // END for all points

  return( rc );
}

//------------------------------------------------------------------------------
int TestOutput(vtkMultiBlockDataSet* mbds, int wholeExtent[6])
{
  int rc = 0;

  // Check if the output grid has gaps
  vtkStructuredImplicitConnectivity* gridConnectivity =
      vtkStructuredImplicitConnectivity::New();
  gridConnectivity->SetWholeExtent(wholeExtent);

  vtkDataSet* grid = NULL;
  for(unsigned int block=0; block < mbds->GetNumberOfBlocks(); ++block)
    {
    grid = vtkDataSet::SafeDownCast( mbds->GetBlock(block) );
    if( grid != NULL )
      {
      if( grid->IsA("vtkStructuredGrid") )
        {
        vtkStructuredGrid* sGrid = vtkStructuredGrid::SafeDownCast(grid);
        gridConnectivity->RegisterGrid(
            block,
            sGrid->GetExtent(),
            sGrid->GetPoints(),
            sGrid->GetPointData()
            );
        }
      else
        {
        assert("pre: expected rectilinear grid!" &&
                grid->IsA("vtkRectilinearGrid"));
        vtkRectilinearGrid* rGrid = vtkRectilinearGrid::SafeDownCast(grid);
        gridConnectivity->RegisterRectilinearGrid(
            block,
            rGrid->GetExtent(),
            rGrid->GetXCoordinates(),
            rGrid->GetYCoordinates(),
            rGrid->GetZCoordinates(),
            rGrid->GetPointData()
            );
        }

      rc += CheckGrid(grid);
      } // END if grid != NULL

    } // END for all blocks

  int rcLocal = rc;
  Controller->AllReduce(&rcLocal,&rc,1,vtkCommunicator::SUM_OP);
  if( rc > 0 )
    {
    vtkMPIUtilities::Printf(vtkMPIController::SafeDownCast(Controller),
        "ERROR: Check grid failed!");
    }

  gridConnectivity->EstablishConnectivity();

  if( gridConnectivity->HasImplicitConnectivity() )
    {
    vtkMPIUtilities::Printf(vtkMPIController::SafeDownCast(Controller),
        "ERROR: output grid still has a gap!\n");
    ++rc;
    }
  vtkMPIUtilities::Printf(
      vtkMPIController::SafeDownCast(Controller),"Grid has no gaps!\n");
  gridConnectivity->Delete();

  return( rc );
}

//------------------------------------------------------------------------------
// Tests StructuredGridConnectivity on a distributed data-set
int TestImplicitGridConnectivity2DYZ( )
{
  assert( "pre: MPI Controller is NULL!" && (Controller != NULL) );

  vtkMPIUtilities::Printf(vtkMPIController::SafeDownCast(Controller),
      "=======================\nTesting 2-D Dataset on the YZ-plane\n");

  int rc = 0;

  int wholeExtent[6];
  wholeExtent[0]   = 0;
  wholeExtent[1]   = 0;
  wholeExtent[2]   = 0;
  wholeExtent[3]   = 49;
  wholeExtent[4]   = 0;
  wholeExtent[5]   = 49;
  double h[3]      = {0.5,0.5,0.5};
  double origin[3] = {0.0,0.0,0.0};

  // STEP 0: We generate the same number of partitions as processes
  int numPartitions = NumberOfProcessors;

  // STEP 1: Acquire the distributed structured grid for this process.
  // Each process has the same number of blocks, but not all entries are
  // populated. A NULL entry indicates that the block belongs to a different
  // process.
  vtkMultiBlockDataSet *mbds =
      GetDataSet( numPartitions,origin,h,wholeExtent );
  Controller->Barrier();
  assert( "pre: mbds != NULL" && (mbds != NULL) );
  assert( "pre: numBlocks mismatch" &&
           (static_cast<int>(mbds->GetNumberOfBlocks())==numPartitions) );
  WriteDistributedDataSet("INPUT2DYZ",mbds);

  // STEP 2: Setup the grid connectivity
  vtkStructuredImplicitConnectivity *gridConnectivity =
      vtkStructuredImplicitConnectivity::New();
  gridConnectivity->SetWholeExtent(
      mbds->GetInformation()->Get(
          vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));

  // STEP 3: Register the grids
  RegisterGrid( mbds, gridConnectivity );
  Controller->Barrier();

  // STEP 4: Compute neighbors
  gridConnectivity->EstablishConnectivity();
  Controller->Barrier();

  // print the neighboring information from each process
  std::ostringstream oss;
  gridConnectivity->Print( oss );
  vtkMPIUtilities::SynchronizedPrintf(
      vtkMPIController::SafeDownCast(Controller),"%s\n",oss.str().c_str());

  if(!gridConnectivity->HasImplicitConnectivity())
    {
    ++rc;
    }

  // STEP 5: Exchange the data
  gridConnectivity->ExchangeData();

  // STEP 6: Get output data
  int gridID = Controller->GetLocalProcessId();
  vtkStructuredGrid* outGrid = vtkStructuredGrid::New();
  gridConnectivity->GetOutputStructuredGrid(gridID,outGrid);

  vtkMultiBlockDataSet *outputMBDS = vtkMultiBlockDataSet::New();
  outputMBDS->SetNumberOfBlocks( numPartitions );
  outputMBDS->SetBlock(gridID,outGrid);
  outGrid->Delete();

  WriteDistributedDataSet("OUTPUT2DYZ",outputMBDS);

  // STEP 7: Verify Test output data
  TestOutput(outputMBDS,wholeExtent);

  // STEP 8: Deallocate
  mbds->Delete();
  outputMBDS->Delete();
  gridConnectivity->Delete();

  return( rc );
}

//------------------------------------------------------------------------------
// Tests StructuredGridConnectivity on a distributed data-set
int TestImplicitGridConnectivity2DXZ( )
{
  assert( "pre: MPI Controller is NULL!" && (Controller != NULL) );

  vtkMPIUtilities::Printf(vtkMPIController::SafeDownCast(Controller),
      "=======================\nTesting 2-D Dataset on the XZ-plane\n");

  int rc = 0;

  int wholeExtent[6];
  wholeExtent[0]   = 0;
  wholeExtent[1]   = 49;
  wholeExtent[2]   = 0;
  wholeExtent[3]   = 0;
  wholeExtent[4]   = 0;
  wholeExtent[5]   = 49;
  double h[3]      = {0.5,0.5,0.5};
  double origin[3] = {0.0,0.0,0.0};

  // STEP 0: We generate the same number of partitions as processes
  int numPartitions = NumberOfProcessors;

  // STEP 1: Acquire the distributed structured grid for this process.
  // Each process has the same number of blocks, but not all entries are
  // populated. A NULL entry indicates that the block belongs to a different
  // process.
  vtkMultiBlockDataSet *mbds =
      GetDataSet( numPartitions,origin,h,wholeExtent );
  Controller->Barrier();
  assert( "pre: mbds != NULL" && (mbds != NULL) );
  assert( "pre: numBlocks mismatch" &&
           (static_cast<int>(mbds->GetNumberOfBlocks())==numPartitions) );
  WriteDistributedDataSet("INPUT2DXZ",mbds);

  // STEP 2: Setup the grid connectivity
  vtkStructuredImplicitConnectivity *gridConnectivity =
      vtkStructuredImplicitConnectivity::New();
  gridConnectivity->SetWholeExtent(
      mbds->GetInformation()->Get(
          vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));


  // STEP 3: Register the grids
  RegisterGrid( mbds, gridConnectivity );
  Controller->Barrier();

  // STEP 4: Compute neighbors
  gridConnectivity->EstablishConnectivity();
  Controller->Barrier();

  // print the neighboring information from each process
  std::ostringstream oss;
  gridConnectivity->Print( oss );
  vtkMPIUtilities::SynchronizedPrintf(
      vtkMPIController::SafeDownCast(Controller),"%s\n",oss.str().c_str());

  if(!gridConnectivity->HasImplicitConnectivity())
    {
    ++rc;
    }

  // STEP 5: Exchange the data
  gridConnectivity->ExchangeData();

  // STEP 6: Get output data
  int gridID = Controller->GetLocalProcessId();
  vtkStructuredGrid* outGrid = vtkStructuredGrid::New();
  gridConnectivity->GetOutputStructuredGrid(gridID,outGrid);

  vtkMultiBlockDataSet *outputMBDS = vtkMultiBlockDataSet::New();
  outputMBDS->SetNumberOfBlocks( numPartitions );
  outputMBDS->SetBlock(gridID,outGrid);
  outGrid->Delete();

  WriteDistributedDataSet("OUTPUT2DXZ",outputMBDS);

  // STEP 7: Verify Test output data
  TestOutput(outputMBDS,wholeExtent);

  // STEP 8: Deallocate
  mbds->Delete();
  outputMBDS->Delete();
  gridConnectivity->Delete();


  return( rc );
}

//------------------------------------------------------------------------------
// Tests StructuredGridConnectivity on a distributed data-set
int TestImplicitGridConnectivity2DXY( )
{
  assert( "pre: MPI Controller is NULL!" && (Controller != NULL) );

  vtkMPIUtilities::Printf(vtkMPIController::SafeDownCast(Controller),
      "=======================\nTesting 2-D Dataset on the XY-plane\n");

  int rc = 0;

  int wholeExtent[6];
  wholeExtent[0]   = 0;
  wholeExtent[1]   = 49;
  wholeExtent[2]   = 0;
  wholeExtent[3]   = 49;
  wholeExtent[4]   = 0;
  wholeExtent[5]   = 0;
  double h[3]      = {0.5,0.5,0.5};
  double origin[3] = {0.0,0.0,0.0};

  // STEP 0: We generate the same number of partitions as processes
  int numPartitions = NumberOfProcessors;

  // STEP 1: Acquire the distributed structured grid for this process.
  // Each process has the same number of blocks, but not all entries are
  // populated. A NULL entry indicates that the block belongs to a different
  // process.
  vtkMultiBlockDataSet *mbds =
      GetDataSet( numPartitions,origin,h,wholeExtent );
  Controller->Barrier();
  assert( "pre: mbds != NULL" && (mbds != NULL) );
  assert( "pre: numBlocks mismatch" &&
           (static_cast<int>(mbds->GetNumberOfBlocks())==numPartitions) );
  WriteDistributedDataSet("INPUT2DXY",mbds);

  // STEP 2: Setup the grid connectivity
  vtkStructuredImplicitConnectivity *gridConnectivity =
      vtkStructuredImplicitConnectivity::New();
  gridConnectivity->SetWholeExtent(
      mbds->GetInformation()->Get(
          vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));

  // STEP 3: Register the grids
  RegisterGrid( mbds, gridConnectivity );
  Controller->Barrier();


  // STEP 4: Compute neighbors
  gridConnectivity->EstablishConnectivity();
  Controller->Barrier();

  // print the neighboring information from each process
  std::ostringstream oss;
  gridConnectivity->Print( oss );
  vtkMPIUtilities::SynchronizedPrintf(
      vtkMPIController::SafeDownCast(Controller),"%s\n",oss.str().c_str());

  if(!gridConnectivity->HasImplicitConnectivity())
    {
    ++rc;
    }

  // STEP 5: Exchange the data
  gridConnectivity->ExchangeData();

  // STEP 6: Get output data
  int gridID = Controller->GetLocalProcessId();
  vtkStructuredGrid* outGrid = vtkStructuredGrid::New();
  gridConnectivity->GetOutputStructuredGrid(gridID,outGrid);

  vtkMultiBlockDataSet *outputMBDS = vtkMultiBlockDataSet::New();
  outputMBDS->SetNumberOfBlocks( numPartitions );
  outputMBDS->SetBlock(gridID,outGrid);
  outGrid->Delete();

  WriteDistributedDataSet("OUTPUT2DXY",outputMBDS);

  // STEP 7: Verify Test output data
  TestOutput(outputMBDS,wholeExtent);

  // STEP 8: Deallocate
  mbds->Delete();
  outputMBDS->Delete();
  gridConnectivity->Delete();

  return( rc );
}


//------------------------------------------------------------------------------
// Tests StructuredGridConnectivity on a distributed data-set
int TestImplicitGridConnectivity3D( )
{
  assert( "pre: MPI Controller is NULL!" && (Controller != NULL) );

  vtkMPIUtilities::Printf(vtkMPIController::SafeDownCast(Controller),
      "=======================\nTesting 3-D Dataset\n");

  int rc = 0;

  int wholeExtent[6];
  wholeExtent[0]   = 0;
  wholeExtent[1]   = 99;
  wholeExtent[2]   = 0;
  wholeExtent[3]   = 99;
  wholeExtent[4]   = 0;
  wholeExtent[5]   = 99;
  double h[3]      = {0.5,0.5,0.5};
  double origin[3] = {0.0,0.0,0.0};

  // STEP 0: We generate the same number of partitions as processes
  int numPartitions = NumberOfProcessors;

  // STEP 1: Acquire the distributed structured grid for this process.
  // Each process has the same number of blocks, but not all entries are
  // populated. A NULL entry indicates that the block belongs to a different
  // process.
  vtkMultiBlockDataSet *mbds =
      GetDataSet( numPartitions,origin,h,wholeExtent );

  Controller->Barrier();
  assert( "pre: mbds != NULL" && (mbds != NULL) );
  assert( "pre: numBlocks mismatch" &&
           (static_cast<int>(mbds->GetNumberOfBlocks())==numPartitions) );
  WriteDistributedDataSet("INPUT3D",mbds);

  // STEP 2: Setup the grid connectivity
  vtkStructuredImplicitConnectivity *gridConnectivity =
      vtkStructuredImplicitConnectivity::New();
  gridConnectivity->SetWholeExtent(
      mbds->GetInformation()->Get(
          vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));

  // STEP 3: Register the grids
  RegisterGrid( mbds, gridConnectivity );
  Controller->Barrier();

  // STEP 4: Compute neighbors
  gridConnectivity->EstablishConnectivity();
  Controller->Barrier();

  // print the neighboring information from each process
  std::ostringstream oss;
  gridConnectivity->Print( oss );
  vtkMPIUtilities::SynchronizedPrintf(
      vtkMPIController::SafeDownCast(Controller),"%s\n",oss.str().c_str());

  if(!gridConnectivity->HasImplicitConnectivity())
    {
    ++rc;
    }

  // STEP 5: Exchange the data
  gridConnectivity->ExchangeData();

  // STEP 6: Get output data
  int gridID = Controller->GetLocalProcessId();
  vtkStructuredGrid* outGrid = vtkStructuredGrid::New();
  gridConnectivity->GetOutputStructuredGrid(gridID,outGrid);

  vtkMultiBlockDataSet *outputMBDS = vtkMultiBlockDataSet::New();
  outputMBDS->SetNumberOfBlocks( numPartitions );
  outputMBDS->SetBlock(gridID,outGrid);
  outGrid->Delete();

  WriteDistributedDataSet("OUTPUT3D",outputMBDS);

  // STEP 7: Verify Test output data
  TestOutput(outputMBDS,wholeExtent);

  // STEP 8: Deallocate
  mbds->Delete();
  outputMBDS->Delete();
  gridConnectivity->Delete();

  return( rc );
}

//------------------------------------------------------------------------------
int TestRectGridImplicitConnectivity3D()
{
  assert( "pre: MPI Controller is NULL!" && (Controller != NULL) );

  vtkMPIUtilities::Printf(vtkMPIController::SafeDownCast(Controller),
      "=======================\nTesting 3-D Rectilinear Grid Dataset\n");

  int rc = 0;

  int wholeExtent[6];
  wholeExtent[0]   = 0;
  wholeExtent[1]   = 99;
  wholeExtent[2]   = 0;
  wholeExtent[3]   = 99;
  wholeExtent[4]   = 0;
  wholeExtent[5]   = 99;
  double origin[3] = {0.0,0.0,0.0};

  // STEP 0: We generate the same number of partitions as processes
  int numPartitions = NumberOfProcessors;

  // STEP 1: Acquire the distributed rectilinear grid for this process.
  // Each process has the same number of blocks, but not all entries are
  // populated. A NULL entry indicates that the block belongs to a different
  // process.
  vtkMultiBlockDataSet *mbds =
      GetRectGridDataSet( numPartitions,origin,wholeExtent );

  Controller->Barrier();
  assert( "pre: mbds != NULL" && (mbds != NULL) );
  assert( "pre: numBlocks mismatch" &&
           (static_cast<int>(mbds->GetNumberOfBlocks())==numPartitions) );
  WriteDistributedDataSet("INPUT-3D-RECTGRID",mbds);

  // STEP 2: Setup the grid connectivity
  vtkStructuredImplicitConnectivity *gridConnectivity =
      vtkStructuredImplicitConnectivity::New();
  gridConnectivity->SetWholeExtent(
      mbds->GetInformation()->Get(
          vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));

  // STEP 3: Register the grids
  RegisterRectGrid( mbds, gridConnectivity );
  Controller->Barrier();

  // STEP 4: Compute neighbors
  gridConnectivity->EstablishConnectivity();
  Controller->Barrier();

  // print the neighboring information from each process
  std::ostringstream oss;
  gridConnectivity->Print( oss );
  vtkMPIUtilities::SynchronizedPrintf(
      vtkMPIController::SafeDownCast(Controller),"%s\n",oss.str().c_str());

  if(!gridConnectivity->HasImplicitConnectivity())
    {
    ++rc;
    }

  // STEP 5: Exchange the data
  gridConnectivity->ExchangeData();

  // STEP 6: Get output data
  int gridID = Controller->GetLocalProcessId();
  vtkRectilinearGrid* outGrid = vtkRectilinearGrid::New();
  gridConnectivity->GetOutputRectilinearGrid(gridID,outGrid);

  vtkMultiBlockDataSet *outputMBDS = vtkMultiBlockDataSet::New();
  outputMBDS->SetNumberOfBlocks( numPartitions );
  outputMBDS->SetBlock(gridID,outGrid);
  outGrid->Delete();

  WriteDistributedDataSet("OUTPUT-3D-RECTGRID",outputMBDS);

  // STEP 7: Verify Test output data
  TestOutput(outputMBDS,wholeExtent);

  // STEP 8: Deallocate
  mbds->Delete();
  outputMBDS->Delete();
  gridConnectivity->Delete();

  return( rc );
}

//------------------------------------------------------------------------------
// Program main
int TestImplicitConnectivity( int argc, char *argv[] )
{
  int rc = 0;

  // STEP 0: Initialize
  Controller = vtkMPIController::New();
  Controller->Initialize( &argc, &argv, 0 );
  assert("pre: Controller should not be NULL" && (Controller != NULL) );
  vtkMultiProcessController::SetGlobalController( Controller );

  Rank               = Controller->GetLocalProcessId();
  NumberOfProcessors = Controller->GetNumberOfProcesses();
  vtkMPIUtilities::Printf(vtkMPIController::SafeDownCast(Controller),
      "Rank=%d NumRanks=%d\n",Rank,NumberOfProcessors);
  assert( "pre: NumberOfProcessors >= 1" && (NumberOfProcessors >= 1) );
  assert( "pre: Rank is out-of-bounds" && (Rank >= 0) );

  // STEP 1: Run 2-D Test on XY-plane
  rc += TestImplicitGridConnectivity2DXY();
  Controller->Barrier();

  // STEP 2: Run 2-D Test on XZ-plane
  rc += TestImplicitGridConnectivity2DXZ();
  Controller->Barrier();

  // STEP 3: Run 2-D Test on YZ-plane
  rc += TestImplicitGridConnectivity2DYZ();
  Controller->Barrier();

  // STEP 2: Run 3-D Test
  rc += TestImplicitGridConnectivity3D();
  Controller->Barrier();

  // STEP 3: Test 3-D Recilinear Grid
  rc += TestRectGridImplicitConnectivity3D();
  Controller->Barrier();

  // STEP 3: Deallocate controller and exit
  vtkMPIUtilities::Printf(vtkMPIController::SafeDownCast(Controller),
      "Finalizing...\n");
  Controller->Finalize();
  Controller->Delete();

  if( rc != 0 )
    {
    std::cout << "Test Failed!\n";
    rc = 0;
    }
  return( rc );
}
