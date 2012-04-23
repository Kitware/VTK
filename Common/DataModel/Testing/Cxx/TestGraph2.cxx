/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGraph2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME
// .SECTION Description
// This program tests functions in vtkGraph

#include "vtkMutableUndirectedGraph.h"
#include "vtkMutableDirectedGraph.h"
#include <limits>
#include "vtkSmartPointer.h"

#include <vector>

template<class A>
bool fuzzyCompare(A a, A b) {
  return fabs(a - b) < std::numeric_limits<A>::epsilon();
}

int TestGetEdgeId();
int TestToDirectedGraph();
int TestToUndirectedGraph();

int TestGraph2(int,char *[])
{
  std::vector<int> results;

  results.push_back(TestGetEdgeId());
  results.push_back(TestToDirectedGraph());
  results.push_back(TestToUndirectedGraph());

  for(unsigned int i = 0; i < results.size(); i++)
    {
    if(results[i] == EXIT_FAILURE)
      {
      return EXIT_FAILURE;
      }
    }

  return EXIT_SUCCESS;
}

int TestGetEdgeId()
{
  // Create a graph
  vtkSmartPointer<vtkMutableUndirectedGraph> g =
    vtkSmartPointer<vtkMutableUndirectedGraph>::New();
  vtkIdType v0 = g->AddVertex();
  vtkIdType v1 = g->AddVertex();
  vtkIdType v2 = g->AddVertex();

  vtkEdgeType e0 = g->AddEdge(v0, v1);
  vtkEdgeType e1 = g->AddEdge(v1, v2);

  // Test to make sure both edges (in either orientation) are found
  if(g->GetEdgeId(v0, v1) != e0.Id || g->GetEdgeId(v1, v0) != e0.Id)
    {
    return EXIT_FAILURE;
    }

  if(g->GetEdgeId(v1, v2) != e1.Id || g->GetEdgeId(v2, v1) != e1.Id)
    {
    return EXIT_FAILURE;
    }

  // Test to make sure -1 is returned if the edge does not exist
  if(g->GetEdgeId(v1, 3) != -1)
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

int TestToDirectedGraph()
{
  // Create an undirected graph
  vtkSmartPointer<vtkMutableUndirectedGraph> ug =
    vtkSmartPointer<vtkMutableUndirectedGraph>::New();
  vtkIdType v0 = ug->AddVertex();
  vtkIdType v1 = ug->AddVertex();
  vtkIdType v2 = ug->AddVertex();

  ug->AddEdge(v0, v1);
  ug->AddEdge(v1, v2);

  // Convert it to a directed graph
  vtkSmartPointer<vtkMutableDirectedGraph> dg =
    vtkSmartPointer<vtkMutableDirectedGraph>::New();
  ug->ToDirectedGraph(dg);

  // Check that the number of vertices and edges is unchanged
  if(ug->GetNumberOfVertices() != dg->GetNumberOfVertices() ||
     ug->GetNumberOfEdges() != dg->GetNumberOfEdges())
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

int TestToUndirectedGraph()
{
  // Create a directed graph
  vtkSmartPointer<vtkMutableDirectedGraph> dg =
    vtkSmartPointer<vtkMutableDirectedGraph>::New();
  vtkIdType v0 = dg->AddVertex();
  vtkIdType v1 = dg->AddVertex();
  vtkIdType v2 = dg->AddVertex();

  dg->AddEdge(v0, v1);
  dg->AddEdge(v1, v2);

  // Convert it to an undirected graph
  vtkSmartPointer<vtkMutableUndirectedGraph> ug =
    vtkSmartPointer<vtkMutableUndirectedGraph>::New();
  dg->ToUndirectedGraph(ug);

  // Check that the number of vertices and edges is unchanged
  if(ug->GetNumberOfVertices() != dg->GetNumberOfVertices() ||
     ug->GetNumberOfEdges() != dg->GetNumberOfEdges())
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
