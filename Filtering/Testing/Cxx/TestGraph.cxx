/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGraph.cxx

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
#include <vtkstd/limits>
#include "vtkSmartPointer.h"

template<class A>
bool fuzzyCompare(A a, A b) {
  return fabs(a - b) < vtkstd::numeric_limits<A>::epsilon();
}

int TestGraph(int,char *[])
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
