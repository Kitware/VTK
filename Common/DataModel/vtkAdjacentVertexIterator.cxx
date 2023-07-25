// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkAdjacentVertexIterator.h"

#include "vtkGraph.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkCxxSetObjectMacro(vtkAdjacentVertexIterator, Graph, vtkGraph);
vtkStandardNewMacro(vtkAdjacentVertexIterator);
//------------------------------------------------------------------------------
vtkAdjacentVertexIterator::vtkAdjacentVertexIterator()
{
  this->Vertex = 0;
  this->Current = nullptr;
  this->End = nullptr;
  this->Graph = nullptr;
}

//------------------------------------------------------------------------------
vtkAdjacentVertexIterator::~vtkAdjacentVertexIterator()
{
  if (this->Graph)
  {
    this->Graph->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkAdjacentVertexIterator::Initialize(vtkGraph* graph, vtkIdType v)
{
  this->SetGraph(graph);
  this->Vertex = v;
  vtkIdType nedges;
  this->Graph->GetOutEdges(this->Vertex, this->Current, nedges);
  this->End = this->Current + nedges;
}

//------------------------------------------------------------------------------
void vtkAdjacentVertexIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Graph: " << (this->Graph ? "" : "(null)") << endl;
  if (this->Graph)
  {
    this->Graph->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "Vertex: " << this->Vertex << endl;
}
VTK_ABI_NAMESPACE_END
