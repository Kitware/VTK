/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEdgeListIterator.cxx

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

#include "vtkEdgeListIterator.h"

#include "vtkDirectedGraph.h"
#include "vtkDistributedGraphHelper.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkGraph.h"
#include "vtkGraphEdge.h"

vtkStandardNewMacro(vtkEdgeListIterator);
//----------------------------------------------------------------------------
vtkEdgeListIterator::vtkEdgeListIterator()
{
  this->Vertex = 0;
  this->Current = 0;
  this->End = 0;
  this->Graph = 0;
  this->Directed = false;
  this->GraphEdge = 0;
}

//----------------------------------------------------------------------------
vtkEdgeListIterator::~vtkEdgeListIterator()
{
  if (this->Graph)
    {
    this->Graph->Delete();
    }
  if (this->GraphEdge)
    {
    this->GraphEdge->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkEdgeListIterator::SetGraph(vtkGraph *graph)
{
  vtkSetObjectBodyMacro(Graph, vtkGraph, graph);
  this->Current = 0;
  this->End = 0;
  if (this->Graph && this->Graph->GetNumberOfEdges() > 0)
    {
    this->Directed = (vtkDirectedGraph::SafeDownCast(this->Graph) != 0);
    this->Vertex = 0;
    vtkIdType lastVertex = this->Graph->GetNumberOfVertices();

    int myRank = -1;
    vtkDistributedGraphHelper *helper
      = this->Graph->GetDistributedGraphHelper();
    if (helper)
      {
      myRank = this->Graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
      this->Vertex = helper->MakeDistributedId(myRank, this->Vertex);
      lastVertex = helper->MakeDistributedId(myRank, lastVertex);
      }

    // Find a vertex with nonzero out degree.
    while (this->Vertex < lastVertex &&
           this->Graph->GetOutDegree(this->Vertex) == 0)
      {
      ++this->Vertex;
      }
    if (this->Vertex < lastVertex)
      {
      vtkIdType nedges;
      this->Graph->GetOutEdges(this->Vertex, this->Current, nedges);
      this->End = this->Current + nedges;
      // If it is undirected, skip edges that are non-local or
      // entirely-local edges whose source is greater than the target.
      if (!this->Directed)
        {
        while (this->Current != 0
               && (// Skip non-local edges.
                   (helper && helper->GetEdgeOwner(this->Current->Id) != myRank)
                   // Skip entirely-local edges where Source > Target
                   || (((helper
                         && myRank == helper->GetVertexOwner(this->Current->Target))
                        || !helper)
                       && this->Vertex > this->Current->Target)))
          {
          this->Increment();
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
vtkEdgeType vtkEdgeListIterator::Next()
{
  // First, determine the current item.
  vtkEdgeType e(this->Vertex, this->Current->Target, this->Current->Id);

  // Next, increment the iterator.
  this->Increment();
  // If it is undirected, skip edges that are non-local or
  // entirely-local edges whose source is greater than the target.
  if (!this->Directed)
    {
    int myRank = -1;
    vtkDistributedGraphHelper *helper
      = this->Graph->GetDistributedGraphHelper();

    if (helper)
      {
      myRank = this->Graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
      }

    while (this->Current != 0
           && (// Skip non-local edges.
               (helper && helper->GetEdgeOwner(this->Current->Id) != myRank)
               // Skip entirely-local edges where Source > Target
               || (((helper
                     && myRank == helper->GetVertexOwner(this->Current->Target))
                    || !helper)
                   && this->Vertex > this->Current->Target)))
      {
      this->Increment();
      }
    }

  // Return the current item.
  return e;
}

//----------------------------------------------------------------------------
vtkGraphEdge *vtkEdgeListIterator::NextGraphEdge()
{
  vtkEdgeType e = this->Next();
  if (!this->GraphEdge)
    {
    this->GraphEdge = vtkGraphEdge::New();
    }
  this->GraphEdge->SetSource(e.Source);
  this->GraphEdge->SetTarget(e.Target);
  this->GraphEdge->SetId(e.Id);
  return this->GraphEdge;
}

//----------------------------------------------------------------------------
void vtkEdgeListIterator::Increment()
{
  if (!this->Graph)
    {
    return;
    }

  vtkIdType lastVertex = this->Graph->GetNumberOfVertices();

  vtkDistributedGraphHelper *helper = this->Graph->GetDistributedGraphHelper();
  if (helper)
    {
    int myRank
      = this->Graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
    this->Vertex = helper->MakeDistributedId(myRank, this->Vertex);
    lastVertex = helper->MakeDistributedId(myRank, lastVertex);
    }

  ++this->Current;
  if (this->Current == this->End)
    {
    // Find a vertex with nonzero out degree.
    ++this->Vertex;
    while (this->Vertex < lastVertex &&
           this->Graph->GetOutDegree(this->Vertex) == 0)
      {
      ++this->Vertex;
      }

    // If there is another vertex with out edges, get its edges.
    // Otherwise, signal that we have reached the end of the iterator.
    if (this->Vertex < lastVertex)
      {
      vtkIdType nedges;
      this->Graph->GetOutEdges(this->Vertex, this->Current, nedges);
      this->End = this->Current + nedges;
      }
    else
      {
      this->Current = 0;
      }
    }
}

//----------------------------------------------------------------------------
bool vtkEdgeListIterator::HasNext()
{
  return (this->Current != 0);
}

//----------------------------------------------------------------------------
void vtkEdgeListIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Graph: " << (this->Graph ? "" : "(null)") << endl;
  if (this->Graph)
    {
    this->Graph->PrintSelf(os, indent.GetNextIndent());
    }
}
