// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkMutableUndirectedGraph.h"

#include "vtkDataSetAttributes.h"
#include "vtkGraphEdge.h"
#include "vtkGraphInternals.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
// class vtkMutableUndirectedGraph
//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkMutableUndirectedGraph);
//------------------------------------------------------------------------------
vtkMutableUndirectedGraph::vtkMutableUndirectedGraph()
{
  this->GraphEdge = vtkGraphEdge::New();
}

//------------------------------------------------------------------------------
vtkMutableUndirectedGraph::~vtkMutableUndirectedGraph()
{
  this->GraphEdge->Delete();
}

//------------------------------------------------------------------------------
vtkIdType vtkMutableUndirectedGraph::SetNumberOfVertices(vtkIdType numVerts)
{
  vtkIdType retval = -1;

  if (this->GetDistributedGraphHelper())
  {
    vtkWarningMacro("SetNumberOfVertices will not work on distributed graphs.");
    return retval;
  }

  retval = static_cast<vtkIdType>(this->Internals->Adjacency.size());
  this->Internals->Adjacency.resize(numVerts);
  return retval;
}

//------------------------------------------------------------------------------
vtkIdType vtkMutableUndirectedGraph::AddVertex()
{
  if (this->Internals->UsingPedigreeIds && this->GetDistributedGraphHelper() != nullptr)
  {
    vtkErrorMacro("Adding vertex without a pedigree ID into a distributed graph that uses pedigree "
                  "IDs to name vertices");
  }

  return this->AddVertex(nullptr);
}
//------------------------------------------------------------------------------
vtkIdType vtkMutableUndirectedGraph::AddVertex(vtkVariantArray* propertyArr)
{
  if (this->GetVertexData()->GetPedigreeIds() != nullptr)
  {
    this->Internals->UsingPedigreeIds = true;
  }

  vtkIdType vertex;
  this->AddVertexInternal(propertyArr, &vertex);
  return vertex;
}

//------------------------------------------------------------------------------
vtkIdType vtkMutableUndirectedGraph::AddVertex(const vtkVariant& pedigreeId)
{
  this->Internals->UsingPedigreeIds = true;

  vtkIdType vertex;
  this->AddVertexInternal(pedigreeId, &vertex);
  return vertex;
}

//------------------------------------------------------------------------------
vtkEdgeType vtkMutableUndirectedGraph::AddEdge(vtkIdType u, vtkIdType v)
{
  return this->AddEdge(u, v, nullptr);
}

//------------------------------------------------------------------------------
vtkEdgeType vtkMutableUndirectedGraph::AddEdge(
  vtkIdType u, vtkIdType v, vtkVariantArray* propertyArr)
{
  vtkEdgeType e;
  this->AddEdgeInternal(u, v, false, propertyArr, &e);
  return e;
}

//------------------------------------------------------------------------------
vtkEdgeType vtkMutableUndirectedGraph::AddEdge(
  const vtkVariant& u, vtkIdType v, vtkVariantArray* propertyArr)
{
  this->Internals->UsingPedigreeIds = true;

  vtkEdgeType e;
  this->AddEdgeInternal(u, v, false, propertyArr, &e);
  return e;
}

//------------------------------------------------------------------------------
vtkEdgeType vtkMutableUndirectedGraph::AddEdge(
  vtkIdType u, const vtkVariant& v, vtkVariantArray* propertyArr)
{
  this->Internals->UsingPedigreeIds = true;

  vtkEdgeType e;
  this->AddEdgeInternal(u, v, false, propertyArr, &e);
  return e;
}

//------------------------------------------------------------------------------
vtkEdgeType vtkMutableUndirectedGraph::AddEdge(
  const vtkVariant& u, const vtkVariant& v, vtkVariantArray* propertyArr)
{
  this->Internals->UsingPedigreeIds = true;

  vtkEdgeType e;
  this->AddEdgeInternal(u, v, false, propertyArr, &e);
  return e;
}

//------------------------------------------------------------------------------
void vtkMutableUndirectedGraph::LazyAddVertex()
{
  if (this->Internals->UsingPedigreeIds && this->GetDistributedGraphHelper() != nullptr)
  {
    vtkErrorMacro("Adding vertex without a pedigree ID into a distributed graph that uses pedigree "
                  "IDs to name vertices");
  }

  this->LazyAddVertex(nullptr);
}

//------------------------------------------------------------------------------
void vtkMutableUndirectedGraph::LazyAddVertex(vtkVariantArray* propertyArr)
{
  if (this->GetVertexData()->GetPedigreeIds() != nullptr)
  {
    this->Internals->UsingPedigreeIds = true;
  }

  this->AddVertexInternal(propertyArr, nullptr);
}

//------------------------------------------------------------------------------
void vtkMutableUndirectedGraph::LazyAddVertex(const vtkVariant& pedigreeId)
{
  this->Internals->UsingPedigreeIds = true;

  this->AddVertexInternal(pedigreeId, nullptr);
}

//------------------------------------------------------------------------------
void vtkMutableUndirectedGraph::LazyAddEdge(vtkIdType u, vtkIdType v)
{
  this->LazyAddEdge(u, v, nullptr);
}

//------------------------------------------------------------------------------
void vtkMutableUndirectedGraph::LazyAddEdge(vtkIdType u, vtkIdType v, vtkVariantArray* propertyArr)
{
  this->AddEdgeInternal(u, v, false, propertyArr, nullptr);
}

//------------------------------------------------------------------------------
void vtkMutableUndirectedGraph::LazyAddEdge(
  const vtkVariant& u, vtkIdType v, vtkVariantArray* propertyArr)
{
  this->Internals->UsingPedigreeIds = true;

  this->AddEdgeInternal(u, v, false, propertyArr, nullptr);
}

//------------------------------------------------------------------------------
void vtkMutableUndirectedGraph::LazyAddEdge(
  vtkIdType u, const vtkVariant& v, vtkVariantArray* propertyArr)
{
  this->Internals->UsingPedigreeIds = true;

  this->AddEdgeInternal(u, v, false, propertyArr, nullptr);
}

//------------------------------------------------------------------------------
void vtkMutableUndirectedGraph::LazyAddEdge(
  const vtkVariant& u, const vtkVariant& v, vtkVariantArray* propertyArr)
{
  this->Internals->UsingPedigreeIds = true;

  this->AddEdgeInternal(u, v, false, propertyArr, nullptr);
}

//------------------------------------------------------------------------------
vtkGraphEdge* vtkMutableUndirectedGraph::AddGraphEdge(vtkIdType u, vtkIdType v)
{
  vtkEdgeType e = this->AddEdge(u, v);
  this->GraphEdge->SetSource(e.Source);
  this->GraphEdge->SetTarget(e.Target);
  this->GraphEdge->SetId(e.Id);
  return this->GraphEdge;
}

//------------------------------------------------------------------------------
void vtkMutableUndirectedGraph::RemoveVertex(vtkIdType v)
{
  this->RemoveVertexInternal(v, false);
}

//------------------------------------------------------------------------------
void vtkMutableUndirectedGraph::RemoveEdge(vtkIdType e)
{
  this->RemoveEdgeInternal(e, false);
}

//------------------------------------------------------------------------------
void vtkMutableUndirectedGraph::RemoveVertices(vtkIdTypeArray* arr)
{
  this->RemoveVerticesInternal(arr, false);
}

//------------------------------------------------------------------------------
void vtkMutableUndirectedGraph::RemoveEdges(vtkIdTypeArray* arr)
{
  this->RemoveEdgesInternal(arr, false);
}

//------------------------------------------------------------------------------
void vtkMutableUndirectedGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
