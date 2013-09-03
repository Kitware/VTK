/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUndirectedGraph.cxx

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

#include "vtkUndirectedGraph.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOutEdgeIterator.h"
#include "vtkSmartPointer.h"

#include <vtksys/stl/vector>

//----------------------------------------------------------------------------
// class vtkUndirectedGraph
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkUndirectedGraph);
//----------------------------------------------------------------------------
vtkUndirectedGraph::vtkUndirectedGraph()
{
}

//----------------------------------------------------------------------------
vtkUndirectedGraph::~vtkUndirectedGraph()
{
}

//----------------------------------------------------------------------------
void vtkUndirectedGraph::GetInEdges(vtkIdType v, const vtkInEdgeType *& edges, vtkIdType & nedges)
{
  const vtkOutEdgeType *outEdges;
  this->GetOutEdges(v, outEdges, nedges);
  edges = reinterpret_cast<const vtkInEdgeType*>(outEdges);
}

//----------------------------------------------------------------------------
vtkInEdgeType vtkUndirectedGraph::GetInEdge(vtkIdType v, vtkIdType i)
{
  vtkOutEdgeType oe = this->GetOutEdge(v, i);
  vtkInEdgeType ie(oe.Target, oe.Id);
  return ie;
}

//----------------------------------------------------------------------------
vtkIdType vtkUndirectedGraph::GetInDegree(vtkIdType v)
{
  return this->GetOutDegree(v);
}

//----------------------------------------------------------------------------
vtkUndirectedGraph *vtkUndirectedGraph::GetData(vtkInformation *info)
{
  return info? vtkUndirectedGraph::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkUndirectedGraph *vtkUndirectedGraph::GetData(vtkInformationVector *v, int i)
{
  return vtkUndirectedGraph::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
bool vtkUndirectedGraph::IsStructureValid(vtkGraph *g)
{
  if (!g)
    {
    return false;
    }

  if (vtkUndirectedGraph::SafeDownCast(g))
    {
    return true;
    }

  // Verify that there are no in edges and that each edge
  // appears in exactly two edge lists.
  // Loop edges should be in exactly one edge list.
  vtksys_stl::vector<vtkIdType> place(g->GetNumberOfEdges(), -1);
  vtksys_stl::vector<vtkIdType> count(g->GetNumberOfEdges(), 0);
  vtkSmartPointer<vtkOutEdgeIterator> outIter =
    vtkSmartPointer<vtkOutEdgeIterator>::New();
  for (vtkIdType v = 0; v < g->GetNumberOfVertices(); ++v)
    {
    if (g->GetInDegree(v) > 0)
      {
      return false;
      }
    g->GetOutEdges(v, outIter);
    while (outIter->HasNext())
      {
      vtkOutEdgeType e = outIter->Next();
      if (place[e.Id] == v)
        {
        return false;
        }
      place[e.Id] = v;
      count[e.Id]++;
      // Count loops twice so they should all have count == 2
      if (v == e.Target)
        {
        count[e.Id]++;
        }
      }
    }
  for (vtkIdType i = 0; i < g->GetNumberOfEdges(); ++i)
    {
    if (count[i] != 2)
      {
      return false;
      }
    }

  return true;
}

//----------------------------------------------------------------------------
void vtkUndirectedGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
