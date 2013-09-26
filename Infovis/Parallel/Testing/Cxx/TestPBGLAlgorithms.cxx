/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPBGLAlgorithms.cxx

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

#include "vtkDataSetAttributes.h"
#include "vtkEdgeListIterator.h"
#include "vtkInEdgeIterator.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkIOStream.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkOutEdgeIterator.h"
#include "vtkPBGLBreadthFirstSearch.h"
#include "vtkPBGLDistributedGraphHelper.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtksys/stl/algorithm>
#include <vtksys/stl/functional>
#include <vtksys/stl/vector>

#include <stdlib.h>
#include <cassert>

using std::pair;
using std::vector;

static vtkIdType verticesPerNode = 1000;

void TestDirectedGraph()
{
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

  if (numProcs == 1)
    {
    cout << "Distributed-graph test run with one node; nothing to do.\n";
    return;
    }

  // Build a graph with verticesPerNode vertices on each node
  if (myRank == 0)
    {
    (cout << "Building distributed directed graph...").flush();
    }
  for (vtkIdType i = 0; i < verticesPerNode; ++i)
    {
    graph->AddVertex();
    }

  // Add an extra vertex on the first and last processors. These
  // vertices will be the source and the sink of search algorithms,
  // respectively.
  if (myRank == 0 || myRank == numProcs - 1)
    {
    graph->AddVertex();
    }

  // Add edges from the ith vertex on each node to the ith vertex on
  // the next node (not counting the extra vertices, of course).
  if (myRank < numProcs - 1)
    {
    for (vtkIdType i = 0; i < verticesPerNode; ++i)
      {
      graph->AddEdge(helper->MakeDistributedId(myRank, i),
                     helper->MakeDistributedId(myRank+1, i));
      }
    }

  // Add edges from the source vertex to each of the vertices on the
  // first node
  if (myRank == 0)
    {
    for (vtkIdType i = 0; i < verticesPerNode; ++i)
      {
      graph->AddEdge(helper->MakeDistributedId(myRank, verticesPerNode),
                     helper->MakeDistributedId(myRank, i));
      }
    }

  // Add edges from each of the vertices on the last node to the sink
  // vertex.
  if (myRank == numProcs - 1)
    {
    for (vtkIdType i = 0; i < verticesPerNode; ++i)
      {
      graph->AddEdge(helper->MakeDistributedId(myRank, i),
                     helper->MakeDistributedId(myRank, verticesPerNode));
      }
    }

  // Synchronize so that everyone catches up
  helper->Synchronize();
  if (myRank == 0)
    {
    (cout << " done.\n").flush();
    }

  // Build the breadth-first search filter
  vtkSmartPointer<vtkPBGLBreadthFirstSearch> bfs
    = vtkSmartPointer<vtkPBGLBreadthFirstSearch>::New();
  bfs->SetInputData(graph);
  bfs->SetOriginVertex(helper->MakeDistributedId(0, verticesPerNode));

  // Run the breadth-first search
  if (myRank == 0)
    {
    (cout << "  Breadth-first search...").flush();
    }
  bfs->UpdateInformation();
  vtkStreamingDemandDrivenPipeline* exec =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(bfs->GetExecutive());
  exec->SetUpdateNumberOfPieces(exec->GetOutputInformation(0), numProcs);
  exec->SetUpdatePiece(exec->GetOutputInformation(0), myRank);
  bfs->Update();

  // Verify the results of the breadth-first search
  if (myRank == 0)
    {
    (cout << " verifying...").flush();
    }

  graph = vtkMutableDirectedGraph::SafeDownCast(bfs->GetOutput());
  int index;
  vtkIntArray *distArray
    = vtkIntArray::SafeDownCast(graph->GetVertexData()->GetArray("BFS", index));
  assert(distArray);
  for (vtkIdType i = 0; i < verticesPerNode; ++i)
    {
    if (distArray->GetValue(i) != myRank + 1)
      {
        cerr << "Value at vertex " << i << " on rank " << myRank
             << " is " << distArray->GetValue(i) << ", expected "
             << (myRank + 1) << endl;
      }
    assert(distArray->GetValue(i) == myRank + 1);
    }
  if (myRank == 0)
    assert(distArray->GetValue(verticesPerNode) == 0);
  if (myRank == numProcs - 1)
    assert(distArray->GetValue(verticesPerNode) == numProcs + 1);
  helper->Synchronize();
  if (myRank == 0)
    {
    (cout << " done.\n").flush();
    }
}

