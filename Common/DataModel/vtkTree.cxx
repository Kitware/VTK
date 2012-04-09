/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTree.cxx

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

#include "vtkTree.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOutEdgeIterator.h"
#include "vtkSmartPointer.h"

#include <vtksys/stl/vector>

using vtksys_stl::vector;

vtkStandardNewMacro(vtkTree);
//----------------------------------------------------------------------------
vtkTree::vtkTree()
{
  this->Root = -1;
}

//----------------------------------------------------------------------------
vtkTree::~vtkTree()
{
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetChild(vtkIdType v, vtkIdType i)
{
  const vtkOutEdgeType *edges;
  vtkIdType nedges;
  this->GetOutEdges(v, edges, nedges);
  if (i < nedges)
    {
    return edges[i].Target;
    }
  return -1;
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetParent(vtkIdType v)
{
  const vtkInEdgeType *edges;
  vtkIdType nedges;
  this->GetInEdges(v, edges, nedges);
  if (nedges > 0)
    {
    return edges[0].Source;
    }
  return -1;
}

//----------------------------------------------------------------------------
vtkEdgeType vtkTree::GetParentEdge(vtkIdType v)
{
  const vtkInEdgeType *edges;
  vtkIdType nedges;
  this->GetInEdges(v, edges, nedges);
  if (nedges > 0)
    {
    return vtkEdgeType(edges[0].Source, v, edges[0].Id);
    }
  return vtkEdgeType();
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetLevel(vtkIdType vertex)
{
  if (vertex < 0 || vertex >= this->GetNumberOfVertices())
    {
    return -1;
    }
  vtkIdType level = 0;
  while (vertex != this->Root)
    {
    vertex = this->GetParent(vertex);
    level++;
    }
  return level;
}

//----------------------------------------------------------------------------
bool vtkTree::IsLeaf(vtkIdType vertex)
{
  return (this->GetNumberOfChildren(vertex) == 0);
}
  
//----------------------------------------------------------------------------
vtkTree *vtkTree::GetData(vtkInformation *info)
{
  return info? vtkTree::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkTree *vtkTree::GetData(vtkInformationVector *v, int i)
{
  return vtkTree::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
bool vtkTree::IsStructureValid(vtkGraph *g)
{
  if (vtkTree::SafeDownCast(g))
    {
    // Since a tree has the additional root propery, we need
    // to set that here.
    this->Root = (vtkTree::SafeDownCast(g))->Root;
    return true;
    }

  // Empty graph is a valid tree.
  if (g->GetNumberOfVertices() == 0)
    {
    this->Root = -1;
    return true;
    }

  // A tree must have one more vertex than its number of edges.
  if (g->GetNumberOfEdges() != g->GetNumberOfVertices() - 1)
    {
    return false;
    }

  // Find the root and fail if there is more than one.
  vtkIdType root = -1;
  for (vtkIdType v = 0; v < g->GetNumberOfVertices(); ++v)
    {
    vtkIdType indeg = g->GetInDegree(v);
    if (indeg > 1)
      {
      // No tree vertex should have in degree > 1, so fail.
      return false;
      }
    else if (indeg == 0 && root == -1)
      {
      // We found our first root.
      root = v;
      }
    else if (indeg == 0)
      {
      // We already found a root, so fail.
      return false;
      }
    }
  if (root < 0)
    {
    return false;
    }

  // Make sure the tree is connected with no cycles.
  vector<bool> visited(g->GetNumberOfVertices(), false);
  vector<vtkIdType> stack;
  stack.push_back(root);
  vtkSmartPointer<vtkOutEdgeIterator> outIter = 
    vtkSmartPointer<vtkOutEdgeIterator>::New();
  while (!stack.empty())
    {
    vtkIdType v = stack.back();
    stack.pop_back();
    visited[v] = true;
    g->GetOutEdges(v, outIter);
    while (outIter->HasNext())
      {
      vtkIdType id = outIter->Next().Target;
      if (!visited[id])
        {
        stack.push_back(id);
        }
      else
        {
        return false;
        }
      }
    }
  for (vtkIdType v = 0; v < g->GetNumberOfVertices(); ++v)
    {
    if (!visited[v])
      {
      return false;
      }
    }

  // Since a tree has the additional root propery, we need
  // to set that here.
  this->Root = root;

  return true;
}

//----------------------------------------------------------------------------
void vtkTree::ReorderChildren(vtkIdType parent, vtkIdTypeArray *children)
{
  this->ReorderOutVertices(parent, children);
}

//----------------------------------------------------------------------------
void vtkTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Root: " << this->Root << endl;
}
