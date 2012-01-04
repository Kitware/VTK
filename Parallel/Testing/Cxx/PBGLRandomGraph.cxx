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
#include "vtkMutableUndirectedGraph.h"
#include "vtkOutEdgeIterator.h"
#include "vtkPBGLDistributedGraphHelper.h"
#include "vtkSmartPointer.h"
#include "vtkVertexListIterator.h"
#include "vtkIntArray.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkVariantArray.h"
#include "vtkDataSetAttributes.h"

#include <vtksys/stl/algorithm>
#include <vtksys/stl/functional>
#include <vtksys/stl/vector>

#include <boost/mpi/datatype.hpp> // for get_mpi_datatype

#include <stdlib.h>

using std::pair;
using std::vector;

// Percentage of the time that the AddEdge operation in this test will
// perform an "immediate" edge-addition operation, requiring the
// processor initiating the AddEdge call to wait until the owner of
// the edge has actually added the edge.
static const int ImmediateAddEdgeChance = 3;


#define myassert(Cond)                                  \
  if (!(Cond))                                          \
    {                                                   \
      cerr << "error (" __FILE__ ":" << dec << __LINE__ \
           << ") assertion \"" #Cond "\" failed."       \
           << endl;                                     \
    MPI_Abort(MPI_COMM_WORLD, -1);                      \
    }

//----------------------------------------------------------------------------
// Used to store information about an edge we have added to the graph
struct AddedEdge
{
  AddedEdge() : Source(0), Target(0) { }

  AddedEdge(vtkIdType source, vtkIdType target) 
    : Source(source), Target(target) { }

  template<typename Archiver>
  void serialize(Archiver& ar, const unsigned int) 
  {
    ar & Source & Target;
  }

  vtkIdType Source;
  vtkIdType Target;
};

namespace boost { namespace mpi {
    template<>
    struct is_mpi_datatype<AddedEdge> : is_mpi_datatype<vtkIdType> { };
} } // end namespace boost::mpi

// Order added edges by their source
struct OrderEdgesBySource : std::binary_function<AddedEdge, AddedEdge, bool>
{
  bool operator()(AddedEdge const& e1, AddedEdge const& e2)
  {
    return (unsigned long long)e1.Source < (unsigned long long)e2.Source
      || ((unsigned long long)e1.Source == (unsigned long long)e2.Source && (unsigned long long)e1.Target < (unsigned long long)e2.Target);
  }
};

// Order added edges by their target
struct OrderEdgesByTarget : std::binary_function<AddedEdge, AddedEdge, bool>
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

// Predicate that tests whether this is a self-loop
struct IsSelfLoop : std::unary_function<AddedEdge, bool>
{
  bool operator()(AddedEdge const& e)
  {
    return e.Source == e.Target;
  }
};
//----------------------------------------------------------------------------
// Given the locally-generated edges in outEdges (which must be sorted
// by the source or target of the edge, depending on whether source is
// true), determine the incoming edges and put them into inEdges.
void ExchangeEdges(vtkGraph* graph,
                   vector<AddedEdge> const& outEdges, 
                   vector<AddedEdge>& inEdges,
                   bool source)
{
  int numProcs 
    = graph->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
  int myRank
    = graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  vtkDistributedGraphHelper *helper = graph->GetDistributedGraphHelper();

  // Determine the number of incoming edges to send to each processor.
  vector<int> sendCounts(numProcs, 0);
  for (vector<AddedEdge>::size_type i = 0; i < outEdges.size(); ++i)
    {
    ++sendCounts[source? helper->GetVertexOwner(outEdges[i].Source)
                       : helper->GetVertexOwner(outEdges[i].Target)];
    }
  vector<int> offsetsSend(numProcs, 0);
  int count = 0;
  for (int p = 0; p < numProcs; ++p)
    {
    offsetsSend[p] = count;
    count += sendCounts[p];
    }

  // Swap counts with the other processors
  vector<int> recvCounts(numProcs);
  MPI_Alltoall(&sendCounts.front(), 1, MPI_INT, 
               &recvCounts.front(), 1, MPI_INT,
               MPI_COMM_WORLD);

  // Determine the offsets into our own incoming edges buffer
  vector<int> offsetsRecv(numProcs);
  count = 0;
  for (int p = 0; p < numProcs; ++p)
    {
    offsetsRecv[p] = count;
    count += recvCounts[p];
    }

  // Swap incoming edges with the other processors.
  inEdges.resize(count);
  MPI_Alltoallv(const_cast<AddedEdge*>(&outEdges[0]), &sendCounts[0], 
                &offsetsSend[0], boost::mpi::get_mpi_datatype<AddedEdge>(),
                &inEdges[0], &recvCounts[0], &offsetsRecv[0],
                boost::mpi::get_mpi_datatype<AddedEdge>(),
                MPI_COMM_WORLD);
}

