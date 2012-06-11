/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStructuredGridGhostDataGenerator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestStructuredGridGhostDataGenerator.cxx -- Tests generation of ghost
//  data for structured grids.
//
// .SECTION Description
//  Serial tests for 2-D and 3-D ghost data generation of multi-block structured
//  grid datasets. The tests apply an XYZ field to the nodes and cells of the
//  domain and ensure that the created ghost data have the correct fields.

// C++ includes
#include <iostream>
#include <sstream>
#include <cassert>
#include <string>
#include <vector>
#include <set>

// VTK includes
#include "vtkDataSet.h"
#include "vtkStructuredGrid.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStructuredGridPartitioner.h"
#include "vtkStructuredGridGhostDataGenerator.h"
#include "vtkXMLMultiBlockDataWriter.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkCell.h"
#include "vtkDoubleArray.h"
#include "vtkMathUtilities.h"
#include "vtkUniformGrid.h"
#include "vtkImageToStructuredGrid.h"


//#define DEBUG_ON

//------------------------------------------------------------------------------
// Description:
// Write the uniform grid multi-block dataset into an XML file.
void WriteMultiBlock( vtkMultiBlockDataSet *mbds, std::string prefix )
{
#ifdef DEBUG_ON
  assert( "pre: Multi-block is NULL!" && (mbds != NULL) );

  vtkXMLMultiBlockDataWriter *writer = vtkXMLMultiBlockDataWriter::New();
  assert( "pre: Cannot allocate writer" && (writer != NULL) );

  std::ostringstream oss;
  oss.str("");
  oss << prefix << mbds->GetNumberOfBlocks() << "."
      << writer->GetDefaultFileExtension();
  writer->SetFileName( oss.str().c_str() );
  writer->SetInputData( mbds );
  writer->Write();

  writer->Delete();
#else
  (void)(prefix);
  (void)(mbds);
#endif
}

//------------------------------------------------------------------------------
bool CheckNodeFieldsForGrid( vtkStructuredGrid *grid )
{
  assert("pre: grid should not be NULL" && (grid != NULL) );
  assert("pre: grid should have a NODE-XYZ array" &&
          grid->GetPointData()->HasArray("NODE-XYZ") );

  double xyz[3];
  vtkDoubleArray *array =
     vtkDoubleArray::SafeDownCast( grid->GetPointData()->GetArray("NODE-XYZ") );
  assert("pre: num tuples must match number of nodes" &&
         (array->GetNumberOfTuples() == grid->GetNumberOfPoints()) );
  assert("pre: num components must be 3" &&
         (array->GetNumberOfComponents()==3));

  for( vtkIdType idx=0; idx < grid->GetNumberOfPoints(); ++idx )
    {
    grid->GetPoint( idx, xyz );

    for( int i=0; i < 3; ++i )
      {
      if( !vtkMathUtilities::FuzzyCompare(xyz[i],array->GetComponent(idx,i) ) )
        {
        std::cout << "Node Data mismatch: " << xyz[i] << " ";
        std::cout <<  array->GetComponent(idx,i);
        std::cout << std::endl;
        std::cout.flush();
        return false;
        } // END if fuzzy-compare
      } // END for all components
    } // END for all nodes
  return true;
}

//------------------------------------------------------------------------------
bool CheckCellFieldsForGrid( vtkStructuredGrid *grid )
{
  assert("pre: grid should not be NULL" && (grid != NULL) );
  assert("pre: grid should have a NODE-XYZ array" &&
          grid->GetCellData()->HasArray("CELL-XYZ") );

  double centroid[3];
  double xyz[3];
  vtkDoubleArray *array =
      vtkDoubleArray::SafeDownCast(grid->GetCellData()->GetArray("CELL-XYZ") );
  assert("pre: num tuples must match number of nodes" &&
         (array->GetNumberOfTuples() == grid->GetNumberOfCells()) );
  assert("pre: num components must be 3" &&
         (array->GetNumberOfComponents()==3));

  vtkIdList *nodeIds = vtkIdList::New();
  for( vtkIdType cellIdx=0; cellIdx < grid->GetNumberOfCells(); ++cellIdx )
    {
    nodeIds->Initialize();
    grid->GetCellPoints(cellIdx,nodeIds);
    double xsum = 0.0;
    double ysum = 0.0;
    double zsum = 0.0;
    for( vtkIdType node=0; node < nodeIds->GetNumberOfIds(); ++node )
      {
      vtkIdType meshPntIdx = nodeIds->GetId( node );
      grid->GetPoint( meshPntIdx, xyz );
      xsum += xyz[0];
      ysum += xyz[1];
      zsum += xyz[2];
      } // END for all nodes

    centroid[0] = centroid[1] = centroid[2] = 0.0;
    centroid[0] = xsum / static_cast<double>( nodeIds->GetNumberOfIds() );
    centroid[1] = ysum / static_cast<double>( nodeIds->GetNumberOfIds() );
    centroid[2] = zsum / static_cast<double>( nodeIds->GetNumberOfIds() );

    for( int i=0; i < 3; ++i )
      {
      if( !vtkMathUtilities::FuzzyCompare(
          centroid[i],array->GetComponent(cellIdx,i)) )
        {
        std::cout << "Cell Data mismatch: " << centroid[i] << " ";
        std::cout <<  array->GetComponent(cellIdx,i);
        std::cout << std::endl;
        std::cout.flush();
        nodeIds->Delete();
        return false;
        } // END if fuzz-compare
      } // END for all components
    } // END for all cells
  nodeIds->Delete();
  return true;
}

