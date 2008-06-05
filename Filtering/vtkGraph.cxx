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

#include "vtkAdjacentVertexIterator.h"
#include "vtkDataSetAttributes.h"
#include "vtkDistributedGraphHelper.h"
#include "vtkEdgeListIterator.h"
#include "vtkGraphInternals.h"
#include "vtkIdTypeArray.h"
#include "vtkInEdgeIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOutEdgeIterator.h"
#include "vtkPoints.h"
#include "vtkVertexListIterator.h"
#include "vtkVariantArray.h"
#include "vtkStringArray.h"

#include <assert.h>
#include <vtksys/stl/vector>

using vtksys_stl::vector;

double vtkGraph::DefaultPoint[3] = {0, 0, 0};

//----------------------------------------------------------------------------
// class vtkGraph
//----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkGraph, Points, vtkPoints);
vtkCxxSetObjectMacro(vtkGraph, Internals, vtkGraphInternals);
vtkCxxRevisionMacro(vtkGraph, "1.12.4.16");
//----------------------------------------------------------------------------
vtkGraph::vtkGraph()
{
  this->VertexData = vtkDataSetAttributes::New();
  this->EdgeData = vtkDataSetAttributes::New();
  this->Points = 0;
  vtkMath::UninitializeBounds(this->Bounds);

  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_PIECES_EXTENT);
  this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);

  this->Internals = vtkGraphInternals::New();
}

//----------------------------------------------------------------------------
vtkGraph::~vtkGraph()
{
  this->VertexData->Delete();
  this->EdgeData->Delete();
  if (this->Points)
    {
    this->Points->Delete();
    }
  this->Internals->Delete();
}

//----------------------------------------------------------------------------
double *vtkGraph::GetPoint(vtkIdType ptId)
{
  if (this->Points)
    {
    return this->Points->GetPoint(ptId);
    }
  return this->DefaultPoint;
}

//----------------------------------------------------------------------------
void vtkGraph::GetPoint(vtkIdType ptId, double x[3])
{
  if (this->Points)
    {
    vtkIdType index = ptId;
    if (vtkDistributedGraphHelper *helper = this->GetDistributedGraphHelper())
      {
        int myRank = this->Information->Get(vtkDataObject::DATA_PIECE_NUMBER());
        if (myRank != helper->GetVertexOwner(ptId))
          {
          vtkErrorMacro("vtkGraph cannot retrieve a point for a non-local vertex");
          }

        index = helper->GetVertexIndex(ptId);
      }

    this->Points->GetPoint(index, x);
    }
  else
    {
    for (int i = 0; i < 3; i++)
      {
      x[i] = this->DefaultPoint[i];
      }
    }
}

//----------------------------------------------------------------------------
vtkPoints *vtkGraph::GetPoints()
{
  if (!this->Points)
    {
    this->Points = vtkPoints::New();
    }
  if (this->Points->GetNumberOfPoints() != this->GetNumberOfVertices())
    {
    this->Points->SetNumberOfPoints(this->GetNumberOfVertices());
    for (vtkIdType i = 0; i < this->GetNumberOfVertices(); i++)
      {
      this->Points->SetPoint(i, 0, 0, 0);
      }
    }
  return this->Points;
}

//----------------------------------------------------------------------------
void vtkGraph::ComputeBounds()
{
  double *bounds;

  if ( this->Points )
    {
    bounds = this->Points->GetBounds();
    for (int i=0; i<6; i++)
      {
      this->Bounds[i] = bounds[i];
      }
    // TODO: how to compute the bounds for a distributed graph?
    this->ComputeTime.Modified();
    }
}

//----------------------------------------------------------------------------
double *vtkGraph::GetBounds()
{
  this->ComputeBounds();
  return this->Bounds;
}

//----------------------------------------------------------------------------
void vtkGraph::GetBounds(double bounds[6])
{
  this->ComputeBounds();
  for (int i=0; i<6; i++)
    {
    bounds[i] = this->Bounds[i];
    }
}

