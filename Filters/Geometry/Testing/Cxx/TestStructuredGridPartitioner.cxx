/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStructuredGridPartitioner.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestStructuredGridPartitioner.cxx -- Simple test for partitioning
//  structured grids.
//
// .SECTION Description
//  Simple test for structured grid partitioner

#include <cassert>
#include <iostream>
#include <sstream>

#include "vtkMultiBlockDataSet.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridPartitioner.h"
#include "vtkXMLMultiBlockDataWriter.h"
#include "vtkXMLStructuredGridReader.h"

//------------------------------------------------------------------------------
// Description:
// Writes multi-block dataset to grid
void WriteMultiBlock(const std::string& file, vtkMultiBlockDataSet* mbds)
{
  assert("pre: nullptr multi-block dataset!" && (mbds != nullptr));

  std::ostringstream oss;
  vtkXMLMultiBlockDataWriter* writer = vtkXMLMultiBlockDataWriter::New();

  oss << file << "." << writer->GetDefaultFileExtension();
  writer->SetFileName(oss.str().c_str());
  writer->SetInputData(mbds);
  writer->Update();
  writer->Delete();
}

//------------------------------------------------------------------------------
// Description:
// Get grid from file
vtkStructuredGrid* GetGridFromFile(std::string& file)
{
  vtkXMLStructuredGridReader* reader = vtkXMLStructuredGridReader::New();
  reader->SetFileName(file.c_str());
  reader->Update();
  vtkStructuredGrid* myGrid = vtkStructuredGrid::New();
  myGrid->DeepCopy(reader->GetOutput());
  reader->Delete();
  return (myGrid);
}

//------------------------------------------------------------------------------
// Description:
// Program Main
int TestStructuredGridPartitioner(int argc, char* argv[])
{

  if (argc != 3)
  {
    std::cout << "Usage: ./TestStructuredGridPartitioner <vtsfile> <N>\n";
    std::cout.flush();
    return -1;
  }

  std::string fileName = std::string(argv[1]);
  int NumPartitions = atoi(argv[2]);

  vtkStructuredGrid* grid = GetGridFromFile(fileName);
  assert("pre: grid is not nullptr" && (grid != nullptr));

  vtkStructuredGridPartitioner* gridPartitioner = vtkStructuredGridPartitioner::New();
  gridPartitioner->SetInputData(grid);
  gridPartitioner->SetNumberOfPartitions(NumPartitions);
  gridPartitioner->Update();

  vtkMultiBlockDataSet* mbds = gridPartitioner->GetOutput();
  WriteMultiBlock("PartitionedGrid", mbds);

  grid->Delete();
  gridPartitioner->Delete();
  return 0;
}
