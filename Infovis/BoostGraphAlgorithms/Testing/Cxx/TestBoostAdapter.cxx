/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBoostAdapter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkGraph.h"
#include "vtkBoostGraphAdapter.h"
#include "vtkIntArray.h"
#include "vtkIdTypeArray.h"
#include "vtkMath.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"

#include <vtksys/stl/vector>
#include <vtksys/stl/map>
#include <vtksys/stl/utility>

// In Boost 1.34.1, boost/pending/relaxed_heap.hpp does not include <climits>
// but uses CHAR_BIT. gcc>=4.3 has stricter and cleaner header files
// and fails with this version of Boost.
#include <boost/version.hpp>
#if BOOST_VERSION < 103500
#include <climits> // defines CHAR_BIT
#endif

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/transitive_closure.hpp>

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

using namespace boost;
using namespace vtksys_stl;

template <typename Graph>
void TestTraversal(Graph g, int repeat, int& vtkNotUsed(errors))
{
  typedef typename graph_traits<Graph>::edge_descriptor Edge;
  typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
  
  VTK_CREATE(vtkTimerLog, timer);
  typename graph_traits<Graph>::vertex_iterator vi, viEnd;

  // Traverse the edge list of each vertex
  timer->StartTimer();
  int count = 0;
  for (int r = 0; r < repeat; r++)
    {
    for (tie(vi, viEnd) = vertices(g); vi != viEnd; ++vi)
      {
      typename graph_traits<Graph>::out_edge_iterator oi, oiEnd;
      tie(oi, oiEnd) = out_edges(*vi, g);
      count++;
      }
    }
  timer->StopTimer();
  double time_out_edges = timer->GetElapsedTime();
  cerr << "getting out edges: " << time_out_edges / count << " sec." << endl;

  Edge e = *(edges(g).first);
  Vertex v = *(vertices(g).first);
  vector<Edge> edge_vec;
  vector<Vertex> vert_vec;

  timer->StartTimer();
  count = 0;
  for (int r = 0; r < repeat; r++)
    {
    for (tie(vi, viEnd) = vertices(g); vi != viEnd; ++vi)
      {
      typename graph_traits<Graph>::out_edge_iterator oi, oiEnd;
      for (tie(oi, oiEnd) = out_edges(*vi, g); oi != oiEnd; ++oi)
        {
        count++;
        }
      }
    }
  timer->StopTimer();
  double time_inc = timer->GetElapsedTime();
  cerr << "+increment: " << time_inc / count << " sec." << endl;
  cerr << "  just increment: " << (time_inc - time_out_edges) / count << " sec." << endl;

  timer->StartTimer();
  count = 0;
  for (int r = 0; r < repeat; r++)
    {
    edge_vec.clear();
    vert_vec.clear();
    for (tie(vi, viEnd) = vertices(g); vi != viEnd; ++vi)
      {
      typename graph_traits<Graph>::out_edge_iterator oi, oiEnd;
      for (tie(oi, oiEnd) = out_edges(*vi, g); oi != oiEnd; ++oi)
        {
        edge_vec.push_back(e);
        vert_vec.push_back(v);
        count++;
        }
      }
    }
  timer->StopTimer();
  double time_push_back = timer->GetElapsedTime();
  cerr << "+push_back: " << time_push_back / count << " sec." << endl;
  cerr << "  just push_back: " << (time_push_back - time_inc) / count << " sec." << endl;

  timer->StartTimer();
  count = 0;
  for (int r = 0; r < repeat; r++)
    {
    edge_vec.clear();
    vert_vec.clear();
    for (tie(vi, viEnd) = vertices(g); vi != viEnd; ++vi)
      {
      typename graph_traits<Graph>::out_edge_iterator oi, oiEnd;
      for (tie(oi, oiEnd) = out_edges(*vi, g); oi != oiEnd; ++oi)
        {
        Edge e1 = *oi;
        edge_vec.push_back(e1);
        vert_vec.push_back(v);
        count++;
        }
      }
    }
  timer->StopTimer();
  double time_deref = timer->GetElapsedTime();
  cerr << "+dereference: " << time_deref / count << " sec." << endl;
  cerr << "  just dereference: " << (time_deref - time_push_back) / count << " sec." << endl;

  timer->StartTimer();
  count = 0;
  for (int r = 0; r < repeat; r++)
    {
    edge_vec.clear();
    vert_vec.clear();
    for (tie(vi, viEnd) = vertices(g); vi != viEnd; ++vi)
      {
      typename graph_traits<Graph>::out_edge_iterator oi, oiEnd;
      for (tie(oi, oiEnd) = out_edges(*vi, g); oi != oiEnd; ++oi)
        {
        Edge e1 = *oi;
        edge_vec.push_back(e1);
        Vertex v1 = target(e1, g);
        vert_vec.push_back(v1);
        count++;
        }
      }
    }
  timer->StopTimer();
  double time_target = timer->GetElapsedTime();
  cerr << "+target: " << time_target / count << " sec." << endl;
  cerr << "  just target: " << (time_target - time_deref) / count << " sec." << endl;
}

