/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInEdgeIterator.cxx

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

#include "vtkInEdgeIterator.h"

#include "vtkObjectFactory.h"
#include "vtkGraph.h"
#include "vtkGraphEdge.h"

vtkCxxSetObjectMacro(vtkInEdgeIterator, Graph, vtkGraph);
vtkStandardNewMacro(vtkInEdgeIterator);
//----------------------------------------------------------------------------
vtkInEdgeIterator::vtkInEdgeIterator()
{
  this->Vertex = 0;
  this->Current = 0;
  this->End = 0;
  this->Graph = 0;
  this->GraphEdge = 0;
}

//----------------------------------------------------------------------------
vtkInEdgeIterator::~vtkInEdgeIterator()
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
void vtkInEdgeIterator::Initialize(vtkGraph *graph, vtkIdType v)
{
  this->SetGraph(graph);
  this->Vertex = v;
  vtkIdType nedges;
  this->Graph->GetInEdges(this->Vertex, this->Current, nedges);
  this->End = this->Current + nedges;
}

//----------------------------------------------------------------------------
vtkGraphEdge *vtkInEdgeIterator::NextGraphEdge()
{
  vtkInEdgeType e = this->Next();
  if (!this->GraphEdge)
  {
    this->GraphEdge = vtkGraphEdge::New();
  }
  this->GraphEdge->SetSource(e.Source);
  this->GraphEdge->SetTarget(this->Vertex);
  this->GraphEdge->SetId(e.Id);
  return this->GraphEdge;
}

//----------------------------------------------------------------------------
void vtkInEdgeIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Graph: " << (this->Graph ? "" : "(null)") << endl;
  if (this->Graph)
  {
    this->Graph->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "Vertex: " << this->Vertex << endl;
}
