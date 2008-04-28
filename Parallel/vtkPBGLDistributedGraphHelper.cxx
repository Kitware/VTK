/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLDistributedGraphHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* 
 * Copyright (C) 2008 The Trustees of Indiana University.
 * Use, modification and distribution is subject to the Boost Software
 * License, Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)
 */
#include "vtkPBGLDistributedGraphHelper.h"

#include "assert.h"
#include "vtkGraph.h"
#include "vtkGraphInternals.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPBGLGraphAdapter.h"
#include "vtkDataSetAttributes.h"
#include "vtkVariantArray.h"
#include "vtkDataArray.h";
#include "vtkStringArray.h";
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/bind.hpp>

//----------------------------------------------------------------------------
// private class vtkPBGLDistributedGraphHelperInternals
//----------------------------------------------------------------------------
class vtkPBGLDistributedGraphHelperInternals : public vtkObject
{
public:
  static vtkPBGLDistributedGraphHelperInternals *New();
  vtkTypeRevisionMacro(vtkPBGLDistributedGraphHelperInternals, vtkObject);
                                      
  // Description:
  // Process group used by this helper
  boost::graph::distributed::mpi_process_group process_group;
};

vtkStandardNewMacro(vtkPBGLDistributedGraphHelperInternals);
vtkCxxRevisionMacro(vtkPBGLDistributedGraphHelperInternals, "1.1.2.9");

//----------------------------------------------------------------------------
// class vtkPBGLDistributedGraphHelper
//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkPBGLDistributedGraphHelper, "1.1.2.9");
vtkStandardNewMacro(vtkPBGLDistributedGraphHelper);

//----------------------------------------------------------------------------
vtkPBGLDistributedGraphHelper::vtkPBGLDistributedGraphHelper() 
{
  this->Internals = vtkPBGLDistributedGraphHelperInternals::New();
}

//----------------------------------------------------------------------------
vtkPBGLDistributedGraphHelper::~vtkPBGLDistributedGraphHelper() 
{
  this->Internals->Delete();
}

//----------------------------------------------------------------------------
void vtkPBGLDistributedGraphHelper::Synchronize()
{
  synchronize(this->Internals->process_group);
}

//----------------------------------------------------------------------------
boost::graph::distributed::mpi_process_group vtkPBGLDistributedGraphHelper::GetProcessGroup()
{
  return this->Internals->process_group.base();
}

