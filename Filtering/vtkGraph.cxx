/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraph.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkGraph.h"

#include "vtkObjectFactory.h"
#include "vtkIdTypeArray.h"
#include "vtkGraphIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkVertexLinks.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

#include <vtkstd/algorithm>

// 
// Standard functions
//

vtkCxxRevisionMacro(vtkGraph, "1.6");
vtkStandardNewMacro(vtkGraph);

//----------------------------------------------------------------------------

void vtkGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Edges: " << endl;
  this->Edges->PrintSelf(os, indent.GetNextIndent());
  os << indent << "VertexLinks: " << endl;
  this->VertexLinks->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Directed: " << (this->Directed ? "yes" : "no") << endl;
}

//----------------------------------------------------------------------------

vtkGraph::vtkGraph()
{
  this->Directed = 0;
  this->Edges = vtkIdTypeArray::New();
  this->Edges->SetNumberOfComponents(2);
  this->VertexLinks = vtkVertexLinks::New();
}

//----------------------------------------------------------------------------

void vtkGraph::Initialize()
{
  this->Superclass::Initialize();
  this->Directed = 0;
  this->Edges->Delete();
  this->Edges = vtkIdTypeArray::New();
  this->Edges->SetNumberOfComponents(2);
  this->VertexLinks->Delete();
  this->VertexLinks = vtkVertexLinks::New();
}

//----------------------------------------------------------------------------