//----------------------------------------------------------------------------
unsigned long int vtkGraph::GetMTime()
{
  unsigned long int doTime = vtkDataObject::GetMTime();

  if ( this->VertexData->GetMTime() > doTime )
    {
    doTime = this->VertexData->GetMTime();
    }
  if ( this->EdgeData->GetMTime() > doTime )
    {
    doTime = this->EdgeData->GetMTime();
    }
  if ( this->Points ) 
    {
    if ( this->Points->GetMTime() > doTime )
      {
      doTime = this->Points->GetMTime();
      }
    }

  return doTime;
}

//----------------------------------------------------------------------------
void vtkGraph::Initialize()
{
  this->ForceOwnership();
  Superclass::Initialize();
  this->EdgeData->Initialize();
  this->VertexData->Initialize();
  this->Internals->NumberOfEdges = 0;
  this->Internals->Adjacency.clear();
}

//----------------------------------------------------------------------------
void vtkGraph::GetOutEdges(vtkIdType v, vtkOutEdgeIterator *it)
{
  if (vtkDistributedGraphHelper *helper = this->GetDistributedGraphHelper())
    {
    int myRank = this->Information->Get(vtkDataObject::DATA_PIECE_NUMBER());
    if (myRank != helper->GetVertexOwner(v))
      {
      vtkErrorMacro("vtkGraph cannot retrieve the out edges for a non-local vertex");
      }
    }

  if (it)
    {
    it->Initialize(this, v);
    }
}