//----------------------------------------------------------------------------
void
vtkPBGLDistributedGraphHelper::AddEdgeInternal(vtkIdType u, 
                                               vtkIdType v,
                                               bool directed,
                                               vtkEdgeType *edge)
{
  int rank = this->Graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  int numProcs = this->Graph->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
  int uOwner = this->Graph->GetVertexOwner (u);

  if (uOwner == rank)
    {
    // The source of the edge is local.
    vtkGraphInternals* GraphInternals = this->Graph->GetGraphInternals(true);
    
    // The edge ID involves our rank and the local number of edges.
    vtkIdType edgeId 
      = this->Graph->MakeDistributedId(rank, GraphInternals->NumberOfEdges);

    // Add the forward edge.
    GraphInternals->Adjacency[this->Graph->GetVertexIndex(u)]
      .OutEdges.push_back(vtkOutEdgeType(v, edgeId));

    // We've added an edge.
    GraphInternals->NumberOfEdges++;

    int vOwner = this->Graph->GetVertexOwner(v);
    if (vOwner == rank)
      {
        // The target vertex is local. Add the appropriate back edge.
        if (directed)
          {
          GraphInternals->Adjacency[this->Graph->GetVertexIndex(v)]
            .InEdges.push_back(vtkInEdgeType(u, edgeId));
          }
        else if (u != v)
          {
          // Avoid storing self-loops twice in undirected graphs
          GraphInternals->Adjacency[this->Graph->GetVertexIndex(v)]
            .OutEdges.push_back(vtkOutEdgeType(u, edgeId));
          }
      }
    else
      {
      // The target vertex is remote: send a message asking its
      // owner to add the back edge.
      send(this->Internals->process_group, vOwner, 
           directed? ADD_DIRECTED_BACK_EDGE_TAG : ADD_UNDIRECTED_BACK_EDGE_TAG,
           vtkEdgeType(u, v, edgeId));
      }

    if (edge)
      {
      *edge = vtkEdgeType(u, v, edgeId);
      }
    }
  else 
    {
    // The source of the edge is non-local.
      if (edge)
        {
        // Send an AddEdge request to the owner of "u", and wait
        // patiently for the reply.
        send_oob_with_reply(this->Internals->process_group, uOwner, 
                            directed? ADD_DIRECTED_EDGE_WITH_REPLY_TAG 
                                    : ADD_UNDIRECTED_EDGE_WITH_REPLY_TAG,
                            vtkstd::pair<vtkIdType, vtkIdType>(u, v),
                            *edge);
        }
      else
        {
        // We're adding a remote edge, but we don't need to wait
        // until the edge has been added. Just send a message to the
        // owner of the source; we don't need (or want) a reply.
        send(this->Internals->process_group, uOwner, 
             directed? ADD_DIRECTED_EDGE_NO_REPLY_TAG 
                     : ADD_UNDIRECTED_EDGE_NO_REPLY_TAG,
             vtkstd::pair<vtkIdType, vtkIdType>(u, v));
        }
    }
}
//----------------------------------------------------------------------------
void
vtkPBGLDistributedGraphHelper::AddEdgeInternal(vtkIdType u, 
                                               vtkIdType v,
                                               bool directed,
                                               vtkEdgeType *edge,
                                               vtkVariantArray *propertyArr)
{
  int rank = this->Graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  int numProcs = this->Graph->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
  int uOwner = this->Graph->GetVertexOwner (u);

  if (uOwner == rank)
    {
    // The source of the edge is local.
    vtkGraphInternals* GraphInternals = this->Graph->GetGraphInternals(true);
    
    // The edge ID involves our rank and the local number of edges.
    vtkIdType edgeId 
      = this->Graph->MakeDistributedId(rank, GraphInternals->NumberOfEdges);
    
    if (propertyArr)      // Add edge properties
      {
      vtkDataSetAttributes *edgeData = this->Graph->GetEdgeData();
      int numProps = propertyArr->GetNumberOfValues();   // # of properties = # of arrays
      assert(numProps == edgeData->GetNumberOfArrays());
      for (int iprop=0; iprop<numProps; iprop++)
        {
        vtkAbstractArray* arr = edgeData->GetAbstractArray(iprop);
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

    // Add the forward edge.
    GraphInternals->Adjacency[this->Graph->GetVertexIndex(u)]
      .OutEdges.push_back(vtkOutEdgeType(v, edgeId));

    // We've added an edge.
    GraphInternals->NumberOfEdges++;

    int vOwner = this->Graph->GetVertexOwner(v);
    if (vOwner == rank)
      {
        // The target vertex is local. Add the appropriate back edge.
        if (directed)
          {
          GraphInternals->Adjacency[this->Graph->GetVertexIndex(v)]
            .InEdges.push_back(vtkInEdgeType(u, edgeId));
          }
        else if (u != v)
          {
          // Avoid storing self-loops twice in undirected graphs
          GraphInternals->Adjacency[this->Graph->GetVertexIndex(v)]
            .OutEdges.push_back(vtkOutEdgeType(u, edgeId));
          }
      }
    else
      {
      // The target vertex is remote: send a message asking its
      // owner to add the back edge.
      send(this->Internals->process_group, vOwner, 
           directed? ADD_DIRECTED_BACK_EDGE_TAG : ADD_UNDIRECTED_BACK_EDGE_TAG,
           vtkEdgeType(u, v, edgeId));
      }

    if (edge)
      {
      *edge = vtkEdgeType(u, v, edgeId);
      }
    }
  else 
    {
    // The source of the edge is non-local.
      if (edge)
        {
        // Send an AddEdge request to the owner of "u", and wait
        // patiently for the reply.
        send_oob_with_reply(this->Internals->process_group, uOwner, 
                            directed? ADD_DIRECTED_EDGE_WITH_REPLY_TAG 
                                    : ADD_UNDIRECTED_EDGE_WITH_REPLY_TAG,
                            vtkstd::pair<vtkIdType, vtkIdType>(u, v),
                            *edge);
        }
      else
        {
        // We're adding a remote edge, but we don't need to wait
        // until the edge has been added. Just send a message to the
        // owner of the source; we don't need (or want) a reply.
        send(this->Internals->process_group, uOwner, 
             directed? ADD_DIRECTED_EDGE_NO_REPLY_TAG 
                     : ADD_UNDIRECTED_EDGE_NO_REPLY_TAG,
             vtkstd::pair<vtkIdType, vtkIdType>(u, v));
        }
    }
}
//----------------------------------------------------------------------------
void vtkPBGLDistributedGraphHelper::AttachToGraph(vtkGraph *graph)
{
  if (graph && 
      (graph->GetNumberOfVertices() != 0 || graph->GetNumberOfEdges() != 0))
    {
    vtkErrorMacro("Cannot attach a distributed graph helper to a non-empty vtkGraph");  
    }

  this->Superclass::AttachToGraph(graph);

  if (this->Graph)
    {
    // Set the piece number and number of pieces so that the
    // vtkGraph knows the layout of the graph.
    this->Graph->GetInformation()->Set(
      vtkDataObject::DATA_PIECE_NUMBER(),
      process_id(this->Internals->process_group));
    this->Graph->GetInformation()->Set(
      vtkDataObject::DATA_NUMBER_OF_PIECES(),
      num_processes(this->Internals->process_group));

    // Add our triggers to the process group
    typedef vtkstd::pair<vtkIdType, vtkIdType> IdPair;
    this->Internals->process_group.make_distributed_object();
    this->Internals->process_group.trigger<vtkEdgeType>
      (ADD_DIRECTED_BACK_EDGE_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelper::HandleAddBackEdge, 
                   this, _3, true));
    this->Internals->process_group.trigger<vtkEdgeType>
      (ADD_UNDIRECTED_BACK_EDGE_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelper::HandleAddBackEdge, 
                   this, _3, false));
    this->Internals->process_group.trigger<IdPair>
      (ADD_DIRECTED_EDGE_NO_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelper::HandleAddEdge,
                   this, _3, true));
    this->Internals->process_group.trigger<IdPair>
      (ADD_UNDIRECTED_EDGE_NO_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelper::HandleAddEdge,
                   this, _3, false));
    this->Internals->process_group.trigger_with_reply<IdPair>
      (ADD_DIRECTED_EDGE_WITH_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelper::HandleAddEdge,
                   this, _3, true));
    this->Internals->process_group.trigger_with_reply<IdPair>
      (ADD_UNDIRECTED_EDGE_WITH_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelper::HandleAddEdge,
                   this, _3, false));
    }
}

