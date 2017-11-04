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

// VTK includes
#include "vtkDataSet.h"
#include "vtkUniformGrid.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkDataObject.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkStructuredGridConnectivity.h"
#include "vtkStructuredNeighbor.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIntArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkCell.h"
#include "vtkUniformGridPartitioner.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkXMLMultiBlockDataWriter.h"
#include "vtkMathUtilities.h"

// C++ includes
#include <iostream>
#include <sstream>
#include <cassert>
#include <string>
#include <vector>
#include <set>

//#define ENABLE_IO

namespace
{

//------------------------------------------------------------------------------
// Description:
// This method attaches a point array to the given grid that will label the
// the points by color -- 0(off) or 1(on) -- to indicate whether or not a
// particular flag is "ON"
void AttachPointFlagsArray(
    vtkUniformGrid *grid, const int flag, const std::string& label )
{
  assert( "pre: grid should not be nullptr!" && (grid != nullptr) );

  vtkUnsignedIntArray *flags = vtkUnsignedIntArray::New();
  flags->SetName( label.c_str() ) ;
  flags->SetNumberOfComponents( 1 );
  flags->SetNumberOfTuples( grid->GetNumberOfPoints() );

  vtkIdType pidx = 0;
  for( ; pidx < grid->GetNumberOfPoints(); ++pidx )
  {
    unsigned char nodeProperty =
         *(grid->GetPointGhostArray()->GetPointer( pidx ));
    if( nodeProperty & flag )
    {
      flags->SetValue( pidx, 1);
    }
    else
    {
      flags->SetValue( pidx, 0);
    }
  } // END for all points

  grid->GetPointData()->AddArray( flags );
  flags->Delete();
}

//------------------------------------------------------------------------------
// Description:
// This method attaches a cell array to the given grid that will label the
// the points by color -- 0(off) or 1(on) -- to indicate whether or not a
// particular flag is "ON"
void AttachCellFlagsArray(
    vtkUniformGrid *grid, const int flag, const std::string& label )
{
  assert( "pre: grid should not be nullptr" && (grid != nullptr) );

  vtkUnsignedIntArray *flags = vtkUnsignedIntArray::New();
  flags->SetName( label.c_str() );
  flags->SetNumberOfComponents( 1 );
  flags->SetNumberOfTuples( grid->GetNumberOfCells() );

  vtkIdType cellIdx = 0;
  for( ; cellIdx < grid->GetNumberOfCells(); ++cellIdx )
  {
    unsigned char cellProperty =
        *(grid->GetCellGhostArray()->GetPointer(cellIdx));
    if( cellProperty & flag )
    {
      flags->SetValue( cellIdx, 1 );
    }
    else
    {
      flags->SetValue( cellIdx, 0 );
    }
  } // END for all cells

  grid->GetCellData()->AddArray( flags );
  flags->Delete();
}

//------------------------------------------------------------------------------
// Description:
// This method loops through all the blocks in the dataset and attaches arrays
// for each ghost property that label whether a property is off(0) or on(1).
void AttachNodeAndCellGhostFlags( vtkMultiBlockDataSet *mbds )
{
  assert( "pre: Multi-block is nullptr!" && (mbds != nullptr) );
  unsigned int block=0;
  for( ; block < mbds->GetNumberOfBlocks(); ++block )
  {
    vtkUniformGrid *myGrid =
       vtkUniformGrid::SafeDownCast(mbds->GetBlock( block ) );
    if( myGrid != nullptr )
    {
      // original IGNORE
      AttachPointFlagsArray( myGrid, vtkDataSetAttributes::DUPLICATEPOINT,
                             "DUPLICATEPOINT" );
      // original DUPLICATE
      AttachCellFlagsArray( myGrid, vtkDataSetAttributes::DUPLICATECELL,
                            "DUPLICATECELL" );
    }
  } // END for all blocks
}

//------------------------------------------------------------------------------
// Description:
// Applies an XYZ field to the nodes and cells of the grid whose value is
// corresponding to the XYZ coordinates at that location
void ApplyXYZFieldToGrid( vtkUniformGrid *grd, const std::string &prefix )
{
  assert( "pre: grd should not be nullptr" && (grd != nullptr)  );

  // Get the grid's point-data and cell-data data-structures
  vtkCellData  *CD = grd->GetCellData();
  vtkPointData *PD = grd->GetPointData();
  assert( "pre: Cell data is nullptr" && (CD != nullptr) );
  assert( "pre: Point data is nullptr" && (PD != nullptr)  );

  std::ostringstream oss;

  // Allocate arrays
  oss.str("");
  oss << prefix << "-CellXYZ";
  vtkDoubleArray *cellXYZArray = vtkDoubleArray::New();
  cellXYZArray->SetName( oss.str().c_str() );
  cellXYZArray->SetNumberOfComponents( 3 );
  cellXYZArray->SetNumberOfTuples( grd->GetNumberOfCells() );


  oss.str("");
  oss << prefix << "-NodeXYZ";
  vtkDoubleArray *nodeXYZArray = vtkDoubleArray::New();
  nodeXYZArray->SetName( oss.str().c_str() );
  nodeXYZArray->SetNumberOfComponents( 3 );
  nodeXYZArray->SetNumberOfTuples( grd->GetNumberOfPoints() );

  // Compute field arrays
  std::set< vtkIdType > visited;
  for( vtkIdType cellIdx=0; cellIdx < grd->GetNumberOfCells(); ++cellIdx )
  {
    vtkCell *c = grd->GetCell( cellIdx );
    assert( "pre: cell is not nullptr" && (c != nullptr) );

    double centroid[3];
    double xsum = 0.0;
    double ysum = 0.0;
    double zsum = 0.0;

    for( vtkIdType node=0; node < c->GetNumberOfPoints(); ++node )
    {
      double xyz[3];

      vtkIdType meshPntIdx = c->GetPointId( node );
      grd->GetPoint(  meshPntIdx, xyz );
      xsum += xyz[0];
      ysum += xyz[1];
      zsum += xyz[2];

      if( visited.find( meshPntIdx ) == visited.end() )
      {
        visited.insert( meshPntIdx );


        nodeXYZArray->SetComponent( meshPntIdx, 0, xyz[0] );
        nodeXYZArray->SetComponent( meshPntIdx, 1, xyz[1] );
        nodeXYZArray->SetComponent( meshPntIdx, 2, xyz[2] );
      } // END if
    } // END for all nodes

    centroid[0] = xsum / c->GetNumberOfPoints();
    centroid[1] = ysum / c->GetNumberOfPoints();
    centroid[2] = zsum / c->GetNumberOfPoints();

    cellXYZArray->SetComponent( cellIdx, 0, centroid[0] );
    cellXYZArray->SetComponent( cellIdx, 1, centroid[1] );
    cellXYZArray->SetComponent( cellIdx, 2, centroid[2] );
  } // END for all cells

  // Insert field arrays to grid point/cell data
  CD->AddArray( cellXYZArray );
  cellXYZArray->Delete();

  PD->AddArray( nodeXYZArray );
  nodeXYZArray->Delete();
}

//------------------------------------------------------------------------------
void ApplyFieldsToDataSet( vtkMultiBlockDataSet *mbds, const std::string &prefix )
{
  unsigned int block = 0;
  for( ; block < mbds->GetNumberOfBlocks(); ++block )
  {
    vtkUniformGrid *grid =
        vtkUniformGrid::SafeDownCast( mbds->GetBlock(block) );
    ApplyXYZFieldToGrid( grid, prefix );
  } // END for all blocks
}

//------------------------------------------------------------------------------
// Description:
// Get Grid whole extent and dimensions
void GetGlobalGrid( const int dimension, int wholeExtent[6], int dims[3] )
{
  for( int i=0; i < 3; ++i )
  {
    wholeExtent[ i*2   ] = 0;
    wholeExtent[ i*2+1 ] = 0;
    dims[ i ]            = 1;
  }

  switch( dimension )
  {
    case 2:
      wholeExtent[0] = 0;
      wholeExtent[1] = 9;
      wholeExtent[2] = 0;
      wholeExtent[3] = 9;

      dims[0] = wholeExtent[1] - wholeExtent[0] + 1;
      dims[1] = wholeExtent[3] - wholeExtent[2] + 1;
      break;
    case 3:
      wholeExtent[0] = 0;
      wholeExtent[1] = 9;
      wholeExtent[2] = 0;
      wholeExtent[3] = 9;
      wholeExtent[4] = 0;
      wholeExtent[5] = 9;

      dims[0] = wholeExtent[1] - wholeExtent[0] + 1;
      dims[1] = wholeExtent[3] - wholeExtent[2] + 1;
      dims[2] = wholeExtent[5] - wholeExtent[4] + 1;
      break;
    default:
      assert( "Cannot create grid of invalid dimension" && false );
  }
}

//------------------------------------------------------------------------------
// Description:
// Generates a multi-block dataset
vtkMultiBlockDataSet* GetDataSet(
    const int dimension, const int numPartitions, const int numGhosts)
{
  int wholeExtent[6];
  int dims[3];
  GetGlobalGrid( dimension, wholeExtent, dims );


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
  gridPartitioner->SetNumberOfGhostLayers( numGhosts );
  gridPartitioner->Update();

// THIS DOES NOT COPY THE INFORMATION KEYS!
//  vtkMultiBlockDataSet *mbds = vtkMultiBlockDataSet::New();
//  mbds->ShallowCopy( gridPartitioner->GetOutput() );

  vtkMultiBlockDataSet *mbds =
      vtkMultiBlockDataSet::SafeDownCast( gridPartitioner->GetOutput() );
  mbds->SetReferenceCount( mbds->GetReferenceCount()+1 );
  ApplyFieldsToDataSet( mbds, "COMPUTED" );

  wholeGrid->Delete();
  gridPartitioner->Delete();

  assert( "pre: mbds is nullptr" && (mbds != nullptr) );
  return( mbds );
}

//------------------------------------------------------------------------------
// Description:
// Computes the total number of nodes in the multi-block dataset.
int GetTotalNumberOfNodes( vtkMultiBlockDataSet *multiblock )
{
  assert( "multi-block grid is nullptr" && (multiblock != nullptr) );

  int numNodes = 0;

  for(unsigned int block=0; block < multiblock->GetNumberOfBlocks(); ++block )
  {
    vtkUniformGrid *grid =
        vtkUniformGrid::SafeDownCast( multiblock->GetBlock( block ) );

    if( grid != nullptr )
    {
      vtkIdType pntIdx = 0;
      for( ; pntIdx < grid->GetNumberOfPoints(); ++pntIdx )
      {
        unsigned char nodeProperty =
            *(grid->GetPointGhostArray()->GetPointer( pntIdx ));
        if( ! (nodeProperty & (vtkDataSetAttributes::DUPLICATEPOINT |
                               vtkDataSetAttributes::HIDDENPOINT)))
        {
          ++numNodes;
        }
      } // END for all nodes
    } // END if grid != nullptr

  } // END for all blocks

  return( numNodes );
}

//------------------------------------------------------------------------------
// Description:
// Computes the total number of nodes in the multi-block dataset.
int GetTotalNumberOfCells( vtkMultiBlockDataSet *multiblock )
{
  assert( "multi-block grid is nullptr" && (multiblock != nullptr) );

  int numCells = 0;

  for( unsigned int block=0; block < multiblock->GetNumberOfBlocks(); ++block )
  {
    vtkUniformGrid *grid =
        vtkUniformGrid::SafeDownCast( multiblock->GetBlock( block ) );

    if( grid != nullptr )
    {
      vtkIdType cellIdx = 0;
      for( ; cellIdx < grid->GetNumberOfCells(); ++cellIdx )
      {
        unsigned char cellProperty =
            *(grid->GetCellGhostArray()->GetPointer( cellIdx ) );
        if( ! (cellProperty & vtkDataSetAttributes::DUPLICATECELL) )

        {
          ++numCells;
        }
      } // END for all cells
    } // END if grid != nullptr
  } // END for all blocks
  return( numCells );
}

//------------------------------------------------------------------------------
void RegisterGrids(
    vtkMultiBlockDataSet *mbds, vtkStructuredGridConnectivity *connectivity )
{
  assert( "pre: Multi-block is nullptr!" && (mbds != nullptr) );
  assert( "pre: connectivity is nullptr!" && (connectivity != nullptr) );

  for( unsigned int block=0; block < mbds->GetNumberOfBlocks(); ++block )
  {
    vtkUniformGrid *grid = vtkUniformGrid::SafeDownCast(mbds->GetBlock(block));
    assert( "pre: grid should not be nullptr!" && (grid != nullptr) );
    grid->AllocatePointGhostArray();
    grid->AllocateCellGhostArray();

    vtkInformation *info = mbds->GetMetaData( block );
    assert( "pre: metadata should not be nullptr" && (info != nullptr) );
    assert( "pre: must have piece extent!" &&
            info->Has(vtkDataObject::PIECE_EXTENT() ) );

    connectivity->RegisterGrid(
        block,info->Get(vtkDataObject::PIECE_EXTENT()),
        grid->GetPointGhostArray(),
        grid->GetCellGhostArray(),
        grid->GetPointData(),
        grid->GetCellData(),
        nullptr);
  } // END for all blocks
}

//------------------------------------------------------------------------------
void WriteMultiBlock( vtkMultiBlockDataSet *mbds, const std::string &prefix )
{


  assert( "pre: Multi-block is nullptr!" && (mbds != nullptr) );

  vtkXMLMultiBlockDataWriter *writer = vtkXMLMultiBlockDataWriter::New();
  assert( "pre: Cannot allocate writer" && (writer != nullptr) );

  std::ostringstream oss;
  oss.str("");
  oss << prefix << mbds->GetNumberOfBlocks() << "."
      << writer->GetDefaultFileExtension();
  writer->SetFileName( oss.str().c_str() );
  writer->SetInputData( mbds );
#ifdef ENABLE_IO
  writer->Write();
#endif
  writer->Delete();
}

//------------------------------------------------------------------------------
vtkUniformGrid* GetGhostedGridFromGrid( vtkUniformGrid *grid, int gext[6] )
{
  assert( "pre: input grid is nullptr" && (grid != nullptr) );
  vtkUniformGrid *newGrid = vtkUniformGrid::New();

  int dims[3];
  vtkStructuredData::GetDimensionsFromExtent( gext, dims );

  double h[3];
  grid->GetSpacing( h );

  double origin[3];
  for( int i=0; i < 3; ++i )
  {
    // Assumes a global origing @(0,0,0)
    origin[ i ] = 0.0 + gext[i*2]*h[i];
  }

  newGrid->SetOrigin( origin );
  newGrid->SetDimensions( dims );
  newGrid->SetSpacing( grid->GetSpacing() );
  return( newGrid );
}

//------------------------------------------------------------------------------
vtkMultiBlockDataSet* GetGhostedDataSet(
    vtkMultiBlockDataSet *mbds,
    vtkStructuredGridConnectivity *SGC, int numGhosts )
{
  assert( "pre: Multi-block dataset is not nullptr" && (mbds != nullptr) );
  assert( "pre: SGC is nullptr" && (SGC != nullptr) );
  assert( "pre: Number of ghosts requested is invalid" && (numGhosts >= 1) );
  assert( "pre: Number of blocks in input must match registered grids!" &&
          mbds->GetNumberOfBlocks()==SGC->GetNumberOfGrids() );

  vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::New();
  output->SetNumberOfBlocks( mbds->GetNumberOfBlocks() );

  SGC->CreateGhostLayers( numGhosts );

  int GhostedGridExtent[6];
  int GridExtent[6];
  unsigned int block=0;
  for( ; block < output->GetNumberOfBlocks(); ++block )
  {
    vtkUniformGrid *grid =
        vtkUniformGrid::SafeDownCast( mbds->GetBlock(block) );
    assert( "pre: Uniform grid should not be nullptr" && (grid != nullptr) );


    SGC->GetGridExtent( block, GridExtent );
    SGC->GetGhostedGridExtent( block, GhostedGridExtent );

    vtkUniformGrid *ghostedGrid= GetGhostedGridFromGrid(grid,GhostedGridExtent);
    assert( "pre:ghosted grid is nullptr!" && (ghostedGrid != nullptr) );

    // Copy the point data
    ghostedGrid->GetPointData()->DeepCopy(
        SGC->GetGhostedGridPointData(block) );
    ghostedGrid->GetCellData()->DeepCopy(
        SGC->GetGhostedGridCellData(block) );

    // Copy the ghost arrays
    vtkUnsignedCharArray* ghosts = SGC->GetGhostedPointGhostArray(block);
    ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
    ghostedGrid->GetPointData()->AddArray(ghosts);
    ghosts = SGC->GetGhostedCellGhostArray(block);
    ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
    ghostedGrid->GetCellData()->AddArray(ghosts);

    output->SetBlock( block, ghostedGrid );
    ghostedGrid->Delete();
  } // END for all blocks

  return( output );
}

//------------------------------------------------------------------------------
bool Check(
    const std::string &name, const int val, const int expected, bool verbose=true )
{
  bool status = false;

  if( verbose )
  {
    std::cout << name << "=" << val << " EXPECTED=" << expected << "...";
    std::cout.flush();
  }
  if( val == expected )
  {
    if( verbose )
    {
      std::cout << "[OK]\n";
      std::cout.flush();
    }
    status = true;
  }
  else
  {
    if( verbose )
    {
      std::cout << "[ERROR]!\n";
      std::cout.flush();
    }
    status = false;
  }

  return( status );
}

//------------------------------------------------------------------------------
// Program main
int TestStructuredGridConnectivity_internal( int argc, char *argv[] )
{
  // Silence compiler wanrings for unused vars argc and argv
  static_cast<void>( argc );
  static_cast<void>( argv );

  int expected      = 10*10*10;
  int expectedCells = 9*9*9;
  int rc = 0;
  int numberOfPartitions[] = {4};
  int numGhostLayers[]     = {1};
//  int numberOfPartitions[] = { 2, 4, 8, 16, 32, 64, 128, 256 };
//  int numGhostLayers[]     = { 0, 1, 2, 3 };

  for( int i=0; i < 1; ++i )
  {
    for( int j=0; j < 1; ++j )
    {
      // STEP 0: Construct the dataset
      vtkMultiBlockDataSet *mbds =
          GetDataSet(3,numberOfPartitions[ i ], numGhostLayers[ j ] );
      assert( "pre: multi-block is nullptr" && (mbds != nullptr) );
      assert( "pre: NumBlocks mismatch!" &&
       (numberOfPartitions[i] ==static_cast<int>(mbds->GetNumberOfBlocks()) ) );

      // STEP 1: Construct the grid connectivity
      vtkStructuredGridConnectivity *gridConnectivity=
          vtkStructuredGridConnectivity::New();
      gridConnectivity->SetNumberOfGrids( mbds->GetNumberOfBlocks() );
      gridConnectivity->SetNumberOfGhostLayers( numGhostLayers[j] );
      int ext[6];
      mbds->GetInformation()->Get(
          vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),ext);
      gridConnectivity->SetWholeExtent( ext );

      // STEP 2: Registers the grids
      RegisterGrids( mbds, gridConnectivity );

      // STEP 3: Compute neighbors
      gridConnectivity->ComputeNeighbors();

      // STEP 5: Compute total number of nodes & compare to expected
      int NumNodes = GetTotalNumberOfNodes( mbds );
      if( !Check( "NODES", NumNodes, expected ) )
      {
        ++rc;
      }

      // STEP 6: Compute total number of cells & compare to expected
      int NumCells = GetTotalNumberOfCells( mbds );
      if( !Check( "CELLS", NumCells, expectedCells ) )
      {
        ++rc;
      }

      if(rc != 0)
      {
        mbds->Delete();
        gridConnectivity->Delete();
        return( rc );
      }

      // STEP 7: Create one layer of additional ghost nodes
      vtkMultiBlockDataSet *gmbds = GetGhostedDataSet(mbds,gridConnectivity,1);

      // STEP 8: Ensure number of nodes/cells is the same on ghosted dataset
      int GhostedNumNodes = GetTotalNumberOfNodes( gmbds );
      int GhostedNumCells = GetTotalNumberOfCells( gmbds );
      if( !Check( "GHOSTED_NODES", GhostedNumNodes, expected ) )
      {
        ++rc;
      }
      if( !Check( "GHOSTED_CELLS", GhostedNumCells, expectedCells ) )
      {
        ++rc;
      }

      // STEP 9: De-allocated data-structures
      gmbds->Delete();
      mbds->Delete();
      gridConnectivity->Delete();

      if(rc != 0)
      {
        return rc;
      }
    }// END for all ghost layer tests
  } // END for all numPartition tests

