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

#include "vtkEdgeListIterator.h"
#include "vtkInEdgeIterator.h"
#include "vtkInformation.h"
#include "vtkIOStream.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkOutEdgeIterator.h"
#include "vtkPBGLDistributedGraphHelper.h"
#include "vtkSmartPointer.h"
#include "vtkVertexListIterator.h"

#include <vtksys/stl/algorithm>
#include <vtksys/stl/functional>
#include <vtksys/stl/vector>

#include <boost/parallel/mpi/datatype.hpp> // for get_mpi_datatype

#include <stdlib.h>

using vtkstd::pair;

#define myassert(Cond)                                  \
  if (!(Cond))                                          \
    {                                                   \
      cerr << "error (" __FILE__ ":" << __LINE__        \
           << ") assertion \"" #Cond "\" failed."       \
           << endl;                                     \
    MPI_Abort(MPI_COMM_WORLD, -1);                      \
    }

// Used to store information about an edge we have added to the graph
struct AddedEdge
{
  AddedEdge() : Source(0), Target(0) { }

  AddedEdge(vtkIdType source, vtkIdType target) 
    : Source(source), Target(target) { }

  vtkIdType Source;
  vtkIdType Target;
};

bool operator<(AddedEdge const& e1, AddedEdge const& e2)
{
  return (unsigned long long)e1.Source < (unsigned long long)e2.Source 
    || ((unsigned long long)e1.Source == (unsigned long long)e2.Source && (unsigned long long)e1.Target < (unsigned long long)e2.Target);
 }

// Order added edges by their target
struct OrderEdgesByTarget : vtkstd::binary_function<AddedEdge, AddedEdge, bool>
{
  bool operator()(AddedEdge const& e1, AddedEdge const& e2)
  {
    return (unsigned long long)e1.Target < (unsigned long long)e2.Target
      || ((unsigned long long)e1.Target == (unsigned long long)e2.Target && (unsigned long long)e1.Source < (unsigned long long)e2.Source);
  }
};

