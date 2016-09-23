/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutEdgeIterator.cxx

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

#include "vtkOutEdgeIterator.h"

#include "vtkObjectFactory.h"
#include "vtkGraph.h"
#include "vtkGraphEdge.h"

vtkCxxSetObjectMacro(vtkOutEdgeIterator, Graph, vtkGraph);
vtkStandardNewMacro(vtkOutEdgeIterator);
//----------------------------------------------------------------------------
vtkOutEdgeIterator::vtkOutEdgeIterator()
{
  this->Vertex = 0;
  this->Current = 0;
  this->End = 0;
  this->Graph = 0;
  this->GraphEdge = 0;
}

//----------------------------------------------------------------------------
vtkOutEdgeIterator::~vtkOutEdgeIterator()
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
void vtkOutEdgeIterator::Initialize(vtkGraph *graph, vtkIdType v)
{
  this->SetGraph(graph);
  this->Vertex = v;
  vtkIdType nedges;
  this->Graph->GetOutEdges(this->Vertex, this->Current, nedges);
  this->End = this->Current + nedges;
}

//----------------------------------------------------------------------------
vtkGraphEdge *vtkOutEdgeIterator::NextGraphEdge()
{
  vtkOutEdgeType e = this->Next();
  if (!this->GraphEdge)
  {
    this->GraphEdge = vtkGraphEdge::New();
  }
  this->GraphEdge->SetSource(this->Vertex);
  this->GraphEdge->SetTarget(e.Target);
  this->GraphEdge->SetId(e.Id);
  return this->GraphEdge;
}

//----------------------------------------------------------------------------
void vtkOutEdgeIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Graph: " << (this->Graph ? "" : "(null)") << endl;
  if (this->Graph)
  {
    this->Graph->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "Vertex: " << this->Vertex << endl;
}