//----------------------------------------------------------------------------
void vtkGraph::GetOutEdges(vtkIdType v, const vtkOutEdgeType *& edges, vtkIdType & nedges)
{
  vtkIdType index = v;
  if (vtkDistributedGraphHelper *helper = this->GetDistributedGraphHelper())
    {
    int myRank = this->Information->Get(vtkDataObject::DATA_PIECE_NUMBER());
    if (myRank != helper->GetVertexOwner(v))
      {
      vtkErrorMacro("vtkGraph cannot retrieve the out edges for a non-local vertex");
      }

    index = helper->GetVertexIndex(v);
    }

  nedges = this->Internals->Adjacency[index].OutEdges.size();
  if (nedges > 0)
    {
    edges = &(this->Internals->Adjacency[index].OutEdges[0]);
    }
  else
    {
    edges = 0;
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetOutDegree(vtkIdType v)
{
  vtkIdType index = v;
  if (vtkDistributedGraphHelper *helper = this->GetDistributedGraphHelper())
    {
    int myRank = this->Information->Get(vtkDataObject::DATA_PIECE_NUMBER());
    if (myRank != helper->GetVertexOwner(v))
      {
      vtkErrorMacro("vtkGraph cannot determine the out degree for a non-local vertex");
      }

    index = helper->GetVertexIndex(v);
    }
  return this->Internals->Adjacency[index].OutEdges.size();
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetDegree(vtkIdType v)
{
  vtkIdType index = v;
 
  if (vtkDistributedGraphHelper *helper = this->GetDistributedGraphHelper())
    {
    int myRank = this->Information->Get(vtkDataObject::DATA_PIECE_NUMBER());
    if (myRank != helper->GetVertexOwner(v))
      {
      vtkErrorMacro("vtkGraph cannot determine the degree for a non-local vertex");
      }

    index = helper->GetVertexIndex(v);
    }

  return this->Internals->Adjacency[index].InEdges.size() +
         this->Internals->Adjacency[index].OutEdges.size();
}

//----------------------------------------------------------------------------
void vtkGraph::GetInEdges(vtkIdType v, vtkInEdgeIterator *it)
{
  if (vtkDistributedGraphHelper *helper = this->GetDistributedGraphHelper())
    {
    int myRank = this->Information->Get(vtkDataObject::DATA_PIECE_NUMBER());
    if (myRank != helper->GetVertexOwner(v))
      {
      vtkErrorMacro("vtkGraph cannot retrieve the in edges for a non-local vertex");
      }
    }

  if (it)
    {
    it->Initialize(this, v);
    }
}

//----------------------------------------------------------------------------
void vtkGraph::GetInEdges(vtkIdType v, const vtkInEdgeType *& edges, vtkIdType & nedges)
{
  vtkIdType index = v;

  if (vtkDistributedGraphHelper *helper = this->GetDistributedGraphHelper())
    {
    int myRank = this->Information->Get(vtkDataObject::DATA_PIECE_NUMBER());
    if (myRank != helper->GetVertexOwner(v))
      {
      vtkErrorMacro("vtkGraph cannot retrieve the in edges for a non-local vertex");
      }

    index = helper->GetVertexIndex(v);
    }

  nedges = this->Internals->Adjacency[index].InEdges.size();
  if (nedges > 0)
    {
    edges = &(this->Internals->Adjacency[index].InEdges[0]);
    }
  else
    {
    edges = 0;
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetInDegree(vtkIdType v)
{
  vtkIdType index = v;

  if (vtkDistributedGraphHelper *helper = this->GetDistributedGraphHelper())
    {
    int myRank = this->Information->Get(vtkDataObject::DATA_PIECE_NUMBER());
    if (myRank != helper->GetVertexOwner(v))
      {
      vtkErrorMacro("vtkGraph cannot determine the in degree for a non-local vertex");
      }

    index = helper->GetVertexIndex(v);
    }

  return this->Internals->Adjacency[index].InEdges.size();
}

//----------------------------------------------------------------------------
void vtkGraph::GetAdjacentVertices(vtkIdType v, vtkAdjacentVertexIterator *it)
{
  if (vtkDistributedGraphHelper *helper = this->GetDistributedGraphHelper())
    {
    int myRank = this->Information->Get(vtkDataObject::DATA_PIECE_NUMBER());
    if (myRank != helper->GetVertexOwner(v))
      {
      vtkErrorMacro("vtkGraph cannot retrieve the adjacent vertices for a non-local vertex");
      }
    }

  if (it)
    {
    it->Initialize(this, v);
    }
}

//----------------------------------------------------------------------------
void vtkGraph::GetEdges(vtkEdgeListIterator *it)
{
  if (it)
    {
    it->SetGraph(this);
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetNumberOfEdges()
{
  return this->Internals->NumberOfEdges;
}

//----------------------------------------------------------------------------
void vtkGraph::GetVertices(vtkVertexListIterator *it)
{
  if (it)
    {
    it->SetGraph(this);
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetNumberOfVertices()
{
  return this->Internals->Adjacency.size();
}

//----------------------------------------------------------------------------
void vtkGraph::SetDistributedGraphHelper(vtkDistributedGraphHelper *helper)
{
  if (this->Internals->DistributedHelper)
    this->Internals->DistributedHelper->AttachToGraph(0);

  this->Internals->DistributedHelper = helper;
  if (this->Internals->DistributedHelper)
    {
    this->Internals->DistributedHelper->Register(this);
    this->Internals->DistributedHelper->AttachToGraph(this);
    }
}

//----------------------------------------------------------------------------
vtkDistributedGraphHelper *vtkGraph::GetDistributedGraphHelper()
{
  return this->Internals->DistributedHelper;
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::FindVertex(const vtkVariant& pedigreeId)
{
  vtkAbstractArray *abstract = this->GetVertexData()->GetPedigreeIds();
  if (abstract == NULL)
    {
    return -1;
    }
  vtkVariantArray *pedigrees = vtkVariantArray::SafeDownCast(abstract);
  if (pedigrees == NULL)
    {
    return -1;
    }

  if (vtkDistributedGraphHelper *helper = this->GetDistributedGraphHelper())
    {
    vtkIdType myRank 
      = this->Information->Get(vtkDataObject::DATA_PIECE_NUMBER());
    if (helper->GetVertexOwnerByPedigreeId(pedigreeId) 
          != myRank)
      {
      // The vertex is remote; ask the distributed graph helper to
      // find it.
      return helper->FindVertex(pedigreeId);
      }
    
    vtkIdType result = pedigrees->LookupValue(pedigreeId);
    if (result == -1)
      return -1;

    return helper->MakeDistributedId(myRank, result);
    }

  return pedigrees->LookupValue(pedigreeId);
}

//----------------------------------------------------------------------------
bool vtkGraph::CheckedShallowCopy(vtkGraph *g)
{
  if (!g)
    {
    return false;
    }
  bool valid = this->IsStructureValid(g);
  if (valid)
    {
    this->CopyInternal(g, false);
    }
  return valid;
}

//----------------------------------------------------------------------------
bool vtkGraph::CheckedDeepCopy(vtkGraph *g)
{
  if (!g)
    {
    return false;
    }
  bool valid = this->IsStructureValid(g);
  if (valid)
    {
    this->CopyInternal(g, true);
    }
  return valid;
}

//----------------------------------------------------------------------------
void vtkGraph::ShallowCopy(vtkDataObject *obj)
{
  vtkGraph *g = vtkGraph::SafeDownCast(obj);
  if (!g)
    {
    vtkErrorMacro("Can only shallow copy from vtkGraph subclass.");
    return;
    }
  bool valid = this->IsStructureValid(g);
  if (valid)
    {
    this->CopyInternal(g, false);
    }
  else
    {
    vtkErrorMacro("Invalid graph structure for this type of graph.");
    }
}

//----------------------------------------------------------------------------
void vtkGraph::DeepCopy(vtkDataObject *obj)
{
  vtkGraph *g = vtkGraph::SafeDownCast(obj);
  if (!g)
    {
    vtkErrorMacro("Can only shallow copy from vtkGraph subclass.");
    return;
    }
  bool valid = this->IsStructureValid(g);
  if (valid)
    {
    this->CopyInternal(g, true);
    }
  else
    {
    vtkErrorMacro("Invalid graph structure for this type of graph.");
    }
}

//----------------------------------------------------------------------------
void vtkGraph::CopyStructure(vtkGraph *g)
{
  // Copy on write.
  this->SetInternals(g->Internals);
  if (g->Points)
    {
    if (!this->Points)
      {
      this->Points = vtkPoints::New();
      }
    this->Points->ShallowCopy(g->Points);
    }
  else if (this->Points)
    {
    this->Points->Delete();
    this->Points = 0;
    }

  // Propagate information used by distributed graphs
  this->Information->Set
    (vtkDataObject::DATA_PIECE_NUMBER(),
     g->Information->Get(vtkDataObject::DATA_PIECE_NUMBER()));
  this->Information->Set
    (vtkDataObject::DATA_NUMBER_OF_PIECES(),
     g->Information->Get(vtkDataObject::DATA_NUMBER_OF_PIECES()));
}

//----------------------------------------------------------------------------
void vtkGraph::CopyInternal(vtkGraph *g, bool deep)
{
  if (deep)
    {
    vtkDataObject::DeepCopy(g);
    }
  else
    {
    vtkDataObject::ShallowCopy(g);
    }
  // Copy on write.
  this->SetInternals(g->Internals);
  if (deep)
    {
    this->EdgeData->DeepCopy(g->EdgeData);
    this->VertexData->DeepCopy(g->VertexData);
    }
  else
    {
    this->EdgeData->ShallowCopy(g->EdgeData);
    this->VertexData->ShallowCopy(g->VertexData);
    }
  if (g->Points)
    {
    if (!this->Points)
      {
      this->Points = vtkPoints::New();
      }
    if (deep)
      {
      this->Points->DeepCopy(g->Points);
      }
    else
      {
      this->Points->ShallowCopy(g->Points);
      }
    }
  else if (this->Points)
    {
    this->Points->Delete();
    this->Points = 0;
    }

  // Propagate information used by distributed graphs
  this->Information->Set
    (vtkDataObject::DATA_PIECE_NUMBER(),
     g->Information->Get(vtkDataObject::DATA_PIECE_NUMBER()));
  this->Information->Set
    (vtkDataObject::DATA_NUMBER_OF_PIECES(),
     g->Information->Get(vtkDataObject::DATA_NUMBER_OF_PIECES()));
}

//----------------------------------------------------------------------------
void vtkGraph::Squeeze()
{
  if ( this->Points )
    {
    this->Points->Squeeze();
    }
  this->EdgeData->Squeeze();
  this->VertexData->Squeeze();
}

//----------------------------------------------------------------------------
vtkGraph *vtkGraph::GetData(vtkInformation *info)
{
  return info? vtkGraph::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkGraph *vtkGraph::GetData(vtkInformationVector *v, int i)
{
  return vtkGraph::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkGraph::AddVertexInternal(vtkVariantArray *propertyArr,
                                 vtkIdType *vertex)
{
  // TODO: This isn't going to properly deal with remote vertex
  // addition in the presence of pedigree IDs. We need to look at the
  // pedigree ID inside propertyArr and use that to route the vertex
  // addition.
  this->ForceOwnership();
  this->Internals->Adjacency.push_back(vtkVertexAdjacencyList());

  if (propertyArr)      // Add vertex properties
    {
    vtkDataSetAttributes *vertexData = this->GetVertexData();
    int numProps = propertyArr->GetNumberOfValues();   // # of properties = # of arrays
    assert(numProps == vertexData->GetNumberOfArrays());
    for (int iprop=0; iprop<numProps; iprop++)
      {
      vtkAbstractArray* arr = vertexData->GetAbstractArray(iprop);
      if (vtkDataArray::SafeDownCast(arr))
        {
        vtkDataArray::SafeDownCast(arr)->InsertNextTuple1(propertyArr->GetPointer(iprop)->ToDouble());
        }
      else if (vtkStringArray::SafeDownCast(arr))
        {
        vtkStringArray::SafeDownCast(arr)->InsertNextValue(propertyArr->GetPointer(iprop)->ToString());
        }
      else
        {
        vtkErrorMacro("Unsupported array type");
        }
      }
    }
  
  if (vertex)
    {
    if (vtkDistributedGraphHelper *helper = this->GetDistributedGraphHelper())
      {
      *vertex = helper->MakeDistributedId
                  (this->Information->Get(vtkDataObject::DATA_PIECE_NUMBER()),
                   this->Internals->Adjacency.size() - 1);
      }
    else
      {
      *vertex = this->Internals->Adjacency.size() - 1;
      }
    }
}

//----------------------------------------------------------------------------
void vtkGraph::AddVertexInternal(const vtkVariant& pedigreeId, 
                                 vtkIdType *vertex)
{
  vtkDistributedGraphHelper *helper = this->GetDistributedGraphHelper();
  if (helper)
    {
    vtkIdType myRank 
      = this->Information->Get(vtkDataObject::DATA_PIECE_NUMBER());
    if (helper->GetVertexOwnerByPedigreeId(pedigreeId) != myRank) 
      {
      helper->AddVertexInternal(pedigreeId, vertex);
      return;
      }
    }

  vtkIdType existingVertex = this->FindVertex(pedigreeId);
  if (existingVertex != -1)
    {
    // We found this vertex; nothing more to do.
    if (vertex)
      {
      *vertex = existingVertex;
      }
    return ;
    }

  // Add the vertex locally
  this->ForceOwnership();
  vtkIdType v;
  this->AddVertexInternal(0, &v);
  if (vertex)
    {
    *vertex = v;
    }

  // Add the pedigree ID of the vertex
  vtkAbstractArray *abstract = this->GetVertexData()->GetPedigreeIds();
  if (abstract == NULL)
    {
    vtkErrorMacro("Added a vertex with a pedigree ID to a vtkGraph with no pedigree ID array");
    return;
    }
  vtkVariantArray *pedigrees = vtkVariantArray::SafeDownCast(abstract);
  if (pedigrees == NULL)
    {
    vtkErrorMacro("Pedigree ID array in a vtkGraph is not a vtkVariantArray.");
    return;
    }

  vtkIdType index = v;
  if (helper)
    {
    index = helper->GetVertexIndex(v);
    }

  pedigrees->InsertValue(index, pedigreeId);
}

//----------------------------------------------------------------------------
void vtkGraph::AddEdgeInternal(vtkIdType u, vtkIdType v, bool directed, 
                               vtkVariantArray *propertyArr, vtkEdgeType *edge)
{
  this->ForceOwnership();
  if (this->Internals->DistributedHelper)
    {
    this->Internals->DistributedHelper->AddEdgeInternal(u, v, directed, 
                                                        propertyArr, edge);
    return;
    }

  vtkIdType edgeId = this->Internals->NumberOfEdges;
  this->Internals->NumberOfEdges++;
  this->Internals->Adjacency[u].OutEdges.push_back(vtkOutEdgeType(v, edgeId));
  if (directed)
    {
    this->Internals->Adjacency[v].InEdges.push_back(vtkInEdgeType(u, edgeId));
    }
  else if (u != v)
    {
    // Avoid storing self-loops twice in undirected graphs.
    this->Internals->Adjacency[v].OutEdges.push_back(vtkOutEdgeType(u, edgeId));
    }
  if (edge)
    {
    *edge = vtkEdgeType(u, v, edgeId);
    }

  if (propertyArr)
    {
    // Insert edge properties
    vtkDataSetAttributes *edgeData = this->GetEdgeData();
    int numProps = propertyArr->GetNumberOfValues();
    assert(numProps == edgeData->GetNumberOfArrays());
    for (int iprop=0; iprop<numProps; iprop++)
      {
      vtkAbstractArray* array = edgeData->GetAbstractArray(iprop);
      if (vtkDataArray::SafeDownCast(array))
        {
        vtkDataArray::SafeDownCast(array)->InsertNextTuple1(propertyArr->GetPointer(iprop)->ToDouble());
        }
      else if (vtkStringArray::SafeDownCast(array))
        {
        vtkStringArray::SafeDownCast(array)->InsertNextValue(propertyArr->GetPointer(iprop)->ToString());
        }
      else
        {
        vtkErrorMacro("Unsupported array type");
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkGraph::AddEdgeInternal(const vtkVariant& uPedigreeId, vtkIdType v, 
                               bool directed, vtkVariantArray *propertyArr, 
                               vtkEdgeType *edge)
{
  this->ForceOwnership();
  if (this->Internals->DistributedHelper)
    {
    this->Internals->DistributedHelper->AddEdgeInternal(uPedigreeId, v, 
                                                        directed, propertyArr,
                                                        edge);
    return;
    }
  
  vtkIdType u;
  this->AddVertexInternal(uPedigreeId, &u);
  this->AddEdgeInternal(u, v, directed, propertyArr, edge);
}

//----------------------------------------------------------------------------
void vtkGraph::AddEdgeInternal(vtkIdType u, const vtkVariant& vPedigreeId, 
                               bool directed, vtkVariantArray *propertyArr, 
                               vtkEdgeType *edge)
{
  this->ForceOwnership();
  if (this->Internals->DistributedHelper)
    {
    this->Internals->DistributedHelper->AddEdgeInternal(u, vPedigreeId, 
                                                        directed, propertyArr,
                                                        edge);
    return;
    }
  
  vtkIdType v;
  this->AddVertexInternal(vPedigreeId, &v);
  this->AddEdgeInternal(u, v, directed, propertyArr, edge);
}

//----------------------------------------------------------------------------
void vtkGraph::AddEdgeInternal(const vtkVariant& uPedigreeId, 
                               const vtkVariant& vPedigreeId, 
                               bool directed, vtkVariantArray *propertyArr, 
                               vtkEdgeType *edge)
{
  this->ForceOwnership();
  if (this->Internals->DistributedHelper)
    {
    this->Internals->DistributedHelper->AddEdgeInternal(uPedigreeId, 
                                                        vPedigreeId, directed,
                                                        propertyArr, edge);
    return;
    }
  
  vtkIdType u, v;
  this->AddVertexInternal(uPedigreeId, &u);
  this->AddVertexInternal(vPedigreeId, &v);
  this->AddEdgeInternal(u, v, directed, propertyArr, edge);
}

//----------------------------------------------------------------------------
void vtkGraph::ReorderOutVertices(vtkIdType v, vtkIdTypeArray *vertices)
{
  vtkIdType index = v;
  if (vtkDistributedGraphHelper *helper = this->GetDistributedGraphHelper())
    {
    int myRank = this->Information->Get(vtkDataObject::DATA_PIECE_NUMBER());
    if (myRank != helper->GetVertexOwner(v))
      {
      vtkErrorMacro("vtkGraph cannot reorder the out vertices for a non-local vertex");
      return;
      }

    index = helper->GetVertexIndex(v);
    }

  this->ForceOwnership();
  vtksys_stl::vector<vtkOutEdgeType> outEdges;
  vtksys_stl::vector<vtkOutEdgeType>::iterator it, itEnd;
  itEnd = this->Internals->Adjacency[index].OutEdges.end();
  for (vtkIdType i = 0; i < vertices->GetNumberOfTuples(); ++i)
    {
    vtkIdType vert = vertices->GetValue(i);
    // Find the matching edge
    for (it = this->Internals->Adjacency[index].OutEdges.begin(); it != itEnd; ++it)
      {
      if (it->Target == vert)
        {
        outEdges.push_back(*it);
        break;
        }
      }
    }
  if (outEdges.size() != this->Internals->Adjacency[index].OutEdges.size())
    {
    vtkErrorMacro("Invalid reorder list.");
    return;
    }
  this->Internals->Adjacency[index].OutEdges = outEdges;
}

//----------------------------------------------------------------------------
bool vtkGraph::IsSameStructure(vtkGraph *other)
{
  return (this->Internals == other->Internals);
}

//----------------------------------------------------------------------------
vtkGraphInternals* vtkGraph::GetGraphInternals(bool modifying)
{
  if (modifying)
    {
    this->ForceOwnership();
    }
  return this->Internals;
}

//----------------------------------------------------------------------------
void vtkGraph::ForceOwnership()
{
  // If the reference count == 1, we own it and can change it.
  // If the reference count > 1, we must make a copy to avoid
  // changing the structure of other graphs.
  if (this->Internals->GetReferenceCount() > 1)
    {
    vtkGraphInternals *internals = vtkGraphInternals::New();
    internals->Adjacency = this->Internals->Adjacency;
    internals->NumberOfEdges = this->Internals->NumberOfEdges;
    this->SetInternals(internals);
    internals->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "VertexData: " << (this->VertexData ? "" : "(none)") << endl;
  if (this->VertexData)
    {
    this->VertexData->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "EdgeData: " << (this->EdgeData ? "" : "(none)") << endl;
  if (this->EdgeData)
    {
    this->EdgeData->PrintSelf(os, indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
// Supporting operators
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
bool operator==(vtkEdgeBase e1, vtkEdgeBase e2)
{
  return e1.Id == e2.Id;
}

//----------------------------------------------------------------------------
bool operator!=(vtkEdgeBase e1, vtkEdgeBase e2)
{
  return e1.Id != e2.Id;
}

//----------------------------------------------------------------------------
ostream& operator<<(ostream& out, vtkEdgeBase e)
{
  return out << e.Id;
}
