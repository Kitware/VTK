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
#include <boost/parallel/mpi/bsp_process_group.hpp>
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
  boost::parallel::mpi::bsp_process_group process_group;
};

vtkStandardNewMacro(vtkPBGLDistributedGraphHelperInternals);
vtkCxxRevisionMacro(vtkPBGLDistributedGraphHelperInternals, "1.1.2.2");

//----------------------------------------------------------------------------
// class vtkPBGLDistributedGraphHelper
//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkPBGLDistributedGraphHelper, "1.1.2.2");
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
vtkEdgeType 
vtkPBGLDistributedGraphHelper::AddEdgeInternal(vtkIdType u, 
                                               vtkIdType v,
                                               bool directed)
{
  int rank = this->Graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  int numProcs = this->Graph->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
  int uOwner = this->Graph->GetVertexOwner (u);

  // Borrowed from GetVertexOwner -- TODO: needs to use integer arithmetic!
  int idNumBits = sizeof(vtkIdType) << 3;  // numBytes * 8
  int procBits = ceil(log2(numProcs));
  
  if (uOwner == rank)
    {
    // Forward part of the edge is local.
    vtkGraphInternals* GraphInternals = this->Graph->GetGraphInternals(true);
    
    // The edge ID involves our rank and the local number of edges.
    vtkIdType edgeId 
      = (rank << (idNumBits - procBits)) | GraphInternals->NumberOfEdges;

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
      send(this->Internals->process_group, vOwner, ADD_BACK_EDGE_TAG,
           vtkstd::pair<vtkEdgeType, bool>(vtkEdgeType(u, v, edgeId), 
                                           directed));
      }

    return vtkEdgeType(u, v, edgeId);
    }
  else
    {
    // Forward part of the edge is non-local; we abort at this point,
    // because we're not yet ready to deal with non-local edge
    // additions.
    vtkErrorMacro("Parallel BGL distributed graph cannot yet support adding non-local edges");

    // This is a dummy return value.
    return vtkEdgeType(u, v, -1);
    }
}

//----------------------------------------------------------------------------
void vtkPBGLDistributedGraphHelper::AttachToGraph(vtkGraph *graph)
{
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

    // Put our message handler into the process group
    this->Internals->process_group.replace_handler(
      boost::bind(&vtkPBGLDistributedGraphHelper::HandleMessage, this, _1, _2));
    }
}

void vtkPBGLDistributedGraphHelper::HandleMessage(int source, int tag)
{
  switch (tag)
    {
    case ADD_BACK_EDGE_TAG:
      {
        // Receive the incoming message
        vtkstd::pair<vtkEdgeType, bool> data;
        receive(this->Internals->process_group, source, tag, data);
        this->AddBackEdge(data.first, data.second);
      }
      break;
    }
}

void vtkPBGLDistributedGraphHelper::AddBackEdge(vtkEdgeType edge, bool directed)
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
// Parallel BGL interface functions
//----------------------------------------------------------------------------
namespace boost { namespace parallel {

  //----------------------------------------------------------------------------
  process_group_type<vtkGraph *>::type
  process_group(vtkGraph *graph)
  {
    vtkDistributedGraphHelper *helper = graph->GetDistributedGraphHelper();
    if (helper)
      {
      vtkErrorWithObjectMacro(graph, "A vtkGraph without a distributed graph helper is not a distributed graph");
      return boost::parallel::mpi::bsp_process_group();
      }

    vtkPBGLDistributedGraphHelper *pbglHelper 
      = vtkPBGLDistributedGraphHelper::SafeDownCast(helper);
    if (!pbglHelper)
      {
      vtkErrorWithObjectMacro(graph, "A vtkGraph with a non-Parallel BGL distributed graph helper cannot be used with the Parallel BGL");
      return boost::parallel::mpi::bsp_process_group();
      }

    return pbglHelper->Internals->process_group;
  }
} } // end namespace boost::parallel
