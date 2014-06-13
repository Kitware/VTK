/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRectilinearGridPartitioner.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestRectilinearGridPartitioner.cxx -- Simple test for partitioning
//  rectilinear grids.
//
// .SECTION Description
//  Simple test for rectilinear grid partitioner

#include <iostream>
#include <sstream>
#include <cassert>

#include "vtkRectilinearGridPartitioner.h"
#include "vtkRectilinearGrid.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkXMLRectilinearGridReader.h"
#include "vtkXMLMultiBlockDataWriter.h"


//------------------------------------------------------------------------------
// Description:
// Writes multi-block dataset to grid
void WriteMultiBlock(std::string file, vtkMultiBlockDataSet *mbds)
{
  assert("pre: NULL multi-block dataset!" && (mbds != NULL) );

  std::ostringstream oss;
  vtkXMLMultiBlockDataWriter *writer = vtkXMLMultiBlockDataWriter::New();

  oss << file << "." << writer->GetDefaultFileExtension();
  writer->SetFileName( oss.str().c_str() );
  writer->SetInputData( mbds );
  writer->Update();
  writer->Delete();
}


//------------------------------------------------------------------------------
// Description:
// Get grid from file
vtkRectilinearGrid* GetGridFromFile( std::string &file )
{
  vtkXMLRectilinearGridReader *reader = vtkXMLRectilinearGridReader::New();
  reader->SetFileName( file.c_str() );
  reader->Update();
  vtkRectilinearGrid* myGrid = vtkRectilinearGrid::New();
  myGrid->DeepCopy( reader->GetOutput() );
  reader->Delete();
  return( myGrid );
}

//------------------------------------------------------------------------------
// Description:
// Program Main
int TestRectilinearGridPartitioner( int argc, char *argv[] )
{
  if( argc != 3 )
    {
    std::cout << "Usage: ./TestRectilinearGridPartitioner <vtsfile> <N>\n";
    std::cout.flush();
    return -1;
    }

  std::string fileName = std::string( argv[1] );
  int NumPartitions    = atoi( argv[2] );

  vtkRectilinearGrid *grid = GetGridFromFile( fileName );
  assert("pre: grid is not NULL" && (grid != NULL) );

  vtkRectilinearGridPartitioner *gridPartitioner =
      vtkRectilinearGridPartitioner::New();
  gridPartitioner->SetInputData(grid);
  gridPartitioner->SetNumberOfPartitions(NumPartitions);
  gridPartitioner->Update();

  std::cout << "Writing the partitioned output...";
  std::cout.flush();
  vtkMultiBlockDataSet *mbds = gridPartitioner->GetOutput();
  WriteMultiBlock( "PartitionedGrid", mbds );
  std::cout << "[DONE]\n";
  std::cout.flush();

  grid->Delete();
  gridPartitioner->Delete();
  return 0;
}