  return( rc );
}

//------------------------------------------------------------------------------
bool CheckArrays( vtkDoubleArray *computed, vtkDoubleArray *expected )
{
  std::cout << "Checking " << computed->GetName();
  std::cout << " to "      << expected->GetName() << std::endl;
  std::cout.flush();

  if( computed->GetNumberOfComponents() != expected->GetNumberOfComponents() )
  {
    std::cout << "Number of components mismatch!\n";
    std::cout.flush();
    return false;
  }

  if( computed->GetNumberOfTuples() != expected->GetNumberOfTuples() )
  {
    std::cout << "Number of tuples mismatch!\n";
    std::cout.flush();
    return false;
  }

  bool status   = true;
  vtkIdType idx = 0;
  for( ; idx < computed->GetNumberOfTuples(); ++idx )
  {
    int comp = 0;
    for( ; comp < computed->GetNumberOfComponents(); ++comp )
    {
      double compVal = computed->GetComponent(idx,comp);
      double expVal  = expected->GetComponent(idx,comp);
      if( !vtkMathUtilities::FuzzyCompare(compVal,expVal) )
      {
//        std::cerr << "ERROR: " << compVal << " != " << expVal << std::endl;
//        std::cerr.flush();
        status = false;
      }
    } // END for all components
  } // END for all tuples

  return( status );
}