bool operator==(AddedEdge const& e1, AddedEdge const& e2)
{
  return e1.Source == e2.Source && e1.Target == e2.Target;
}

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
  const vtkIdType V = 1000;
  const vtkIdType E = 10000;
  vtkstd::vector<AddedEdge> addedEdges;

  if (myRank == 0)
    {
    (cout << "Build distributed graph with V=" << V*numProcs << ", E = " 
          << E*numProcs << "...").flush();
    }

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

    addedEdges.push_back(AddedEdge(source, target));
    }

  if (myRank == 0)
    {
    (cout << " synchronizing... ").flush();
    }

  // Synchronize the graph, so that everyone finishes adding edges.
  graph->GetDistributedGraphHelper()->Synchronize();

  if (myRank == 0)
    {
    (cout << " done.\n").flush();
    }

  // Test the vertex descriptors
  if (myRank == 0)
    {
    (cout << "Testing vertex descriptors...").flush();
    }
  vtkIdType vExpected = graph->MakeDistributedId(myRank, 0);
  vtkSmartPointer<vtkVertexListIterator> vertices
    = vtkSmartPointer<vtkVertexListIterator>::New();
  graph->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType vActual = vertices->Next();
    myassert(vActual == vExpected);
    ++vExpected;
    }
  myassert(graph->GetVertexIndex(vExpected) == V);
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }

  // Keep our list of the edges we added sorted by source.
  vtkstd::sort(addedEdges.begin(), addedEdges.end());

  // Test the outgoing edges of each local vertex
  if (myRank == 0)
    {
    (cout << "Testing out edges...").flush();
    }
  typedef vtkstd::vector<AddedEdge>::iterator AddedEdgeIterator;
  vtkstd::vector<pair<AddedEdgeIterator, AddedEdgeIterator> > startPositions(V);
  graph->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType u = vertices->Next();
    vtkSmartPointer<vtkOutEdgeIterator> outEdges
      = vtkSmartPointer<vtkOutEdgeIterator>::New();

    // Find bounds within the addedEdges array where the edges for
    // this node will occur.
    vtkstd::vector<AddedEdge>::iterator myEdgesStart, myEdgesEnd;
    myEdgesStart = vtkstd::lower_bound(addedEdges.begin(), addedEdges.end(),
                                       AddedEdge
                                         (u,
                                          graph->MakeDistributedId(0, 0)));
    myEdgesEnd = vtkstd::lower_bound(myEdgesStart, addedEdges.end(),
                                     AddedEdge
                                       (u+1, 
                                        graph->MakeDistributedId(0, 0)));
    startPositions[graph->GetVertexIndex(u)].first = myEdgesStart;
    startPositions[graph->GetVertexIndex(u)].second = myEdgesEnd;

    graph->GetOutEdges(u, outEdges);
    while (outEdges->HasNext()) 
      {
      vtkOutEdgeType e = outEdges->Next();

      // Make sure we're expecting to find more out-edges
      myassert(myEdgesStart != myEdgesEnd);

      // Make sure that we added an edge with the same source/target
      vtkstd::vector<AddedEdge>::iterator found 
        = vtkstd::find(myEdgesStart, myEdgesEnd,
                       AddedEdge(u, e.Target));
      myassert(found != myEdgesEnd);

      // Move this edge out of the way, so we don't find it again
      --myEdgesEnd;
      vtkstd::swap(*found, *myEdgesEnd);
      }

    // Make sure that the constructed graph isn't missing any edges
    assert(myEdgesStart == myEdgesEnd);
    }
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }

  // Test all of the local edges
  if (myRank == 0)
    {
    (cout << "Testing all edges...").flush();
    }
  vtkSmartPointer<vtkEdgeListIterator> edges
    = vtkSmartPointer<vtkEdgeListIterator>::New();
  graph->GetEdges(edges);
  while (edges->HasNext())
    {
      vtkEdgeType e = edges->Next();
      pair<AddedEdgeIterator, AddedEdgeIterator>& bracket 
        = startPositions[graph->GetVertexIndex(e.Source)];
      
      // Make sure we're expecting to find more edges in this source's
      // bracket
      myassert(bracket.first != bracket.second);

      // Make sure that we added an edge with the same source/target
      vtkstd::vector<AddedEdge>::iterator found 
        = vtkstd::find(bracket.first, bracket.second,
                       AddedEdge(e.Source, e.Target));
      myassert(found != bracket.second);

      // Move this edge out of the way, so we don't find it again
      --bracket.second;
      vtkstd::swap(*found, *bracket.second);
    }
  // Ensure that all of the edges we added actually got added
  for (vtkIdType v = 0; v < V; ++v)
    {
    myassert(startPositions[v].first == startPositions[v].second);
    }
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }

  // Let everyone know about the in-edges they should have.

  // Sort the edges by target, so we can send out blocks of edges
  // to their owners.
  vtkstd::sort(addedEdges.begin(), addedEdges.end(), OrderEdgesByTarget());
  
  // Determine the number of incoming edges to send to each processor.
  vtkstd::vector<int> sendCounts(numProcs, 0);
  for (vtkstd::vector<AddedEdge>::size_type i = 0; i < addedEdges.size(); ++i)
    {
    ++sendCounts[graph->GetVertexOwner(addedEdges[i].Target)];
    }
  vtkstd::vector<int> offsetsSend(numProcs, 0);
  int count = 0;
  for (int p = 0; p < numProcs; ++p)
    {
    offsetsSend[p] = count;
    count += sendCounts[p];
    }

  // Swap counts with the other processors
  vtkstd::vector<int> recvCounts(numProcs);
  MPI_Alltoall(&sendCounts.front(), 1, MPI_INT, 
               &recvCounts.front(), 1, MPI_INT,
               MPI_COMM_WORLD);

  // Determine the offsets into our own incoming edges buffer
  vtkstd::vector<int> offsetsRecv(numProcs);
  count = 0;
  for (int p = 0; p < numProcs; ++p)
    {
    offsetsRecv[p] = count;
    count += recvCounts[p];
    }

  // Build a datatype so that we can transmit AddedEdge structures.
  // TODO: If we're being really, really, really picky, this isn't 100% 
  // correct, because AddedEdge is not an aggregate and therefore might
  // have a non-trivial layout. But no C++ compiler takes advantage
  // of this latitude, and this won't be the first program to break 
  // because of it, so forget that you even read this overlong comment.
  MPI_Datatype addedEdgeDatatype;
  MPI_Type_contiguous(2, boost::parallel::mpi::get_mpi_datatype<vtkIdType>(),
                      &addedEdgeDatatype);
  MPI_Type_commit(&addedEdgeDatatype);
  
  // Swap incoming edges with the other processors.
  vtkstd::vector<AddedEdge> inEdges(count);
  MPI_Alltoallv(&addedEdges[0], &sendCounts[0], &offsetsSend[0], 
                addedEdgeDatatype,
                &inEdges[0], &recvCounts[0], &offsetsRecv[0],
                addedEdgeDatatype,
                MPI_COMM_WORLD);

  // Free the AddedEdges datatype
  MPI_Type_free(&addedEdgeDatatype);

  // Test the incoming edges of each local vertex
  if (myRank == 0)
    {
    (cout << "Testing in edges...").flush();
    }
  vtkstd::sort(inEdges.begin(), inEdges.end(), OrderEdgesByTarget());
  graph->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType u = vertices->Next();

    // Find bounds within the inEdges array where the incoming edges
    // for this node will occur.
    vtkstd::vector<AddedEdge>::iterator myEdgesStart, myEdgesEnd;
    myEdgesStart = vtkstd::lower_bound(inEdges.begin(), inEdges.end(),
                                       AddedEdge
                                         (graph->MakeDistributedId(0, 0), 
                                          u),
                                       OrderEdgesByTarget());
    myEdgesEnd = vtkstd::lower_bound(myEdgesStart, inEdges.end(),
                                     AddedEdge
                                       (graph->MakeDistributedId(0, 0),
                                        u+1),
                                     OrderEdgesByTarget());

    vtkSmartPointer<vtkInEdgeIterator> inEdges
      = vtkSmartPointer<vtkInEdgeIterator>::New();
    graph->GetInEdges(u, inEdges);
    while (inEdges->HasNext()) 
      {
      vtkInEdgeType e = inEdges->Next();

      // Make sure we're expecting to find more in-edges
      myassert(myEdgesStart != myEdgesEnd);

      // Make sure that we added an edge with the same source/target
      vtkstd::vector<AddedEdge>::iterator found 
        = vtkstd::find(myEdgesStart, myEdgesEnd,
                       AddedEdge(e.Source, u));
      myassert(found != myEdgesEnd);

      // Move this edge out of the way, so we don't find it again
      --myEdgesEnd;
      vtkstd::swap(*found, *myEdgesEnd);
      }

    // Make sure that the constructed graph isn't missing any edges
    assert(myEdgesStart == myEdgesEnd);
    }  
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }

  MPI_Finalize();
  return 0;
}