//----------------------------------------------------------------------------
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
  srand((myRank + 1)*117);

  // The simplest test of all: add V vertices to each processor, then
  // each processor adds E randomly-generated edges to the
  // graph. Then, we'll test whether the distributed graph data
  // structure is consistent.
  const vtkIdType V = 10000;
  const vtkIdType E = 100000;
  vector<AddedEdge> generatedEdges;

  if (myRank == 0)
    {
    (cout << "Build distributed directed graph (V=" << V*numProcs 
          << ", E=" << E*numProcs << ")...").flush();
    }

  for (vtkIdType v = 0; v < V; ++v)
    {
    graph->AddVertex();
    }

  for (vtkIdType e = 0; e < E; ++e)
    {
    vtkIdType source = helper->MakeDistributedId(rand() % numProcs, rand() % V);
    vtkIdType target = helper->MakeDistributedId(rand() % numProcs, rand() % V);

    if (rand() % 100 < ImmediateAddEdgeChance)
      graph->AddEdge(source, target);
    else
      graph->LazyAddEdge(source, target);

    generatedEdges.push_back(AddedEdge(source, target));
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

  // We know which edges we generated, but some of those edges were
  // actually added on other nodes. Do a large exchange so that
  // addedEdges contains all of the edges that should originate on
  // this node.
  std::sort(generatedEdges.begin(), generatedEdges.end(), OrderEdgesBySource());
  std::vector<AddedEdge> addedEdges;
  ExchangeEdges(graph, generatedEdges, addedEdges, true);
  std::vector<AddedEdge>().swap(generatedEdges);

  // Test the vertex descriptors
  if (myRank == 0)
    {
    (cout << "  Testing vertex descriptors...").flush();
    }
  vtkIdType vExpected = helper->MakeDistributedId(myRank, 0);
  vtkSmartPointer<vtkVertexListIterator> vertices
    = vtkSmartPointer<vtkVertexListIterator>::New();
  graph->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType vActual = vertices->Next();
    myassert(vActual == vExpected);
    ++vExpected;
    }
  myassert(helper->GetVertexIndex(vExpected) == V);
  MPI_Barrier(MPI_COMM_WORLD);
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }

  // Keep our list of the edges we added sorted by source.
  std::sort(addedEdges.begin(), addedEdges.end(), OrderEdgesBySource());

  // Test the outgoing edges of each local vertex
  if (myRank == 0)
    {
    (cout << "  Testing out edges...").flush();
    }
  typedef vector<AddedEdge>::iterator AddedEdgeIterator;
  vector<pair<AddedEdgeIterator, AddedEdgeIterator> > startPositions(V);
  graph->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType u = vertices->Next();
    vtkSmartPointer<vtkOutEdgeIterator> outEdges
      = vtkSmartPointer<vtkOutEdgeIterator>::New();

    // Find bounds within the addedEdges array where the edges for
    // this node will occur.
    vector<AddedEdge>::iterator myEdgesStart, myEdgesEnd;
    myEdgesStart = std::lower_bound(addedEdges.begin(), addedEdges.end(),
                                       AddedEdge
                                         (u,
                                          helper->MakeDistributedId(0, 0)),
                                       OrderEdgesBySource());
    myEdgesEnd = std::lower_bound(myEdgesStart, addedEdges.end(),
                                     AddedEdge
                                       (u+1, 
                                        helper->MakeDistributedId(0, 0)),
                                     OrderEdgesBySource());
    startPositions[helper->GetVertexIndex(u)].first = myEdgesStart;
    startPositions[helper->GetVertexIndex(u)].second = myEdgesEnd;

    graph->GetOutEdges(u, outEdges);
    while (outEdges->HasNext()) 
      {
      vtkOutEdgeType e = outEdges->Next();

      // Make sure we're expecting to find more out-edges
      myassert(myEdgesStart != myEdgesEnd);

      // Make sure that we added an edge with the same source/target
      vector<AddedEdge>::iterator found 
        = std::find(myEdgesStart, myEdgesEnd,
                       AddedEdge(u, e.Target));
      myassert(found != myEdgesEnd);

      // Move this edge out of the way, so we don't find it again
      --myEdgesEnd;
      std::swap(*found, *myEdgesEnd);

      // Check the source and target of the edge.
      myassert(u == graph->GetSourceVertex(e.Id));
      myassert(e.Target == graph->GetTargetVertex(e.Id));
      }

    // Make sure that the constructed graph isn't missing any edges
    assert(myEdgesStart == myEdgesEnd);
    }
  helper->Synchronize();
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }

  // Test all of the local edges
  if (myRank == 0)
    {
    (cout << "  Testing all edges...").flush();
    }
  vtkSmartPointer<vtkEdgeListIterator> edges
    = vtkSmartPointer<vtkEdgeListIterator>::New();
  graph->GetEdges(edges);
  while (edges->HasNext())
    {
      vtkEdgeType e = edges->Next();
      pair<AddedEdgeIterator, AddedEdgeIterator>& bracket 
        = startPositions[helper->GetVertexIndex(e.Source)];
      
      // Make sure we're expecting to find more edges in this source's
      // bracket
      myassert(bracket.first != bracket.second);

      // Make sure that we added an edge with the same source/target
      vector<AddedEdge>::iterator found 
        = std::find(bracket.first, bracket.second,
                       AddedEdge(e.Source, e.Target));
      myassert(found != bracket.second);

      // Move this edge out of the way, so we don't find it again
      --bracket.second;
      std::swap(*found, *bracket.second);
    }
  // Ensure that all of the edges we added actually got added
  for (vtkIdType v = 0; v < V; ++v)
    {
    myassert(startPositions[v].first == startPositions[v].second);
    }
  MPI_Barrier(MPI_COMM_WORLD);
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }

  // Let everyone know about the in-edges they should have.
  std::sort(addedEdges.begin(), addedEdges.end(), OrderEdgesByTarget());
  vector<AddedEdge> inEdges;
  ExchangeEdges(graph, addedEdges, inEdges, false);

  // Test the incoming edges of each local vertex
  if (myRank == 0)
    {
    (cout << "  Testing in edges...").flush();
    }
  std::sort(inEdges.begin(), inEdges.end(), OrderEdgesByTarget());
  graph->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType u = vertices->Next();

    // Find bounds within the inEdges array where the incoming edges
    // for this node will occur.
    vector<AddedEdge>::iterator myEdgesStart, myEdgesEnd;
    myEdgesStart = std::lower_bound(inEdges.begin(), inEdges.end(),
                                       AddedEdge
                                         (helper->MakeDistributedId(0, 0), 
                                          u),
                                       OrderEdgesByTarget());
    myEdgesEnd = std::lower_bound(myEdgesStart, inEdges.end(),
                                     AddedEdge
                                       (helper->MakeDistributedId(0, 0),
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
      vector<AddedEdge>::iterator found 
        = std::find(myEdgesStart, myEdgesEnd,
                       AddedEdge(e.Source, u));
      myassert(found != myEdgesEnd);

      // Move this edge out of the way, so we don't find it again
      --myEdgesEnd;
      std::swap(*found, *myEdgesEnd);

      // Check the source and target of the edge.
      myassert(e.Source == graph->GetSourceVertex(e.Id));
      myassert(u == graph->GetTargetVertex(e.Id));
      }

    // Make sure that the constructed graph isn't missing any edges
    assert(myEdgesStart == myEdgesEnd);
    }  
  helper->Synchronize();
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }
}