template <typename Graph>
void TestGraph(Graph g, vtkIdType numVertices, vtkIdType numEdges, int repeat, int& errors)
{
  typedef typename graph_traits<Graph>::edge_descriptor Edge;
  typedef typename graph_traits<Graph>::vertex_descriptor Vertex;

  VTK_CREATE(vtkTimerLog, timer);
  
  vector<Vertex> graphVerts;
  vector<Edge> graphEdges;
  typename graph_traits<Graph>::vertex_iterator vi, viEnd;

  // Create a graph
  timer->StartTimer();
  for (int i = 0; i < numVertices; ++i)
    {
    add_vertex(g);
    }
  timer->StopTimer();
  cerr << "vertex insertion: " << timer->GetElapsedTime() / numVertices  << " sec." << endl;
  
  if (static_cast<int>(num_vertices(g)) != numVertices)
    {
    cerr << "ERROR: Number of vertices (" << num_vertices(g) 
         << ") not as expected (" << numVertices << ")." << endl;
    errors++;
    }

  for (tie(vi, viEnd) = vertices(g); vi != viEnd; ++vi)
    {
    graphVerts.push_back(*vi);
    }

  timer->StartTimer();
  for (int i = 0; i < numEdges; ++i)
    {
    int u = static_cast<int>(vtkMath::Random(0, numVertices));
    int v = static_cast<int>(vtkMath::Random(0, numVertices));
    add_edge(graphVerts[u], graphVerts[v], g);
    }
  timer->StopTimer();
  cerr << "edge insertion: " << timer->GetElapsedTime() / numEdges  << " sec." << endl;

  if (static_cast<int>(num_edges(g)) != numEdges)
    {
    cerr << "ERROR: Number of edges (" << num_edges(g) 
         << ") not as expected (" << numEdges << ")." << endl;
    errors++;
    }

  typename graph_traits<Graph>::edge_iterator ei, eiEnd;
  for (tie(ei, eiEnd) = edges(g); ei != eiEnd; ++ei)
    {
    graphEdges.push_back(*ei);
    }
  
  TestTraversal(g, repeat, errors);
  
#if 0
  timer->StartTimer();
  while (num_edges(g) > 0)
    {
    remove_edge(*(edges(g).first), g);
    }
  timer->StopTimer();
  cerr << "edge deletion: " << timer->GetElapsedTime() / numEdges  << " sec." << endl;

  // Perform edge deletions followed by accesses
  timer->StartTimer();
  while (num_vertices(g) > 0)
    {
    remove_vertex(*(vertices(g).first), g);
    }
  timer->StopTimer();
  cerr << "vertex deletion: " << timer->GetElapsedTime() / numVertices  << " sec." << endl;
#endif
}

int TestBoostAdapter(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int errors = 0;
  int repeat = 100;
  vtkIdType numVertices = 1000;
  vtkIdType numEdges = 2000;

  cerr << "Testing boost list graph..." << endl;
  typedef adjacency_list<listS, listS, directedS, property<vertex_index_t, unsigned int>, property<edge_index_t, unsigned int>, no_property, listS> ListGraph;
  ListGraph listGraph;
  TestGraph(listGraph, numVertices, numEdges, repeat, errors);
  cerr << "...done." << endl << endl;
  
  cerr << "Testing boost vector graph..." << endl;
  typedef adjacency_list<vecS, vecS, directedS, property<vertex_index_t, unsigned int>, property<edge_index_t, unsigned int>, no_property, vecS> VectorGraph;
  VectorGraph vectorGraph;
  TestGraph(vectorGraph, numVertices, numEdges, repeat, errors);
  cerr << "...done." << endl << endl;

  cerr << "Testing undirected graph adapter..." << endl;
  vtkMutableUndirectedGraph* ug = vtkMutableUndirectedGraph::New();
  TestGraph(ug, numVertices, numEdges, repeat, errors);
  ug->Delete();
  cerr << "...done." << endl << endl;

  cerr << "Testing directed graph adapter..." << endl;
  vtkMutableDirectedGraph* dg = vtkMutableDirectedGraph::New();
  TestGraph(dg, numVertices, numEdges, repeat, errors);
  dg->Delete();
  cerr << "...done." << endl << endl;
  
  cerr << "Testing tree adapter..." << endl;
  vtkMutableDirectedGraph *builder = vtkMutableDirectedGraph::New();
  builder->AddVertex();
  for (vtkIdType i = 1; i < numVertices; i++)
    {
    builder->AddChild(static_cast<vtkIdType>(vtkMath::Random(0, i)));
    }
  vtkTree* t = vtkTree::New();
  if (!t->CheckedShallowCopy(builder))
    {
    cerr << "Invalid tree structure!" << endl;
    ++errors;
    }
  TestTraversal(t, repeat, errors);
  builder->Delete();
  t->Delete();
  cerr << "...done." << endl << endl;
  
  return errors;
}