//------------------------------------------------------------------------------
bool CompareFieldsForGrid( vtkUniformGrid *grid )
{
  // Sanity check
  assert( "pre: grid should not be nullptr" && (grid != nullptr) );
  assert( "pre: COMPUTED-CellXYZ array is expected!" &&
           grid->GetCellData()->HasArray( "COMPUTED-CellXYZ" ) );
  assert( "pre: EXPECTED-CellXYZ array is expected!" &&
           grid->GetCellData()->HasArray( "EXPECTED-CellXYZ" ) );
  assert( "pre: COMPUTED-NodeXYZ array is expected!" &&
           grid->GetPointData()->HasArray( "COMPUTED-NodeXYZ" ) );
  assert( "pre: EXPECTED-NodeXYZ array is expected!" &&
           grid->GetPointData()->HasArray( "EXPECTED-NodeXYZ" ) );

  vtkDoubleArray *computedCellData =
      vtkArrayDownCast<vtkDoubleArray>(
          grid->GetCellData()->GetArray( "COMPUTED-CellXYZ" ) );
  assert( "pre: computedCellData is nullptr" && (computedCellData != nullptr) );

  vtkDoubleArray *expectedCellData =
      vtkArrayDownCast<vtkDoubleArray>(
          grid->GetCellData()->GetArray( "EXPECTED-CellXYZ" ) );
  assert( "pre: expectedCellData is nullptr" && (expectedCellData != nullptr) );

  bool status = CheckArrays( computedCellData, expectedCellData );
  if( !status )
  {
    return status;
  }

  vtkDoubleArray *computedPointData =
      vtkArrayDownCast<vtkDoubleArray>(
          grid->GetPointData()->GetArray( "COMPUTED-NodeXYZ" ) );
  assert( "pre: computePointData is nullptr" && (computedPointData != nullptr) );

  vtkDoubleArray *expectedPointData =
      vtkArrayDownCast<vtkDoubleArray>(
          grid->GetPointData()->GetArray( "EXPECTED-NodeXYZ" ) );
  assert( "pre: expectedPointData is nullptr" && (expectedPointData != nullptr) );

  status = CheckArrays( computedPointData, expectedPointData );
  return( status );
}