//----------------------------------------------------------------------------
void TestDirectedGraphProperties()
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
  srand((myRank + 1)*117);

  // The simplest test of all: add V vertices to each processor, then
  // each processor adds E randomly-generated edges to the
  // graph. Then, we'll test whether the distributed graph data
  // structure is consistent.
//  const vtkIdType V = 5000;
//  const vtkIdType E = 50000;
  const vtkIdType V = 5;
  const vtkIdType E = 10;
  vector<AddedEdge> generatedEdges;

  if (myRank == 0)
    {
    (cout << "Build distributed directed graph w/ properties (V=" << V*numProcs 
          << ", E=" << E*numProcs << ")...").flush();
    }
  
  //  Create some vertex property arrays
  vtkVariantArray* vertexPropertyArr = vtkVariantArray::New();
  int numVertexProperties = 2;
  vertexPropertyArr->SetNumberOfValues(numVertexProperties);
  
  vtkStringArray* vertexProp0Array = vtkStringArray::New();
  vertexProp0Array->SetName("labels");
  graph->GetVertexData()->AddArray(vertexProp0Array);
  
  vtkIntArray* vertexProp1Array = vtkIntArray::New();
  vertexProp1Array->SetName("veight");
  graph->GetVertexData()->AddArray(vertexProp1Array);
  
  const char *vertexLabel[] = {"Dick","Jane","Sally","Spot","Puff"};
  
  
  //  Create some edge property arrays
  vtkVariantArray* edgePropertyArr = vtkVariantArray::New();
  int numEdgeProperties = 2;
  edgePropertyArr->SetNumberOfValues(numEdgeProperties);
  
  vtkIntArray* edgeProp0Array = vtkIntArray::New();
  edgeProp0Array->SetName("happiness");
  graph->GetEdgeData()->AddArray(edgeProp0Array);
  
  vtkDoubleArray* edgeProp1Array = vtkDoubleArray::New();
  edgeProp1Array->SetName("weight");
  graph->GetEdgeData()->AddArray(edgeProp1Array);
  

  const char *stringProp = vertexLabel[0];
  int veight;
  for (vtkIdType v = 0; v < V; ++v)
    {
    stringProp = vertexLabel[rand() % 5];
    veight = rand() % 10;
//    cout << myRank <<" vertex "<< v <<","<< stringProp <<","<<veight<< endl;
    vertexPropertyArr->SetValue(0,stringProp);
    vertexPropertyArr->SetValue(1,veight);
    graph->AddVertex(vertexPropertyArr);
    }

  int prop0 = 13;
  double prop1 = 42.42;
  for (vtkIdType e = 0; e < E; ++e)
    {
    vtkIdType source = helper->MakeDistributedId(rand() % numProcs, rand() % V);
    vtkIdType target = helper->MakeDistributedId(rand() % numProcs, rand() % V);

    edgePropertyArr->SetValue(0,prop0);
    edgePropertyArr->SetValue(1,prop1);
    graph->LazyAddEdge(source, target, edgePropertyArr);
    prop0 += 1;
    prop1 += 1.0;
//    cout << myRank <<","<< prop0<<","<<prop1 << endl;

    generatedEdges.push_back(AddedEdge(source, target));
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

  
  MPI_Barrier(MPI_COMM_WORLD);
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }
  


  // We know which edges we generated, but some of those edges were
  // actually added on other nodes. Do a large exchange so that
  // addedEdges contains all of the edges that should originate on
  // this node.
  std::sort(generatedEdges.begin(), generatedEdges.end(), OrderEdgesBySource());
  std::vector<AddedEdge> addedEdges;
  ExchangeEdges(graph, generatedEdges, addedEdges, true);
  std::vector<AddedEdge>().swap(generatedEdges);

  // Test the vertex descriptors
  if (myRank == 0)
    {
    (cout << "  Testing vertex descriptors...").flush();
    }
  vtkIdType vExpected = helper->MakeDistributedId(myRank, 0);
  vtkSmartPointer<vtkVertexListIterator> vertices
    = vtkSmartPointer<vtkVertexListIterator>::New();
  graph->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType vActual = vertices->Next();
    myassert(vActual == vExpected);
    ++vExpected;
    }
  myassert(helper->GetVertexIndex(vExpected) == V);
  MPI_Barrier(MPI_COMM_WORLD);
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }

  // Keep our list of the edges we added sorted by source.
  std::sort(addedEdges.begin(), addedEdges.end(), OrderEdgesBySource());

  // Test the outgoing edges of each local vertex
  if (myRank == 0)
    {
    (cout << "  Testing out edges...").flush();
    }
  typedef vector<AddedEdge>::iterator AddedEdgeIterator;
  vector<pair<AddedEdgeIterator, AddedEdgeIterator> > startPositions(V);
  graph->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType u = vertices->Next();
    vtkSmartPointer<vtkOutEdgeIterator> outEdges
      = vtkSmartPointer<vtkOutEdgeIterator>::New();

    // Find bounds within the addedEdges array where the edges for
    // this node will occur.
    vector<AddedEdge>::iterator myEdgesStart, myEdgesEnd;
    myEdgesStart = std::lower_bound(addedEdges.begin(), addedEdges.end(),
                                       AddedEdge
                                         (u,
                                          helper->MakeDistributedId(0, 0)),
                                       OrderEdgesBySource());
    myEdgesEnd = std::lower_bound(myEdgesStart, addedEdges.end(),
                                     AddedEdge
                                       (u+1, 
                                        helper->MakeDistributedId(0, 0)),
                                     OrderEdgesBySource());
    startPositions[helper->GetVertexIndex(u)].first = myEdgesStart;
    startPositions[helper->GetVertexIndex(u)].second = myEdgesEnd;

    graph->GetOutEdges(u, outEdges);
    while (outEdges->HasNext()) 
      {
      vtkOutEdgeType e = outEdges->Next();

      // Make sure we're expecting to find more out-edges
      myassert(myEdgesStart != myEdgesEnd);

      // Make sure that we added an edge with the same source/target
      vector<AddedEdge>::iterator found 
        = std::find(myEdgesStart, myEdgesEnd,
                       AddedEdge(u, e.Target));
      myassert(found != myEdgesEnd);

      // Move this edge out of the way, so we don't find it again
      --myEdgesEnd;
      std::swap(*found, *myEdgesEnd);
      }

    // Make sure that the constructed graph isn't missing any edges
    assert(myEdgesStart == myEdgesEnd);
    }
  MPI_Barrier(MPI_COMM_WORLD);
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }

  // Test all of the local edges
  if (myRank == 0)
    {
    (cout << "  Testing all edges...").flush();
    }
  vtkSmartPointer<vtkEdgeListIterator> edges
    = vtkSmartPointer<vtkEdgeListIterator>::New();
  graph->GetEdges(edges);
  while (edges->HasNext())
    {
      vtkEdgeType e = edges->Next();
      pair<AddedEdgeIterator, AddedEdgeIterator>& bracket 
        = startPositions[helper->GetVertexIndex(e.Source)];
      
      // Make sure we're expecting to find more edges in this source's
      // bracket
      myassert(bracket.first != bracket.second);

      // Make sure that we added an edge with the same source/target
      vector<AddedEdge>::iterator found 
        = std::find(bracket.first, bracket.second,
                       AddedEdge(e.Source, e.Target));
      myassert(found != bracket.second);

      // Move this edge out of the way, so we don't find it again
      --bracket.second;
      std::swap(*found, *bracket.second);
    }
  // Ensure that all of the edges we added actually got added
  for (vtkIdType v = 0; v < V; ++v)
    {
    myassert(startPositions[v].first == startPositions[v].second);
    }
  MPI_Barrier(MPI_COMM_WORLD);
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }

  // Let everyone know about the in-edges they should have.
  std::sort(addedEdges.begin(), addedEdges.end(), OrderEdgesByTarget());
  vector<AddedEdge> inEdges;
  ExchangeEdges(graph, addedEdges, inEdges, false);

  // Test the incoming edges of each local vertex
  if (myRank == 0)
    {
    (cout << "  Testing in edges...").flush();
    }
  std::sort(inEdges.begin(), inEdges.end(), OrderEdgesByTarget());
  graph->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType u = vertices->Next();

    // Find bounds within the inEdges array where the incoming edges
    // for this node will occur.
    vector<AddedEdge>::iterator myEdgesStart, myEdgesEnd;
    myEdgesStart = std::lower_bound(inEdges.begin(), inEdges.end(),
                                       AddedEdge
                                         (helper->MakeDistributedId(0, 0), 
                                          u),
                                       OrderEdgesByTarget());
    myEdgesEnd = std::lower_bound(myEdgesStart, inEdges.end(),
                                     AddedEdge
                                       (helper->MakeDistributedId(0, 0),
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
      vector<AddedEdge>::iterator found 
        = std::find(myEdgesStart, myEdgesEnd,
                       AddedEdge(e.Source, u));
      myassert(found != myEdgesEnd);

      // Move this edge out of the way, so we don't find it again
      --myEdgesEnd;
      std::swap(*found, *myEdgesEnd);
      }

    // Make sure that the constructed graph isn't missing any edges
    assert(myEdgesStart == myEdgesEnd);
    }  
  MPI_Barrier(MPI_COMM_WORLD);
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }
}
//----------------------------------------------------------------------------
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
  srand((myRank + 1)*117);

  // The simplest test of all: add V vertices to each processor, then
  // each processor adds E randomly-generated edges to the
  // graph. Then, we'll test whether the distributed graph data
  // structure is consistent.
  const vtkIdType V = 10000;
  const vtkIdType E = 100000;
  vector<AddedEdge> generatedEdges;
  if (myRank == 0)
    {
    (cout << "Build distributed undirected graph with V=" << V*numProcs 
          << ", E=" << E*numProcs << "...").flush();
    }

  for (vtkIdType v = 0; v < V; ++v)
    {
    graph->AddVertex();
    }

  for (vtkIdType e = 0; e < E; ++e)
    {
    vtkIdType source = helper->MakeDistributedId(rand() % numProcs, rand() % V);
    vtkIdType target = helper->MakeDistributedId(rand() % numProcs, rand() % V);
    if (rand() % 100 < ImmediateAddEdgeChance)
      graph->AddEdge(source, target);
    else
      graph->LazyAddEdge(source, target);

    // If source and target are on the same processor, and source >
    // target, swap them. This ensures that the addedEdges list has
    // the edges with the same (source, target) order that we will see
    // them when traversing all of the edges of the graph with
    // vtkEdgeListIterator.
    if (helper->GetVertexOwner(source) == helper->GetVertexOwner(target)
        && source > target)
      std::swap(source, target);

    generatedEdges.push_back(AddedEdge(source, target));
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

  // We know which edges we generated, but some of those edges were
  // actually added on other nodes. Do a large exchange so that
  // addedEdges contains all of the edges that should originate on
  // this node.
  std::sort(generatedEdges.begin(), generatedEdges.end(), OrderEdgesBySource());
  std::vector<AddedEdge> addedEdges;
  ExchangeEdges(graph, generatedEdges, addedEdges, true);
  std::vector<AddedEdge>().swap(generatedEdges);

  // Test the vertex descriptors
  if (myRank == 0)
    {
    (cout << "  Testing vertex descriptors...").flush();
    }
  vtkIdType vExpected = helper->MakeDistributedId(myRank, 0);
  vtkSmartPointer<vtkVertexListIterator> vertices
    = vtkSmartPointer<vtkVertexListIterator>::New();
  graph->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType vActual = vertices->Next();
    myassert(vActual == vExpected);
    ++vExpected;
    }
  myassert(helper->GetVertexIndex(vExpected) == V);
  MPI_Barrier(MPI_COMM_WORLD);
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }

  // Find all of the incoming edges
  vector<AddedEdge> inEdges;
  std::sort(addedEdges.begin(), addedEdges.end(), OrderEdgesByTarget());
  ExchangeEdges(graph, addedEdges, inEdges, false);

  // Remove self-loops from the list of in-edges. We don't want them
  // to appear twice. 
  inEdges.erase(std::remove_if(inEdges.begin(), inEdges.end(), IsSelfLoop()),
                inEdges.end());

  // Build a list of all of the in/out edges we'll see. 
  vector<AddedEdge> allEdges;
  allEdges.reserve(addedEdges.size() + inEdges.size());
  allEdges.insert(allEdges.end(), addedEdges.begin(), addedEdges.end());
  for (vector<AddedEdge>::iterator e = inEdges.begin(); e != inEdges.end(); ++e)
    {
    allEdges.push_back(AddedEdge(e->Target, e->Source));
    }

  // Keep this list of all edges sorted.
  std::sort(allEdges.begin(), allEdges.end(), OrderEdgesBySource());

  // Test the outgoing edges of each local vertex
  if (myRank == 0)
    {
    (cout << "  Testing out edges...").flush();
    }
  typedef vector<AddedEdge>::iterator AddedEdgeIterator;
  graph->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType u = vertices->Next();
    vtkSmartPointer<vtkOutEdgeIterator> outEdges
      = vtkSmartPointer<vtkOutEdgeIterator>::New();

    // Find bounds within the allEdges array where the edges for
    // this node will occur.
    vector<AddedEdge>::iterator myEdgesStart, myEdgesEnd;
    myEdgesStart = std::lower_bound(allEdges.begin(), allEdges.end(),
                                       AddedEdge
                                         (u,
                                          helper->MakeDistributedId(0, 0)),
                                       OrderEdgesBySource());
    myEdgesEnd = std::lower_bound(myEdgesStart, allEdges.end(),
                                     AddedEdge
                                       (u+1, 
                                        helper->MakeDistributedId(0, 0)),
                                     OrderEdgesBySource());

    graph->GetOutEdges(u, outEdges);
    while (outEdges->HasNext()) 
      {
      vtkOutEdgeType e = outEdges->Next();

      // Make sure we're expecting to find more out-edges
      myassert(myEdgesStart != myEdgesEnd);

      // Make sure that we added an edge with the same source/target
      vector<AddedEdge>::iterator found 
        = std::find(myEdgesStart, myEdgesEnd,
                       AddedEdge(u, e.Target));
      myassert(found != myEdgesEnd);

      // Move this edge out of the way, so we don't find it again
      --myEdgesEnd;
      std::swap(*found, *myEdgesEnd);
      }

    // Make sure that the constructed graph isn't missing any edges
    assert(myEdgesStart == myEdgesEnd);
    }
  MPI_Barrier(MPI_COMM_WORLD);
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }

  // Test all of the local edges
  if (myRank == 0)
    {
    (cout << "  Testing all edges...").flush();
    }

  // Find where each of the local vertices has edges stored in the
  // allEdges list.
  std::sort(addedEdges.begin(), addedEdges.end(), OrderEdgesBySource());
  vector<pair<AddedEdgeIterator, AddedEdgeIterator> > startPositions(V);
  AddedEdgeIterator position = addedEdges.begin();
  for (vtkIdType v = 0; v < V; ++v)
    {
      vtkIdType vId = helper->MakeDistributedId(myRank, v);
      startPositions[v].first = position;
      while (position != addedEdges.end() && position->Source == vId) 
        {
        ++position;
        }
      startPositions[v].second = position;
    }

  vtkSmartPointer<vtkEdgeListIterator> edges
    = vtkSmartPointer<vtkEdgeListIterator>::New();
  graph->GetEdges(edges);
  while (edges->HasNext())
    {
      vtkEdgeType e = edges->Next();
      pair<AddedEdgeIterator, AddedEdgeIterator>& bracket 
        = startPositions[helper->GetVertexIndex(e.Source)];

      // Make sure we're expecting to find more edges in this source's
      // bracket
      myassert(bracket.first != bracket.second);

      // Make sure that we added an edge with the same source/target
      vector<AddedEdge>::iterator found 
        = std::find(bracket.first, bracket.second,
                       AddedEdge(e.Source, e.Target));
      myassert(found != bracket.second);

      // Move this edge out of the way, so we don't find it again
      --bracket.second;
      std::swap(*found, *bracket.second);
    }
  // Ensure that all of the edges we added actually got added
  for (vtkIdType v = 0; v < V; ++v)
    {
      if (startPositions[v].first != startPositions[v].second)
        {
          cerr << "Edges rank " << myRank 
               << " expected to find with vtkEdgeListIterator:\n";
          for (AddedEdgeIterator i = startPositions[v].first;
               i != startPositions[v].second; ++i)
            {
            cerr << "  (" << hex << i->Source << ", " << i->Target << ")\n";
            }
          cerr.flush();
        }
    myassert(startPositions[v].first == startPositions[v].second);
    }
  MPI_Barrier(MPI_COMM_WORLD);
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }

  // Test the incoming edges of each local vertex
  if (myRank == 0)
    {
    (cout << "  Testing in edges...").flush();
    }
  graph->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType v = vertices->Next();
    vtkSmartPointer<vtkInEdgeIterator> inEdges
      = vtkSmartPointer<vtkInEdgeIterator>::New();

    // Find bounds within the allEdges array where the edges for
    // this node will occur.
    vector<AddedEdge>::iterator myEdgesStart, myEdgesEnd;
    myEdgesStart = std::lower_bound(allEdges.begin(), allEdges.end(),
                                       AddedEdge
                                         (v,
                                          helper->MakeDistributedId(0, 0)),
                                       OrderEdgesBySource());
    myEdgesEnd = std::lower_bound(myEdgesStart, allEdges.end(),
                                     AddedEdge
                                       (v+1, 
                                        helper->MakeDistributedId(0, 0)),
                                     OrderEdgesBySource());

    graph->GetInEdges(v, inEdges);
    while (inEdges->HasNext()) 
      {
      vtkInEdgeType e = inEdges->Next();

      // Make sure we're expecting to find more in-edges
      myassert(myEdgesStart != myEdgesEnd);

      // Make sure that we added an edge with the same source/target
      vector<AddedEdge>::iterator found 
        = std::find(myEdgesStart, myEdgesEnd,
                       AddedEdge(v, e.Source));
      myassert(found != myEdgesEnd);

      // Move this edge out of the way, so we don't find it again
      --myEdgesEnd;
      std::swap(*found, *myEdgesEnd);
      }

    // Make sure that the constructed graph isn't missing any edges
    assert(myEdgesStart == myEdgesEnd);
    }
  MPI_Barrier(MPI_COMM_WORLD);
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }
}

