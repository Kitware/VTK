// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkMutableUndirectedGraph.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"
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

namespace
{
struct SetUndirectedEdgesWorker
{
  template <typename ArrayT>
  void operator()(ArrayT* edges, std::vector<vtkVertexAdjacencyList>& adjacency,
    vtkIdType& numVerts, vtkIdType& numEdges)
  {
    auto tuples = vtk::DataArrayTupleRange<2>(edges);
    numEdges = static_cast<vtkIdType>(tuples.size());

    // Pass 1: find max vertex id.
    numVerts = 0;
    for (const auto tuple : tuples)
    {
      vtkIdType s = static_cast<vtkIdType>(tuple[0]);
      vtkIdType t = static_cast<vtkIdType>(tuple[1]);
      if (s >= numVerts)
      {
        numVerts = s + 1;
      }
      if (t >= numVerts)
      {
        numVerts = t + 1;
      }
    }

    adjacency.clear();
    adjacency.resize(numVerts);

    // Pass 2: count degrees and reserve.
    // Each edge adds to both endpoints (except self-loops add once).
    std::vector<vtkIdType> deg(numVerts, 0);
    for (const auto tuple : tuples)
    {
      vtkIdType s = static_cast<vtkIdType>(tuple[0]);
      vtkIdType t = static_cast<vtkIdType>(tuple[1]);
      deg[s]++;
      if (s != t)
      {
        deg[t]++;
      }
    }
    for (vtkIdType v = 0; v < numVerts; ++v)
    {
      adjacency[v].OutEdges.reserve(deg[v]);
    }

    // Pass 3: fill adjacency lists.
    vtkIdType e = 0;
    for (const auto tuple : tuples)
    {
      vtkIdType s = static_cast<vtkIdType>(tuple[0]);
      vtkIdType t = static_cast<vtkIdType>(tuple[1]);
      adjacency[s].OutEdges.emplace_back(t, e);
      if (s != t)
      {
        adjacency[t].OutEdges.emplace_back(s, e);
      }
      ++e;
    }
  }
};
} // anonymous namespace

//------------------------------------------------------------------------------
void vtkMutableUndirectedGraph::SetEdges(vtkDataArray* edges)
{
  if (!edges)
  {
    vtkErrorMacro("SetEdges called with null array.");
    return;
  }
  if (edges->GetNumberOfComponents() != 2)
  {
    vtkErrorMacro("SetEdges requires an array with exactly 2 components.");
    return;
  }

  this->ForceOwnership();

  vtkIdType numVerts = 0;
  vtkIdType numEdges = 0;
  SetUndirectedEdgesWorker worker;
  using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Integrals>;
  if (!Dispatcher::Execute(edges, worker, this->Internals->Adjacency, numVerts, numEdges))
  {
    // Fallback for non-dispatched types.
    worker(edges, this->Internals->Adjacency, numVerts, numEdges);
  }

  this->Internals->NumberOfEdges = numEdges;
  this->SetEdgeList(nullptr);
  this->GetVertexData()->SetNumberOfTuples(numVerts);
  this->GetEdgeData()->SetNumberOfTuples(numEdges);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkMutableUndirectedGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
