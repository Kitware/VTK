// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkMutableGraphHelper.h"

#include "vtkGraphEdge.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkCxxSetObjectMacro(vtkMutableGraphHelper, InternalGraph, vtkGraph);
vtkStandardNewMacro(vtkMutableGraphHelper);
//------------------------------------------------------------------------------
vtkMutableGraphHelper::vtkMutableGraphHelper()
{
  this->InternalGraph = nullptr;
  this->DirectedGraph = nullptr;
  this->UndirectedGraph = nullptr;
  this->GraphEdge = vtkGraphEdge::New();
  this->GraphEdge->SetId(-1);
  this->GraphEdge->SetSource(-1);
  this->GraphEdge->SetTarget(-1);
}

//------------------------------------------------------------------------------
vtkMutableGraphHelper::~vtkMutableGraphHelper()
{
  if (this->InternalGraph)
  {
    this->InternalGraph->Delete();
  }
  this->GraphEdge->Delete();
}

//------------------------------------------------------------------------------
void vtkMutableGraphHelper::SetGraph(vtkGraph* g)
{
  this->SetInternalGraph(g);
  this->DirectedGraph = vtkMutableDirectedGraph::SafeDownCast(this->InternalGraph);
  this->UndirectedGraph = vtkMutableUndirectedGraph::SafeDownCast(this->InternalGraph);
  if (!this->DirectedGraph && !this->UndirectedGraph)
  {
    vtkErrorMacro("The graph must be mutable.");
  }
}

//------------------------------------------------------------------------------
vtkGraph* vtkMutableGraphHelper::GetGraph()
{
  return this->GetInternalGraph();
}

//------------------------------------------------------------------------------
vtkIdType vtkMutableGraphHelper::AddVertex()
{
  if (!this->InternalGraph)
  {
    return -1;
  }
  if (this->DirectedGraph)
  {
    return this->DirectedGraph->AddVertex();
  }
  else
  {
    return this->UndirectedGraph->AddVertex();
  }
}

//------------------------------------------------------------------------------
vtkEdgeType vtkMutableGraphHelper::AddEdge(vtkIdType u, vtkIdType v)
{
  if (!this->InternalGraph)
  {
    return vtkEdgeType();
  }
  if (this->DirectedGraph)
  {
    return this->DirectedGraph->AddEdge(u, v);
  }
  else
  {
    return this->UndirectedGraph->AddEdge(u, v);
  }
}

//------------------------------------------------------------------------------
vtkGraphEdge* vtkMutableGraphHelper::AddGraphEdge(vtkIdType u, vtkIdType v)
{
  if (!this->InternalGraph)
  {
    return this->GraphEdge;
  }
  if (this->DirectedGraph)
  {
    return this->DirectedGraph->AddGraphEdge(u, v);
  }
  else
  {
    return this->UndirectedGraph->AddGraphEdge(u, v);
  }
}

//------------------------------------------------------------------------------
void vtkMutableGraphHelper::RemoveVertex(vtkIdType v)
{
  if (!this->InternalGraph)
  {
    return;
  }
  if (this->DirectedGraph)
  {
    return this->DirectedGraph->RemoveVertex(v);
  }
  else
  {
    return this->UndirectedGraph->RemoveVertex(v);
  }
}

//------------------------------------------------------------------------------
void vtkMutableGraphHelper::RemoveVertices(vtkIdTypeArray* verts)
{
  if (!this->InternalGraph)
  {
    return;
  }
  if (this->DirectedGraph)
  {
    return this->DirectedGraph->RemoveVertices(verts);
  }
  else
  {
    return this->UndirectedGraph->RemoveVertices(verts);
  }
}

//------------------------------------------------------------------------------
void vtkMutableGraphHelper::RemoveEdge(vtkIdType e)
{
  if (!this->InternalGraph)
  {
    return;
  }
  if (this->DirectedGraph)
  {
    return this->DirectedGraph->RemoveEdge(e);
  }
  else
  {
    return this->UndirectedGraph->RemoveEdge(e);
  }
}

//------------------------------------------------------------------------------
void vtkMutableGraphHelper::RemoveEdges(vtkIdTypeArray* edges)
{
  if (!this->InternalGraph)
  {
    return;
  }
  if (this->DirectedGraph)
  {
    return this->DirectedGraph->RemoveEdges(edges);
  }
  else
  {
    return this->UndirectedGraph->RemoveEdges(edges);
  }
}

//------------------------------------------------------------------------------
void vtkMutableGraphHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "InternalGraph: " << (this->InternalGraph ? "" : "(null)") << endl;
  if (this->InternalGraph)
  {
    this->InternalGraph->PrintSelf(os, indent.GetNextIndent());
  }
}
VTK_ABI_NAMESPACE_END