//----------------------------------------------------------------------------
void TestUndirectedGraphProperties()
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
  srand((myRank + 1)*117);

  // The simplest test of all: add V vertices to each processor, then
  // each processor adds E randomly-generated edges to the
  // graph. Then, we'll test whether the distributed graph data
  // structure is consistent.
  const vtkIdType V = 10000;
  const vtkIdType E = 100000;
  vector<AddedEdge> generatedEdges;
  if (myRank == 0)
    {
    (cout << "Build distributed undirected graph, with properties, with V=" << V*numProcs 
          << ", E=" << E*numProcs << "...").flush();
    }

  //  Create some vertex property arrays
  vtkVariantArray* vertexPropertyArr = vtkVariantArray::New();
  int numVertexProperties = 2;
  vertexPropertyArr->SetNumberOfValues(numVertexProperties);
  
  vtkStringArray* vertexProp0Array = vtkStringArray::New();
  vertexProp0Array->SetName("labels");
  graph->GetVertexData()->AddArray(vertexProp0Array);
  
  vtkIntArray* vertexProp1Array = vtkIntArray::New();
  vertexProp1Array->SetName("veight");
  graph->GetVertexData()->AddArray(vertexProp1Array);
  
  const char *vertexLabel[] = {"Dick","Jane","Sally","Spot","Puff"};
  
  
  //  Create some edge property arrays
  vtkVariantArray* edgePropertyArr = vtkVariantArray::New();
  int numEdgeProperties = 2;
  edgePropertyArr->SetNumberOfValues(numEdgeProperties);
  
  vtkIntArray* edgeProp0Array = vtkIntArray::New();
  edgeProp0Array->SetName("happiness");
  graph->GetEdgeData()->AddArray(edgeProp0Array);
  
  vtkDoubleArray* edgeProp1Array = vtkDoubleArray::New();
  edgeProp1Array->SetName("weight");
  graph->GetEdgeData()->AddArray(edgeProp1Array);
  

  const char *stringProp = vertexLabel[0];
  int veight;
  for (vtkIdType v = 0; v < V; ++v)
    {
    stringProp = vertexLabel[rand() % 5];
    veight = rand() % 10;
//    cout << myRank <<" vertex "<< v <<","<< stringProp <<","<<veight<< endl;
    vertexPropertyArr->SetValue(0,stringProp);
    vertexPropertyArr->SetValue(1,veight);
    
    graph->AddVertex(vertexPropertyArr);
    }

  for (vtkIdType e = 0; e < E; ++e)
    {
    vtkIdType source = helper->MakeDistributedId(rand() % numProcs, rand() % V);
    vtkIdType target = helper->MakeDistributedId(rand() % numProcs, rand() % V);
    if (rand() % 100 < ImmediateAddEdgeChance)
      graph->AddEdge(source, target);
    else
      graph->LazyAddEdge(source, target);

    // If source and target are on the same processor, and source >
    // target, swap them. This ensures that the addedEdges list has
    // the edges with the same (source, target) order that we will see
    // them when traversing all of the edges of the graph with
    // vtkEdgeListIterator.
    if (helper->GetVertexOwner(source) == helper->GetVertexOwner(target)
        && source > target)
      std::swap(source, target);

    generatedEdges.push_back(AddedEdge(source, target));
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

  // We know which edges we generated, but some of those edges were
  // actually added on other nodes. Do a large exchange so that
  // addedEdges contains all of the edges that should originate on
  // this node.
  std::sort(generatedEdges.begin(), generatedEdges.end(), OrderEdgesBySource());
  std::vector<AddedEdge> addedEdges;
  ExchangeEdges(graph, generatedEdges, addedEdges, true);
  std::vector<AddedEdge>().swap(generatedEdges);

  // Test the vertex descriptors
  if (myRank == 0)
    {
    (cout << "  Testing vertex descriptors...").flush();
    }
  vtkIdType vExpected = helper->MakeDistributedId(myRank, 0);
  vtkSmartPointer<vtkVertexListIterator> vertices
    = vtkSmartPointer<vtkVertexListIterator>::New();
  graph->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType vActual = vertices->Next();
    myassert(vActual == vExpected);
    ++vExpected;
    }
  myassert(helper->GetVertexIndex(vExpected) == V);
  MPI_Barrier(MPI_COMM_WORLD);
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }

  // Find all of the incoming edges
  vector<AddedEdge> inEdges;
  std::sort(addedEdges.begin(), addedEdges.end(), OrderEdgesByTarget());
  ExchangeEdges(graph, addedEdges, inEdges, false);

  // Remove self-loops from the list of in-edges. We don't want them
  // to appear twice. 
  inEdges.erase(std::remove_if(inEdges.begin(), inEdges.end(), IsSelfLoop()),
                inEdges.end());

  // Build a list of all of the in/out edges we'll see. 
  vector<AddedEdge> allEdges;
  allEdges.reserve(addedEdges.size() + inEdges.size());
  allEdges.insert(allEdges.end(), addedEdges.begin(), addedEdges.end());
  for (vector<AddedEdge>::iterator e = inEdges.begin(); e != inEdges.end(); ++e)
    {
    allEdges.push_back(AddedEdge(e->Target, e->Source));
    }

  // Keep this list of all edges sorted.
  std::sort(allEdges.begin(), allEdges.end(), OrderEdgesBySource());

  // Test the outgoing edges of each local vertex
  if (myRank == 0)
    {
    (cout << "  Testing out edges...").flush();
    }
  typedef vector<AddedEdge>::iterator AddedEdgeIterator;
  graph->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType u = vertices->Next();
    vtkSmartPointer<vtkOutEdgeIterator> outEdges
      = vtkSmartPointer<vtkOutEdgeIterator>::New();

    // Find bounds within the allEdges array where the edges for
    // this node will occur.
    vector<AddedEdge>::iterator myEdgesStart, myEdgesEnd;
    myEdgesStart = std::lower_bound(allEdges.begin(), allEdges.end(),
                                       AddedEdge
                                         (u,
                                          helper->MakeDistributedId(0, 0)),
                                       OrderEdgesBySource());
    myEdgesEnd = std::lower_bound(myEdgesStart, allEdges.end(),
                                     AddedEdge
                                       (u+1, 
                                        helper->MakeDistributedId(0, 0)),
                                     OrderEdgesBySource());

    graph->GetOutEdges(u, outEdges);
    while (outEdges->HasNext()) 
      {
      vtkOutEdgeType e = outEdges->Next();

      // Make sure we're expecting to find more out-edges
      myassert(myEdgesStart != myEdgesEnd);

      // Make sure that we added an edge with the same source/target
      vector<AddedEdge>::iterator found 
        = std::find(myEdgesStart, myEdgesEnd,
                       AddedEdge(u, e.Target));
      myassert(found != myEdgesEnd);

      // Move this edge out of the way, so we don't find it again
      --myEdgesEnd;
      std::swap(*found, *myEdgesEnd);
      }

    // Make sure that the constructed graph isn't missing any edges
    assert(myEdgesStart == myEdgesEnd);
    }
  MPI_Barrier(MPI_COMM_WORLD);
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }

  // Test all of the local edges
  if (myRank == 0)
    {
    (cout << "  Testing all edges...").flush();
    }

  // Find where each of the local vertices has edges stored in the
  // allEdges list.
  std::sort(addedEdges.begin(), addedEdges.end(), OrderEdgesBySource());
  vector<pair<AddedEdgeIterator, AddedEdgeIterator> > startPositions(V);
  AddedEdgeIterator position = addedEdges.begin();
  for (vtkIdType v = 0; v < V; ++v)
    {
      vtkIdType vId = helper->MakeDistributedId(myRank, v);
      startPositions[v].first = position;
      while (position != addedEdges.end() && position->Source == vId) 
        {
        ++position;
        }
      startPositions[v].second = position;
    }

  vtkSmartPointer<vtkEdgeListIterator> edges
    = vtkSmartPointer<vtkEdgeListIterator>::New();
  graph->GetEdges(edges);
  while (edges->HasNext())
    {
      vtkEdgeType e = edges->Next();
      pair<AddedEdgeIterator, AddedEdgeIterator>& bracket 
        = startPositions[helper->GetVertexIndex(e.Source)];

      // Make sure we're expecting to find more edges in this source's
      // bracket
      myassert(bracket.first != bracket.second);

      // Make sure that we added an edge with the same source/target
      vector<AddedEdge>::iterator found 
        = std::find(bracket.first, bracket.second,
                       AddedEdge(e.Source, e.Target));
      myassert(found != bracket.second);

      // Move this edge out of the way, so we don't find it again
      --bracket.second;
      std::swap(*found, *bracket.second);
    }
  // Ensure that all of the edges we added actually got added
  for (vtkIdType v = 0; v < V; ++v)
    {
      if (startPositions[v].first != startPositions[v].second)
        {
          cerr << "Edges rank " << myRank 
               << " expected to find with vtkEdgeListIterator:\n";
          for (AddedEdgeIterator i = startPositions[v].first;
               i != startPositions[v].second; ++i)
            {
            cerr << "  (" << hex << i->Source << ", " << i->Target << ")\n";
            }
          cerr.flush();
        }
    myassert(startPositions[v].first == startPositions[v].second);
    }
  MPI_Barrier(MPI_COMM_WORLD);
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }

  // Test the incoming edges of each local vertex
  if (myRank == 0)
    {
    (cout << "  Testing in edges...").flush();
    }
  graph->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType v = vertices->Next();
    vtkSmartPointer<vtkInEdgeIterator> inEdges
      = vtkSmartPointer<vtkInEdgeIterator>::New();

    // Find bounds within the allEdges array where the edges for
    // this node will occur.
    vector<AddedEdge>::iterator myEdgesStart, myEdgesEnd;
    myEdgesStart = std::lower_bound(allEdges.begin(), allEdges.end(),
                                       AddedEdge
                                         (v,
                                          helper->MakeDistributedId(0, 0)),
                                       OrderEdgesBySource());
    myEdgesEnd = std::lower_bound(myEdgesStart, allEdges.end(),
                                     AddedEdge
                                       (v+1, 
                                        helper->MakeDistributedId(0, 0)),
                                     OrderEdgesBySource());

    graph->GetInEdges(v, inEdges);
    while (inEdges->HasNext()) 
      {
      vtkInEdgeType e = inEdges->Next();

      // Make sure we're expecting to find more in-edges
      myassert(myEdgesStart != myEdgesEnd);

      // Make sure that we added an edge with the same source/target
      vector<AddedEdge>::iterator found 
        = std::find(myEdgesStart, myEdgesEnd,
                       AddedEdge(v, e.Source));
      myassert(found != myEdgesEnd);

      // Move this edge out of the way, so we don't find it again
      --myEdgesEnd;
      std::swap(*found, *myEdgesEnd);
      }

    // Make sure that the constructed graph isn't missing any edges
    assert(myEdgesStart == myEdgesEnd);
    }
  MPI_Barrier(MPI_COMM_WORLD);
  if (myRank == 0)
    {
    (cout << "done.\n").flush();
    }
}
//----------------------------------------------------------------------------
int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  TestDirectedGraph();
  TestUndirectedGraph();
  TestDirectedGraphProperties();
  TestUndirectedGraphProperties();
  MPI_Finalize();
  return 0;
}
