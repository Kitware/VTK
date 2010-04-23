/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdjacentVertexIterator.cxx

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

#include "vtkAdjacentVertexIterator.h"

#include "vtkObjectFactory.h"
#include "vtkGraph.h"

vtkCxxSetObjectMacro(vtkAdjacentVertexIterator, Graph, vtkGraph);
vtkStandardNewMacro(vtkAdjacentVertexIterator);
//----------------------------------------------------------------------------
vtkAdjacentVertexIterator::vtkAdjacentVertexIterator()
{
  this->Vertex = 0;
  this->Current = 0;
  this->End = 0;
  this->Graph = 0;
}

//----------------------------------------------------------------------------
vtkAdjacentVertexIterator::~vtkAdjacentVertexIterator()
{
  if (this->Graph)
    {
    this->Graph->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkAdjacentVertexIterator::Initialize(vtkGraph *graph, vtkIdType v)
{
  this->SetGraph(graph);
  this->Vertex = v;
  vtkIdType nedges;
  this->Graph->GetOutEdges(this->Vertex, this->Current, nedges);
  this->End = this->Current + nedges;
}

//----------------------------------------------------------------------------
void vtkAdjacentVertexIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Graph: " << (this->Graph ? "" : "(null)") << endl;
  if (this->Graph)
    {
    this->Graph->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "Vertex: " << this->Vertex << endl;
}
