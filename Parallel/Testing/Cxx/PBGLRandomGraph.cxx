/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PBGLRandomGraph.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* 
 * Copyright (C) 2008 The Trustees of Indiana University.
 * Use, modification and distribution is subject to the Boost Software
 * License, Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)
 */

#include <mpi.h>

#include "vtkInformation.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkPBGLDistributedGraphHelper.h"
#include "vtkSmartPointer.h"

#include <vtksys/stl/utility>
#include <vtksys/stl/vector>

#include <stdlib.h>

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);

  // Create a new graph
  vtkSmartPointer<vtkMutableDirectedGraph> graph 
    = vtkSmartPointer<vtkMutableDirectedGraph>::New();

  // Create a Parallel BGL distributed graph helper
  vtkSmartPointer<vtkPBGLDistributedGraphHelper> helper
    = vtkSmartPointer<vtkPBGLDistributedGraphHelper>::New();

  // Hook the distributed graph helper into the graph. graph will then
  // be a distributed graph.
  graph->SetDistributedGraphHelper(helper);
  int numProcs 
    = graph->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
  int myRank
    = graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  srand((myRank + 1)*117);

  // The simplest test of all: add V vertices to each processor, then
  // each processor adds E randomly-generated edges to the
  // graph. Then, we'll test whether the distributed graph data
  // structure is consistent.
  const vtkIdType V = 10;
  const vtkIdType E = 10;
  vtkstd::vector<vtkstd::pair<vtkIdType, vtkIdType> > added_edges;

  for (vtkIdType v = 0; v < V; ++v)
    {
    graph->AddVertex();
    }

  for (vtkIdType e = 0; e < E; ++e)
    {
    // TODO: For now, we ensure that the source is local
    vtkIdType source = graph->MakeDistributedId(myRank, rand() % V);
    vtkIdType target 
      = graph->MakeDistributedId(rand() % numProcs, rand() % V);
    graph->AddEdge(source, target);

    added_edges.push_back(vtkstd::make_pair(source, target));
    }
  
  // Synchronize the graph, so that everyone finishes adding edges.
  graph->GetDistributedGraphHelper()->Synchronize();

  // TODO: Test the structure of the graph
  
  MPI_Finalize();
  return 0;
}