//------------------------------------------------------------------------------
bool CompareFields( vtkMultiBlockDataSet *mbds )
{
  bool status = true;
  unsigned int block = 0;
  for( ; block < mbds->GetNumberOfBlocks(); ++block )
  {
    vtkUniformGrid *grid= vtkUniformGrid::SafeDownCast( mbds->GetBlock(block) );
    status = CompareFieldsForGrid( grid );
  } // END for all blocks

  return( status );
}

//------------------------------------------------------------------------------
int SimpleTest( int argc, char **argv )
{
  assert( "pre: argument counter must equal 4" && (argc==5) );

  // Doing a void cast here to resolve warnings on unused vars
  static_cast<void>(argc);

  int dim = atoi( argv[1] ); // The dimension of the data
  int np  = atoi( argv[2] ); // The number of partitions to create
  int ng  = atoi( argv[3] ); // The number of initial ghost layers
  int nng = atoi( argv[4] ); // The number of additional ghost layers to create

  assert( "pre: dim must be 2 or 3" && ( (dim==2) || (dim==3) ) );

  std::cout << "Running Simple " << dim << "-D Test..." << std::endl;
  std::cout << "Number of partitions: "   << np << std::endl;
  std::cout << "Number of ghost-layers: " << ng << std::endl;
  std::cout.flush();

  int expected      = 0;
  int expectedCells = 0;
  switch( dim )
  {
    case 2:
      expectedCells = 9*9;
      expected      = 10*10;
      break;
    case 3:
      expectedCells = 9*9*9;
      expected      = 10*10*10;
      break;
    default:
      assert( "Code should not reach here!" && false );
  }


  vtkMultiBlockDataSet *mbds = GetDataSet(dim, np, ng);

  vtkStructuredGridConnectivity *gridConnectivity =
      vtkStructuredGridConnectivity::New();
  gridConnectivity->SetNumberOfGhostLayers( ng );
  gridConnectivity->SetNumberOfGrids( mbds->GetNumberOfBlocks() );

  int wholeExt[6];
  mbds->GetInformation()->Get(
      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExt);
  gridConnectivity->SetWholeExtent(wholeExt);

  RegisterGrids( mbds, gridConnectivity );

  gridConnectivity->ComputeNeighbors();
  gridConnectivity->Print( std::cout );
  std::cout.flush();

  AttachNodeAndCellGhostFlags( mbds );
  WriteMultiBlock( mbds, "INITIAL" );

  int NumNodes = GetTotalNumberOfNodes( mbds );
  int NumCells = GetTotalNumberOfCells( mbds );
  std::cout << "[DONE]\n";
  std::cout.flush();

  Check( "NODES", NumNodes, expected );
  Check( "CELLS", NumCells, expectedCells );

  std::cout << "Creating/Extending ghost layers...";
  std::cout.flush();
  vtkMultiBlockDataSet *gmbds = GetGhostedDataSet( mbds,gridConnectivity,nng );
  std::cout << "[DONE]\n";
  std::cout.flush();

  std::cout << "Ghosted Grid connectivity:\n";
  std::cout.flush();
  gridConnectivity->Print( std::cout );
  std::cout.flush();

  int NumNodesOnGhostedDataSet = GetTotalNumberOfNodes( gmbds );
  int NumCellsOnGhostedDataSet = GetTotalNumberOfCells( gmbds );

  Check( "GHOSTED_NODES", NumNodesOnGhostedDataSet, expected, true );
  Check( "GHOSTED_CELLS", NumCellsOnGhostedDataSet, expectedCells, true );
  AttachNodeAndCellGhostFlags( gmbds );

  ApplyFieldsToDataSet( gmbds, "EXPECTED" );
  bool success = CompareFields( gmbds );
  WriteMultiBlock( gmbds, "GHOSTED" );

  if( !success )
  {
    std::cerr << "FIELD COMPARISON FAILED!\n";
  }

  gmbds->Delete();
  mbds->Delete();
  gridConnectivity->Delete();
  return 0;
}

}

//------------------------------------------------------------------------------
// Program main
int TestStructuredGridConnectivity( int argc, char *argv[] )
{
  int rc = 0;

  if( argc > 1 )
  {
    rc = SimpleTest( argc, argv );
  }
  else
  {
    rc = TestStructuredGridConnectivity_internal( argc, argv );
  }
  return( rc );
}