//----------------------------------------------------------------------------
void 
vtkPBGLDistributedGraphHelper::HandleAddBackEdge
  (vtkEdgeType edge, bool directed)
{
  assert(edge.Source != edge.Target);
  assert(this->Graph->GetVertexOwner(edge.Target)
         == this->Graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER()));
  vtkGraphInternals *GraphInternals = this->Graph->GetGraphInternals(true);
  if (directed)
    {
    GraphInternals->Adjacency[this->Graph->GetVertexIndex(edge.Target)]
      .InEdges.push_back(vtkInEdgeType(edge.Source, edge.Id));
    }
  else 
    {
    GraphInternals->Adjacency[this->Graph->GetVertexIndex(edge.Target)]
      .OutEdges.push_back(vtkOutEdgeType(edge.Source, edge.Id));
    }
}

//----------------------------------------------------------------------------
vtkEdgeType 
vtkPBGLDistributedGraphHelper::HandleAddEdge
  (const vtkstd::pair<vtkIdType, vtkIdType>& msg, bool directed)
{
  vtkEdgeType result;
  AddEdgeInternal(msg.first, msg.second, directed, &result);
  return result;
}

//----------------------------------------------------------------------------
// Parallel BGL interface functions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
boost::graph::distributed::mpi_process_group
process_group(vtkGraph *graph)
{
  vtkDistributedGraphHelper *helper = graph->GetDistributedGraphHelper();
  if (!helper)
    {
    vtkErrorWithObjectMacro(graph, "A vtkGraph without a distributed graph helper is not a distributed graph");
    return boost::graph::distributed::mpi_process_group();
    }

  vtkPBGLDistributedGraphHelper *pbglHelper 
    = vtkPBGLDistributedGraphHelper::SafeDownCast(helper);
  if (!pbglHelper)
    {
    vtkErrorWithObjectMacro(graph, "A vtkGraph with a non-Parallel BGL distributed graph helper cannot be used with the Parallel BGL");
    return boost::graph::distributed::mpi_process_group();
    }

  return pbglHelper->Internals->process_group.base();
}