//------------------------------------------------------------------------------
int CheckFields( vtkMultiBlockDataSet *mbds,bool hasNodeData,bool hasCellData )
{
  assert("pre: input multi-block is NULL" && (mbds != NULL) );

  if( !hasNodeData && !hasCellData )
    {
    return 0;
    }

  for(unsigned int block=0; block < mbds->GetNumberOfBlocks(); ++block )
    {
    vtkStructuredGrid *grid =
        vtkStructuredGrid::SafeDownCast(mbds->GetBlock(block));
    assert("pre: grid is not NULL" && (grid != NULL) );

    if( hasNodeData )
      {
      if( !CheckNodeFieldsForGrid( grid ) )
        {
        return 1;
        }
      }

    if( hasCellData )
      {
      if( !CheckCellFieldsForGrid( grid ) )
        {
        std::cout << "CheckCellFieldsForGrid failed for block " << block << "\n";
        std::cout.flush();
        return 1;
        }
      }

    } // END for all blocks

  return 0;
}

//------------------------------------------------------------------------------
// Description:
// Adds and XYZ vector field in the nodes of the data-set
void AddNodeCenteredXYZField( vtkMultiBlockDataSet *mbds )
{
  assert("pre: Multi-block is NULL!" && (mbds != NULL) );

  for( unsigned int block=0; block < mbds->GetNumberOfBlocks(); ++block )
    {
    vtkStructuredGrid *grid =
        vtkStructuredGrid::SafeDownCast(mbds->GetBlock(block));
    assert("pre: grid is NULL for the given block" && (grid != NULL) );

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
// Adds and XYZ vector field in the nodes of the dataset
void AddCellCenteredXYZField( vtkMultiBlockDataSet *mbds )
{
  assert("pre: Multi-block is NULL!" && (mbds != NULL) );

  for( unsigned int block=0; block < mbds->GetNumberOfBlocks(); ++block )
    {
    vtkStructuredGrid *grid =
        vtkStructuredGrid::SafeDownCast(mbds->GetBlock(block));
    assert("pre: grid is NULL for the given block" && (grid != NULL) );

    vtkDoubleArray *cellXYZArray = vtkDoubleArray::New();
    cellXYZArray->SetName( "CELL-XYZ" );
    cellXYZArray->SetNumberOfComponents( 3 );
    cellXYZArray->SetNumberOfTuples( grid->GetNumberOfCells() );

    double centroid[3];
    double xyz[3];
    for( vtkIdType cellIdx=0; cellIdx < grid->GetNumberOfCells(); ++cellIdx )
      {
      vtkCell *c = grid->GetCell( cellIdx );
      assert( "pre: cell is not NULL" && (c != NULL) );

      double xsum = 0.0;
      double ysum = 0.0;
      double zsum = 0.0;
      for( vtkIdType node=0; node < c->GetNumberOfPoints(); ++node )
        {
        vtkIdType meshPntIdx = c->GetPointId( node );
        grid->GetPoint( meshPntIdx, xyz );
        xsum += xyz[0];
        ysum += xyz[1];
        zsum += xyz[2];
        } // END for all nodes

      centroid[0] = xsum / c->GetNumberOfPoints();
      centroid[1] = ysum / c->GetNumberOfPoints();
      centroid[2] = zsum / c->GetNumberOfPoints();

      cellXYZArray->SetComponent( cellIdx, 0, centroid[0] );
      cellXYZArray->SetComponent( cellIdx, 1, centroid[1] );
      cellXYZArray->SetComponent( cellIdx, 2, centroid[2] );
      } // END for all cells

    grid->GetCellData()->AddArray( cellXYZArray );
    cellXYZArray->Delete();
    } // END for all blocks
}

//------------------------------------------------------------------------------
// Description:
// Creates a test data-set.
vtkMultiBlockDataSet* GetDataSet(
    double globalOrigin[3], int WholeExtent[6],double gridSpacing[3],
    const int numPartitions, const int numGhosts,
    const bool AddNodeData, const bool AddCellData )
{
  // STEP 0: Get the global grid dimensions
  int dims[3];
  vtkStructuredData::GetDimensionsFromExtent( WholeExtent, dims );

  // STEP 1: Get the whole grid as a uniform grid instance
  vtkUniformGrid *wholeGrid = vtkUniformGrid::New();
  wholeGrid->SetOrigin( globalOrigin );
  wholeGrid->SetSpacing( gridSpacing );
  wholeGrid->SetDimensions( dims );

  // STEP 2: Conver to structured grid
  vtkImageToStructuredGrid *img2sgrid = vtkImageToStructuredGrid::New();
  assert("pre:" && (img2sgrid != NULL));
  img2sgrid->SetInputData( wholeGrid );
  img2sgrid->Update();
  vtkStructuredGrid *wholeStructuredGrid = vtkStructuredGrid::New();
  wholeStructuredGrid->DeepCopy( img2sgrid->GetOutput() );
  img2sgrid->Delete();
  wholeGrid->Delete();

  // STEP 3: Partition the structured grid domain
  vtkStructuredGridPartitioner *gridPartitioner =
      vtkStructuredGridPartitioner::New();
  gridPartitioner->SetInputData( wholeStructuredGrid );
  gridPartitioner->SetNumberOfPartitions( numPartitions );
  gridPartitioner->SetNumberOfGhostLayers( numGhosts );
  gridPartitioner->Update();

  // STEP 4: Get the partitioned dataset
  vtkMultiBlockDataSet *mbds =
      vtkMultiBlockDataSet::SafeDownCast(gridPartitioner->GetOutput() );
  mbds->SetReferenceCount( mbds->GetReferenceCount()+1 );

  // STEP 5: Delete temporary data
  wholeStructuredGrid->Delete();
  gridPartitioner->Delete();

  // STEP 6: Add node-centered and cell-centered fields
  if( AddNodeData )
   {
   AddNodeCenteredXYZField( mbds );
   }

  if( AddCellData )
   {
   AddCellCenteredXYZField( mbds );
   }

  return( mbds );
}

//------------------------------------------------------------------------------
// Description:
// Test 2-D StructuredGridGhostDataGenerator
int Test2D(
    const bool hasNodeData, const bool hasCellData,
    const int numPartitions, const int numGhosts )
{
  std::cout << "===================\n";
  std::cout << "Testing 2-D ghost generation....\n";
  std::cout << "Number of Partitions: " << numPartitions << std::endl;
  std::cout << "Number of ghost-layers in the input: " << numGhosts << "\n";
  std::cout << "Number of ghost-layers to be generated: 1\n";
  std::cout << "Node-centered data: ";
  if( hasNodeData )
    {
    std::cout << "Yes\n";
    }
  else
    {
    std::cout << "No\n";
    }
  std::cout << "Cell-centered data: ";
  if( hasCellData )
    {
    std::cout << "Yes\n";
    }
  else
    {
    std::cout << "No\n";
    }
  std::cout.flush();

  int rc = 0;

  int WholeExtent[6] = {0,49,0,49,0,0};
  double h[3] = {0.5,0.5,0.5};
  double p[3] = {0.0,0.0,0.0};

  vtkMultiBlockDataSet *mbds = GetDataSet(p,WholeExtent,h,
      numPartitions,numGhosts,hasNodeData,hasCellData);
  WriteMultiBlock( mbds, "STRUCTUREDINITIAL");

  vtkStructuredGridGhostDataGenerator *ghostDataGenerator =
      vtkStructuredGridGhostDataGenerator::New();

  ghostDataGenerator->SetInputData( mbds );
  ghostDataGenerator->SetNumberOfGhostLayers( 1 );
  ghostDataGenerator->Update();

  vtkMultiBlockDataSet *ghostDataSet = ghostDataGenerator->GetOutput();
 WriteMultiBlock( ghostDataSet, "STRUCTUREDGHOSTED" );

  rc = CheckFields( ghostDataSet, hasNodeData, hasCellData );
  mbds->Delete();
  ghostDataGenerator->Delete();
  return( rc );
}

//------------------------------------------------------------------------------
// Description:
// Test 2-D StructuredGridGhostDataGenerator
int Test3D(
    const bool hasNodeData, const bool hasCellData,
    const int numPartitions, const int numGhosts )
{
  std::cout << "===================\n";
  std::cout << "Testing 3-D ghost generation....\n";
  std::cout << "Number of Partitions: " << numPartitions << std::endl;
  std::cout << "Number of ghost-layers in the input: " << numGhosts << "\n";
  std::cout << "Number of ghost-layers to be generated: 1\n";
  std::cout << "Node-centered data: ";
  if( hasNodeData )
    {
    std::cout << "Yes\n";
    }
  else
    {
    std::cout << "No\n";
    }
  std::cout << "Cell-centered data: ";
  if( hasCellData )
    {
    std::cout << "Yes\n";
    }
  else
    {
    std::cout << "No\n";
    }
  std::cout.flush();

  int rc = 0;
  int WholeExtent[6] = {0,49,0,49,0,49};
  double h[3] = {0.5,0.5,0.5};
  double p[3] = {0.0,0.0,0.0};

  vtkMultiBlockDataSet *mbds = GetDataSet(p,WholeExtent,h,
      numPartitions,numGhosts,hasNodeData,hasCellData);
  WriteMultiBlock( mbds, "STRUCTUREDINITIAL");

  vtkStructuredGridGhostDataGenerator *ghostDataGenerator =
      vtkStructuredGridGhostDataGenerator::New();
  ghostDataGenerator->SetInputData( mbds );
  ghostDataGenerator->SetNumberOfGhostLayers( 1 );
  ghostDataGenerator->Update();

 vtkMultiBlockDataSet *ghostedDataSet = ghostDataGenerator->GetOutput();
 WriteMultiBlock( ghostedDataSet, "STRUCTUREDGHOSTED" );

  rc = CheckFields( ghostedDataSet, hasNodeData, hasCellData );
  mbds->Delete();
  ghostDataGenerator->Delete();

  return( rc );
}

//------------------------------------------------------------------------------
// Description:
// Tests StructuredGridGhostDataGenerator
int TestStructuredGridGhostDataGenerator(int, char *[])
{
  int rc = 0;
  rc += Test2D(false,false,4,0);
  rc += Test2D(true,false,4,0);
  rc += Test2D(false,true,4,0);
  rc += Test2D(true,true,4,0);

  rc += Test3D(true,false,32,0);
  rc += Test3D(false,true,32,0);
  rc += Test3D(true,true,32,0);
  return( rc );
}

//------------------------------------------------------------------------------
// Description:
// Program main
int main( int argc, char **argv )
{
  if( argc==1 )
    {
    return( TestStructuredGridGhostDataGenerator(argc, argv) );
    }
  int rc             = 0;
  int NumBlocks      = 0;
  int NumGhostLayers = 0;
  if( argc != 3 )
    {
    std::cout << "Usage: ./bin/TestStructuredGridGhostDataGenerator <N> <NG>\n";
    std::cout.flush();
    return 0;
    }

  NumBlocks      = atoi(argv[1]);
  NumGhostLayers = atoi(argv[2]);

  std::cout << "Running 2-D Test with just geometry...";
  std::cout.flush();
  rc += Test2D(false,false,NumBlocks,NumGhostLayers);
  if( rc == 0 )
    {
    std::cout << "[OK]\n";
    std::cout.flush();
    }
  else
    {
    std::cout << "FAILED!!!!\n";
    std::cout.flush();
    }


  std::cout << "Running 2-D Test with node fields...";
  std::cout.flush();
  rc += Test2D(true, false, NumBlocks, NumGhostLayers );
  if( rc == 0 )
    {
    std::cout << "[OK]\n";
    std::cout.flush();
    }
  else
    {
    std::cout << "FAILED!!!!\n";
    std::cout.flush();
    }

  std::cout << "Running 2-D Test with both cell/node fields...";
  std::cout.flush();
  rc += Test2D(true,true,NumBlocks,NumGhostLayers);
  if( rc == 0 )
    {
    std::cout << "[OK]\n";
    std::cout.flush();
    }
  else
    {
    std::cout << "FAILED!!!!\n";
    std::cout.flush();
    }
  return 0;
}