vtkGraph::~vtkGraph()
{
  if (this->Edges)
    {
    this->Edges->Delete();
    this->Edges = NULL;
    }
  if (this->VertexLinks)
    {
    this->VertexLinks->Delete();
    this->VertexLinks = NULL;
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetNumberOfEdges()
{
  return this->Edges->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetNumberOfVertices()
{
  return this->VertexLinks->GetNumberOfVertices();
}

//----------------------------------------------------------------------------
void vtkGraph::GetAdjacentVertices(vtkIdType vertex, vtkGraphIdList* vertexIds)
{
  vertexIds->Reset();
  vtkIdType nedges;
  const vtkIdType* edges;
  this->VertexLinks->GetAdjacent(vertex, nedges, edges);
  for (vtkIdType i = 0; i < nedges; i++)
    {
    vertexIds->InsertNextId(this->GetOppositeVertex(edges[i], vertex));
    }
}

//----------------------------------------------------------------------------
void vtkGraph::GetInVertices(vtkIdType vertex, vtkGraphIdList* vertexIds)
{
  if (!this->Directed)
    {
    this->GetAdjacentVertices(vertex, vertexIds);
    return;
    }
  vertexIds->Reset();
  vtkIdType nedges;
  const vtkIdType* edges;
  this->VertexLinks->GetInAdjacent(vertex, nedges, edges);
  for (vtkIdType i = 0; i < nedges; i++)
    {
    vertexIds->InsertNextId(this->GetOppositeVertex(edges[i], vertex));
    }
}

//----------------------------------------------------------------------------
void vtkGraph::GetOutVertices(vtkIdType vertex, vtkGraphIdList* vertexIds)
{
  if (!this->Directed)
    {
    this->GetAdjacentVertices(vertex, vertexIds);
    return;
    }
  vertexIds->Reset();
  vtkIdType nedges;
  const vtkIdType* edges;
  this->VertexLinks->GetOutAdjacent(vertex, nedges, edges);
  for (vtkIdType i = 0; i < nedges; i++)
    {
    vertexIds->InsertNextId(this->GetOppositeVertex(edges[i], vertex));
    }
}

//----------------------------------------------------------------------------
void vtkGraph::GetIncidentEdges(vtkIdType vertex, vtkGraphIdList* edgeIds)
{
  vtkIdType nedges;
  const vtkIdType* edges;
  this->VertexLinks->GetAdjacent(vertex, nedges, edges);
  edgeIds->SetArray(const_cast<vtkIdType*>(edges), nedges, true);
}

//----------------------------------------------------------------------------
void vtkGraph::GetIncidentEdges(vtkIdType vertex, vtkIdType& nedges, const vtkIdType*& edges)
{
  this->VertexLinks->GetAdjacent(vertex, nedges, edges);
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetDegree(vtkIdType vertex)
{
  return this->VertexLinks->GetDegree(vertex);
}

//----------------------------------------------------------------------------
void vtkGraph::GetInEdges(vtkIdType vertex, vtkGraphIdList* edgeIds)
{
  if (!this->Directed)
    {
    this->GetIncidentEdges(vertex, edgeIds);
    return;
    }
  vtkIdType nedges;
  const vtkIdType* edges;
  this->VertexLinks->GetInAdjacent(vertex, nedges, edges);
  edgeIds->SetArray(const_cast<vtkIdType*>(edges), nedges, true);
}

//----------------------------------------------------------------------------
void vtkGraph::GetInEdges(vtkIdType vertex, vtkIdType& nedges, const vtkIdType*& edges)
{
  if (!this->Directed)
    {
    this->GetIncidentEdges(vertex, nedges, edges);
    return;
    }
  this->VertexLinks->GetInAdjacent(vertex, nedges, edges);
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetInDegree(vtkIdType vertex)
{
  if (!this->Directed)
    {
    return this->GetDegree(vertex);
    }
  return this->VertexLinks->GetInDegree(vertex);
}

//----------------------------------------------------------------------------
void vtkGraph::GetOutEdges(vtkIdType vertex, vtkGraphIdList* edgeIds)
{
  if (!this->Directed)
    {
    this->GetIncidentEdges(vertex, edgeIds);
    return;
    }
  vtkIdType nedges;
  const vtkIdType* edges;
  this->VertexLinks->GetOutAdjacent(vertex, nedges, edges);
  edgeIds->SetArray(const_cast<vtkIdType*>(edges), nedges, true);
}

//----------------------------------------------------------------------------
void vtkGraph::GetOutEdges(vtkIdType vertex, vtkIdType& nedges, const vtkIdType*& edges)
{
  if (!this->Directed)
    {
    this->GetIncidentEdges(vertex, nedges, edges);
    return;
    }
  this->VertexLinks->GetOutAdjacent(vertex, nedges, edges);
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetOutDegree(vtkIdType vertex)
{
  if (!this->Directed)
    {
    return this->GetDegree(vertex);
    }
  return this->VertexLinks->GetOutDegree(vertex);
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetSourceVertex(vtkIdType edge)
{
  return this->Edges->GetValue(2*edge);
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetTargetVertex(vtkIdType edge)
{
  return this->Edges->GetValue(2*edge + 1);
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetOppositeVertex(vtkIdType edge, vtkIdType vertex)
{
  if (this->GetSourceVertex(edge) != vertex)
    {
    return this->GetSourceVertex(edge);
    }
  return this->GetTargetVertex(edge);
}

//----------------------------------------------------------------------------
void vtkGraph::SetNumberOfVertices(vtkIdType vertices)
{
  if (vertices >= this->GetNumberOfVertices())
    {
    for (vtkIdType i = this->GetNumberOfVertices(); i < vertices; i++)
      {
      this->AddVertex();
      }
    }
  else
    {
    for (vtkIdType i = this->GetNumberOfVertices() - 1; i >= vertices; i--)
      {
      this->RemoveVertex(i);
      }
    }
}

//----------------------------------------------------------------------------
void vtkGraph::ShallowCopy(vtkDataObject *dataObject)
{
  vtkGraph* graph = vtkGraph::SafeDownCast(dataObject);

  if ( graph != NULL )
    {
    if (this->Edges)
      {
      this->Edges->Delete();
      }
    this->Edges = graph->Edges;
    if (this->Edges)
      {
      this->Edges->Register(this);
      }

    if (this->VertexLinks)
      {
      this->VertexLinks->Delete();
      }
    this->VertexLinks = graph->VertexLinks;
    if (this->VertexLinks)
      {
      this->VertexLinks->Register(this);
      }

    this->Directed = graph->Directed;

    }

  // Do superclass
  this->Superclass::ShallowCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkGraph::DeepCopy(vtkDataObject *dataObject)
{
  vtkGraph* graph = vtkGraph::SafeDownCast(dataObject);

  if ( graph != NULL )
    {
    this->Edges->DeepCopy(graph->Edges);
    this->VertexLinks->DeepCopy(graph->VertexLinks);
    this->Directed = graph->Directed;
    }

  // Do superclass
  this->Superclass::DeepCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkGraph::CopyStructure(vtkDataSet* ds)
{
  vtkGraph* graph = vtkGraph::SafeDownCast(ds);

  if (graph != NULL)
    {
    this->Edges->DeepCopy(graph->Edges);
    this->VertexLinks->DeepCopy(graph->VertexLinks);
    this->Directed = graph->Directed;
    }

  // Do superclass
  this->Superclass::CopyStructure(ds);
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::AddVertex()
{
  return this->VertexLinks->AddVertex();
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::AddEdge(vtkIdType source, vtkIdType target)
{
  if (source > this->GetNumberOfVertices() - 1 || target > this->GetNumberOfVertices() - 1)
    {
    this->SetNumberOfVertices(source > target ? source + 1 : target + 1);
    }

  //cout << "inserting edge from " << source << " to " << target << endl;
  vtkIdType edge = this->Edges->InsertNextValue(source) / 2;
  this->Edges->InsertNextValue(target);

  // Insert the edge into the adjacency lists
  this->VertexLinks->AddOutAdjacent(source, edge);
  this->VertexLinks->AddInAdjacent(target, edge);

  return edge;
}

//----------------------------------------------------------------------------
void vtkGraph::RemoveVertex(vtkIdType vertex)
{
  // Delete any edges adjacent to the vertex.
  // We need to remove the const for sorting the edges.
  // Remove the out edges, then the in edges.
  vtkIdType out;
  const vtkIdType* outEdges;
  this->VertexLinks->GetOutAdjacent(vertex, out, outEdges);
  this->RemoveEdges(const_cast<vtkIdType*>(outEdges), out);
  vtkIdType in;
  const vtkIdType* inEdges;
  this->VertexLinks->GetInAdjacent(vertex, in, inEdges);
  this->RemoveEdges(const_cast<vtkIdType*>(inEdges), in);

  // Move the final vertex on top of the deleted vertex
  vtkIdType movedVertex = this->VertexLinks->RemoveVertex(vertex);

  if (movedVertex != vertex)
    {
    vtkIdType vertexDegree;
    const vtkIdType* vertexEdges;
    this->VertexLinks->GetAdjacent(vertex, vertexDegree, vertexEdges);
    for (vtkIdType e = 0; e < this->VertexLinks->GetInDegree(vertex); e++)
      {
      this->Edges->SetValue(2*vertexEdges[e] + 1, vertex);
      }
    for (vtkIdType e = this->VertexLinks->GetInDegree(vertex); e < vertexDegree; e++)
      {
      this->Edges->SetValue(2*vertexEdges[e], vertex);
      }
    }

  // Move the data of the final vertex on top of the data of the deleted vertex
  for (int i = 0; i < this->GetPointData()->GetNumberOfArrays(); i++)
    {
    vtkAbstractArray* aa = this->GetPointData()->GetAbstractArray(i);
    aa->SetTuple(vertex, movedVertex, aa);
    aa->Resize(aa->GetNumberOfTuples() - 1);
    }
  if (this->Points)
    {
    this->Points->SetPoint(vertex, this->Points->GetPoint(movedVertex));
    // NOTE:
    // vtkPoints does not have a resize method, so we have to do this the slow way.
    // The fast way would be:
    //this->Points->Resize(this->Points->GetNumberOfPoints() - 1);
    vtkPoints* newPoints = vtkPoints::New();
    for (vtkIdType i = 0; i < this->Points->GetNumberOfPoints() - 1; i++)
      {
      newPoints->InsertNextPoint(this->Points->GetPoint(i));
      }
    this->Points->Delete();
    this->Points = newPoints;
    }
}

//----------------------------------------------------------------------------
void vtkGraph::RemoveEdge(vtkIdType edge)
{
  // Remove the edge from the source edge list
  vtkIdType source = this->Edges->GetValue(2*edge);
  this->VertexLinks->RemoveOutAdjacent(source, edge);
  vtkIdType target = this->Edges->GetValue(2*edge + 1);
  this->VertexLinks->RemoveInAdjacent(target, edge);

  // Move the final edge on top of the deleted edge
  vtkIdType movedEdge = this->GetNumberOfEdges() - 1;
  vtkIdType movedSource = this->Edges->GetValue(2*movedEdge);
  vtkIdType movedTarget = this->Edges->GetValue(2*movedEdge + 1);

  this->Edges->SetValue(2*edge, movedSource);
  this->Edges->SetValue(2*edge + 1, movedTarget);
  this->Edges->Resize(this->Edges->GetNumberOfTuples() - 1);

  // Modify the adjacency lists to reflect the id change
  for (vtkIdType e = 0; e < this->VertexLinks->GetOutDegree(movedSource); e++)
    {
    if (this->VertexLinks->GetOutAdjacent(movedSource, e) == movedEdge)
      {
      this->VertexLinks->SetOutAdjacent(movedSource, e, edge);
      break;
      }
    }
  for (vtkIdType e = 0; e < this->VertexLinks->GetInDegree(movedTarget); e++)
    {
    if (this->VertexLinks->GetInAdjacent(movedTarget, e) == movedEdge)
      {
      this->VertexLinks->SetInAdjacent(movedTarget, e, edge);
      break;
      }
    }

  // Move the data of the final edge on top of the data of the deleted edge
  for (int i = 0; i < this->GetCellData()->GetNumberOfArrays(); i++)
    {
    vtkAbstractArray* aa = this->GetCellData()->GetAbstractArray(i);
    aa->SetTuple(edge, movedEdge, aa);
    aa->Resize(aa->GetNumberOfTuples() - 1);
    }
}

//----------------------------------------------------------------------------
void vtkGraph::RemoveVertices(vtkIdType* vertices, vtkIdType size)
{
  // Sort the vertices
  vtkstd::sort(vertices, vertices + size);

  // Delete the vertices in reverse order
  for (vtkIdType i = size - 1; i >= 0; i--)
    {
    // Don't delete the same vertex twice
    if (i == size - 1 || vertices[i] != vertices[i+1])
      {
      this->RemoveVertex(vertices[i]);
      }
    }
}

//----------------------------------------------------------------------------
void vtkGraph::RemoveEdges(vtkIdType* edges, vtkIdType size)
{
  // Sort the edges
  vtkstd::sort(edges, edges + size);

  // Delete the edges in reverse order
  for (vtkIdType i = size - 1; i >= 0; i--)
    {
    // Don't delete the same edge twice.
    // This may happen if there are loops in the graph.
    if (i == size - 1 || edges[i] != edges[i+1])
      {
      this->RemoveEdge(edges[i]);
      }
    }
}

void vtkGraph::ClearVertex(vtkIdType vertex)
{
  // Delete any edges adjacent to the vertex.
  // We need to remove the const for sorting the edges.
  // Remove the out edges, then the in edges.
  vtkIdType out;
  const vtkIdType* outEdges;
  this->VertexLinks->GetOutAdjacent(vertex, out, outEdges);
  this->RemoveEdges(const_cast<vtkIdType*>(outEdges), out);
  vtkIdType in;
  const vtkIdType* inEdges;
  this->VertexLinks->GetInAdjacent(vertex, in, inEdges);
  this->RemoveEdges(const_cast<vtkIdType*>(inEdges), in);
}

//----------------------------------------------------------------------------
vtkGraph* vtkGraph::GetData(vtkInformation* info)
{
  return info? vtkGraph::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkGraph* vtkGraph::GetData(vtkInformationVector* v, int i)
{
  return vtkGraph::GetData(v->GetInformationObject(i));
}

