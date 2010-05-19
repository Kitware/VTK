/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDirectedAcyclicGraph.cxx

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

#include "vtkDirectedAcyclicGraph.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOutEdgeIterator.h"
#include "vtkSmartPointer.h"

#include <vtksys/stl/vector>

using vtksys_stl::vector;

vtkStandardNewMacro(vtkDirectedAcyclicGraph);
//----------------------------------------------------------------------------
vtkDirectedAcyclicGraph::vtkDirectedAcyclicGraph()
{
}

//----------------------------------------------------------------------------
vtkDirectedAcyclicGraph::~vtkDirectedAcyclicGraph()
{
}

//----------------------------------------------------------------------------
vtkDirectedAcyclicGraph *vtkDirectedAcyclicGraph::GetData(vtkInformation *info)
{
  return info? vtkDirectedAcyclicGraph::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkDirectedAcyclicGraph *vtkDirectedAcyclicGraph::GetData(vtkInformationVector *v, int i)
{
  return vtkDirectedAcyclicGraph::GetData(v->GetInformationObject(i));
}

enum { DFS_WHITE, DFS_GRAY, DFS_BLACK };

//----------------------------------------------------------------------------
bool vtkDirectedAcyclicGraphDFSVisit(
  vtkGraph *g,
  vtkIdType u, 
  vtksys_stl::vector<int> color, 
  vtkOutEdgeIterator *adj)
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

//----------------------------------------------------------------------------
bool vtkDirectedAcyclicGraph::IsStructureValid(vtkGraph *g)
{
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
  vector<int> color(numVerts, DFS_WHITE);
  vtkSmartPointer<vtkOutEdgeIterator> adj = 
    vtkSmartPointer<vtkOutEdgeIterator>::New();
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

//----------------------------------------------------------------------------
void vtkDirectedAcyclicGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
