// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkDirectedAcyclicGraph.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOutEdgeIterator.h"
#include "vtkSmartPointer.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDirectedAcyclicGraph);
//------------------------------------------------------------------------------
vtkDirectedAcyclicGraph::vtkDirectedAcyclicGraph() = default;

//------------------------------------------------------------------------------
vtkDirectedAcyclicGraph::~vtkDirectedAcyclicGraph() = default;

//------------------------------------------------------------------------------
vtkDirectedAcyclicGraph* vtkDirectedAcyclicGraph::GetData(vtkInformation* info)
{
  return info ? vtkDirectedAcyclicGraph::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//------------------------------------------------------------------------------
vtkDirectedAcyclicGraph* vtkDirectedAcyclicGraph::GetData(vtkInformationVector* v, int i)
{
  return vtkDirectedAcyclicGraph::GetData(v->GetInformationObject(i));
}

enum
{
  DFS_WHITE,
  DFS_GRAY,
  DFS_BLACK
};

//------------------------------------------------------------------------------
static bool vtkDirectedAcyclicGraphDFSVisit(
  vtkGraph* g, vtkIdType u, std::vector<int> color, vtkOutEdgeIterator* adj)
{
  color[u] = DFS_GRAY;
  g->GetOutEdges(u, adj);
  while (adj->HasNext())
  {
    vtkOutEdgeType e = adj->Next();
    vtkIdType v = e.Target;
    if (color[v] == DFS_WHITE)
    {
      if (!vtkDirectedAcyclicGraphDFSVisit(g, v, color, adj))
      {
        return false;
      }
    }
    else if (color[v] == DFS_GRAY)
    {
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkDirectedAcyclicGraph::IsStructureValid(vtkGraph* g)
{
  if (!g)
  {
    return false;
  }
  if (vtkDirectedAcyclicGraph::SafeDownCast(g))
  {
    return true;
  }

  // Empty graph is a valid DAG.
  if (g->GetNumberOfVertices() == 0)
  {
    return true;
  }

  // A directed graph is acyclic iff a depth-first search of
  // the graph yields no back edges.
  // (from Introduction to Algorithms.
  // Cormen, Leiserson, Rivest, p. 486).
  vtkIdType numVerts = g->GetNumberOfVertices();
  std::vector<int> color(numVerts, DFS_WHITE);
  vtkSmartPointer<vtkOutEdgeIterator> adj = vtkSmartPointer<vtkOutEdgeIterator>::New();
  for (vtkIdType s = 0; s < numVerts; ++s)
  {
    if (color[s] == DFS_WHITE)
    {
      if (!vtkDirectedAcyclicGraphDFSVisit(g, s, color, adj))
      {
        return false;
      }
    }
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkDirectedAcyclicGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