void TestUndirectedGraph()
{
  // Create a new graph
  vtkSmartPointer<vtkMutableUndirectedGraph> graph
    = vtkSmartPointer<vtkMutableUndirectedGraph>::New();

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

  if (numProcs == 1)
    {
    cout << "Distributed-graph test run with one node; nothing to do.\n";
    return;
    }

  // Build a graph with verticesPerNode vertices on each node
  if (myRank == 0)
    {
    (cout << "Building distributed undirected graph...").flush();
    }
  for (vtkIdType i = 0; i < verticesPerNode; ++i)
    {
    graph->AddVertex();
    }

  // Add an extra vertex on the first and last processors. These
  // vertices will be the source and the sink of search algorithms,
  // respectively.
  if (myRank == 0 || myRank == numProcs - 1)
    {
    graph->AddVertex();
    }

  // Add edges from the ith vertex on each node to the ith vertex on
  // the next node (not counting the extra vertices, of course).
  if (myRank < numProcs - 1)
    {
    for (vtkIdType i = 0; i < verticesPerNode; ++i)
      {
      graph->AddEdge(helper->MakeDistributedId(myRank, i),
                     helper->MakeDistributedId(myRank+1, i));
      }
    }

  // Add edges from the source vertex to each of the vertices on the
  // first node
  if (myRank == 0)
    {
    for (vtkIdType i = 0; i < verticesPerNode; ++i)
      {
      graph->AddEdge(helper->MakeDistributedId(myRank, verticesPerNode),
                     helper->MakeDistributedId(myRank, i));
      }
    }

  // Add edges from each of the vertices on the last node to the sink
  // vertex.
  if (myRank == numProcs - 1)
    {
    for (vtkIdType i = 0; i < verticesPerNode; ++i)
      {
      graph->AddEdge(helper->MakeDistributedId(myRank, i),
                     helper->MakeDistributedId(myRank, verticesPerNode));
      }
    }

  // Synchronize so that everyone catches up
  helper->Synchronize();
  if (myRank == 0)
    {
    (cout << " done.\n").flush();
    }

  // Build the breadth-first search filter
  vtkSmartPointer<vtkPBGLBreadthFirstSearch> bfs
    = vtkSmartPointer<vtkPBGLBreadthFirstSearch>::New();
  bfs->SetInputData(graph);
  bfs->SetOriginVertex(helper->MakeDistributedId(0, verticesPerNode));

  // Run the breadth-first search
  if (myRank == 0)
    {
    (cout << "  Breadth-first search...").flush();
    }
  bfs->UpdateInformation();
  vtkStreamingDemandDrivenPipeline* exec =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(bfs->GetExecutive());
  exec->SetUpdateNumberOfPieces(exec->GetOutputInformation(0), numProcs);
  exec->SetUpdatePiece(exec->GetOutputInformation(0), myRank);
  bfs->Update();

  // Verify the results of the breadth-first search
  if (myRank == 0)
    {
    (cout << " verifying...").flush();
    }

  graph = vtkMutableUndirectedGraph::SafeDownCast(bfs->GetOutput());
  int index;
  vtkIntArray *distArray
    = vtkIntArray::SafeDownCast(graph->GetVertexData()->GetArray("BFS", index));
  assert(distArray);
  for (vtkIdType i = 0; i < verticesPerNode; ++i)
    {
    if (distArray->GetValue(i) != myRank + 1)
      {
        cerr << "Value at vertex " << i << " on rank " << myRank
             << " is " << distArray->GetValue(i) << ", expected "
             << (myRank + 1) << endl;
      }
    assert(distArray->GetValue(i) == myRank + 1);
    }
  if (myRank == 0)
    {
    assert(distArray->GetValue(verticesPerNode) == 0);
    }
  if (myRank == numProcs - 1)
    {
    assert(distArray->GetValue(verticesPerNode) == numProcs + 1);
    }
  helper->Synchronize();
  if (myRank == 0)
    {
    (cout << " done.\n").flush();
    }
}

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  TestDirectedGraph();
  TestUndirectedGraph();
  MPI_Finalize();
  return 0;
}
