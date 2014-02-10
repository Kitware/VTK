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

#if !defined(VTK_LEGACY_REMOVE)

#include <cassert>
#include "vtkGraph.h"
#include "vtkGraphInternals.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPBGLGraphAdapter.h"
#include "vtkDataSetAttributes.h"
#include "vtkVariantArray.h"
#include "vtkVariantBoostSerialization.h"
#include "vtkDataArray.h"
#include "vtkStringArray.h"
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/bind.hpp>
#include <utility>


//----------------------------------------------------------------------------
// private class vtkPBGLDistributedGraphHelperMessageTypes
//----------------------------------------------------------------------------
class vtkPBGLDistributedGraphHelperMessageTypes
{
public:
  //----------------------------------------------------------------------------
  // Edge message bundle class for Id:Id edge types
  //----------------------------------------------------------------------------
  class EdgeIIMessageBundle
  {
  public:
    EdgeIIMessageBundle() { uDistributedId=0; vDistributedId=0; propertyArr=NULL; }
    EdgeIIMessageBundle(vtkIdType u, vtkIdType v, vtkVariantArray* prop=NULL)
         : uDistributedId(u), vDistributedId(v), propertyArr(prop)
    { };
    ~EdgeIIMessageBundle() {};

    vtkIdType uDistributedId;
    vtkIdType vDistributedId;
    vtkVariantArray * propertyArr;

  private:
    friend class boost::serialization::access;
    template<typename Archiver>
    void serialize(Archiver& ar, const unsigned int version)
    {
      ar & uDistributedId & vDistributedId & propertyArr;
    }
  };

  //----------------------------------------------------------------------------
  // Edge message bundle class for Id:Name edge types
  //----------------------------------------------------------------------------
  class EdgeINMessageBundle
  {
  public:
    EdgeINMessageBundle() { uDistributedId=0; propertyArr=NULL; }
    EdgeINMessageBundle(vtkIdType u, const vtkVariant& v, vtkVariantArray* prop=NULL)
         : uDistributedId(u), vPedigreeId(v), propertyArr(prop)
    { };
    ~EdgeINMessageBundle() {};

    vtkIdType  uDistributedId;
    vtkVariant vPedigreeId;
    vtkVariantArray * propertyArr;

  private:
    friend class boost::serialization::access;
    template<typename Archiver>
    void serialize(Archiver& ar, const unsigned int version)
    {
      ar & uDistributedId & vPedigreeId & propertyArr;
    }
  };

  //----------------------------------------------------------------------------
  // Edge message bundle class for Name:Id edge types
  //----------------------------------------------------------------------------
  class EdgeNIMessageBundle
  {
  public:
    EdgeNIMessageBundle() { vDistributedId=0; propertyArr=NULL; }
    EdgeNIMessageBundle(const vtkVariant& u, vtkIdType v, vtkVariantArray* prop=NULL)
       : uPedigreeId(u), vDistributedId(v), propertyArr(prop)
    { };
    ~EdgeNIMessageBundle() {};

    vtkVariant uPedigreeId;
    vtkIdType  vDistributedId;
    vtkVariantArray * propertyArr;

  private:
    friend class boost::serialization::access;
    template<typename Archiver>
    void serialize(Archiver& ar, const unsigned int version)
    {
      ar & uPedigreeId & vDistributedId & propertyArr;
    }
  };

  //----------------------------------------------------------------------------
  // Edge message bundle class for Name:Name edge types
  //----------------------------------------------------------------------------
  class EdgeNNMessageBundle
  {
  public:
    EdgeNNMessageBundle() { propertyArr=NULL; }
    EdgeNNMessageBundle(const vtkVariant& u, const vtkVariant& v, vtkVariantArray* prop=NULL)
         : uPedigreeId(u), vPedigreeId(v), propertyArr(prop)
    { };
    ~EdgeNNMessageBundle() {};

    vtkVariant uPedigreeId;
    vtkVariant vPedigreeId;
    vtkVariantArray * propertyArr;

  private:
    friend class boost::serialization::access;
    template<typename Archiver>
    void serialize(Archiver& ar, const unsigned int version)
    {
      ar & uPedigreeId & vPedigreeId & propertyArr;
    }
  };
};
typedef vtkPBGLDistributedGraphHelperMessageTypes::EdgeIIMessageBundle EdgeIIMessageBundle;
typedef vtkPBGLDistributedGraphHelperMessageTypes::EdgeINMessageBundle EdgeINMessageBundle;
typedef vtkPBGLDistributedGraphHelperMessageTypes::EdgeNIMessageBundle EdgeNIMessageBundle;
typedef vtkPBGLDistributedGraphHelperMessageTypes::EdgeNNMessageBundle EdgeNNMessageBundle;


//----------------------------------------------------------------------------
// private class vtkPBGLDistributedGraphHelperInternals
//----------------------------------------------------------------------------
class vtkPBGLDistributedGraphHelperInternals : public vtkObject
{
public:
  static vtkPBGLDistributedGraphHelperInternals *New();
  vtkTypeMacro(vtkPBGLDistributedGraphHelperInternals, vtkObject);

  // Description:
  // Handle a FIND_VERTEX_TAG message.
  vtkIdType HandleFindVertex(const vtkVariant& pedigreeId);

  // Description:
  // Handle a FIND_EDGE_SOURCE_TARGET_TAG message.
  std::pair<vtkIdType, vtkIdType> HandleFindEdgeSourceTarget(vtkIdType id);

  // Description:
  // Add a vertex with the given pedigree, if a vertex with that
  // pedigree ID does not already exist. Returns the ID for that
  // vertex.
  vtkIdType HandleAddVertex(const vtkVariant& pedigreeId);

  // Description:
  // Add a vertex with properties, if a vertex with the
  // pedigree ID (assuming there is one in the properties) does
  // not already exist.  Returns the ID for that vertex.
  vtkIdType HandleAddVertexProps(vtkVariantArray *propArray);

  // Description:
  // Handle a ADD_DIRECTED_BACK_EDGE_TAG or ADD_UNDIRECTED_BACK_END_TAG
  // message.
  void HandleAddBackEdge(vtkEdgeType edge, bool directed);

  // Description:
  // Handle ADD_*DIRECTED_EDGE_*_REPLY_TAG messages.
  vtkEdgeType
  HandleAddEdge(const EdgeIIMessageBundle& msg, bool directed);

  // Description:
  // Handle ADD_*DIRECTED_EDGE_NI_*_REPLY_TAG messages.
  vtkEdgeType
  HandleAddEdgeNI(const EdgeNIMessageBundle& msg, bool directed);

  // Description:
  // Handle ADD_*DIRECTED_EDGE_IN_*_REPLY_TAG messages
  vtkEdgeType
  HandleAddEdgeIN(const EdgeINMessageBundle& msg, bool directed);

  // Description:
  // Handle ADD_*DIRECTED_EDGE_NN_*_REPLY_TAG messages.
  vtkEdgeType
  HandleAddEdgeNN(const EdgeNNMessageBundle& msg, bool directed);



  // Description:
  // The helper class of which this structure is a part.
  vtkPBGLDistributedGraphHelper *Helper;

  // Description:
  // Process group used by this helper
  boost::graph::distributed::mpi_process_group process_group;

private:
  vtkPBGLDistributedGraphHelperInternals()
    : process_group(GetRootProcessGroup()) { }

  // Retrieve the root process group.
  static boost::graph::distributed::mpi_process_group&
  GetRootProcessGroup()
  {
    if (!root_process_group)
      {
      root_process_group = new boost::graph::distributed::mpi_process_group();
      }
    return *root_process_group;
  }

  // Description:
  // The "root" process group, to which all of the process groups in
  // VTK's distributed graphs will eventually attach.
  static boost::graph::distributed::mpi_process_group *root_process_group;
};


// Definition
boost::graph::distributed::mpi_process_group *
vtkPBGLDistributedGraphHelperInternals::root_process_group;

vtkStandardNewMacro(vtkPBGLDistributedGraphHelperInternals);

//----------------------------------------------------------------------------
// class vtkPBGLDistributedGraphHelper
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPBGLDistributedGraphHelper);

//----------------------------------------------------------------------------
vtkPBGLDistributedGraphHelper::vtkPBGLDistributedGraphHelper()
{
  this->Internals = vtkPBGLDistributedGraphHelperInternals::New();
  this->Internals->Helper = this;
  VTK_LEGACY_BODY(vtkPBGLDistributedGraphHelper::vtkPBGLDistributedGraphHelper, "VTK 6.2");
}

//----------------------------------------------------------------------------
vtkPBGLDistributedGraphHelper::~vtkPBGLDistributedGraphHelper()
{
  this->Internals->Delete();
}

//----------------------------------------------------------------------------
void vtkPBGLDistributedGraphHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "vtkPBGLDistributedGraphHelper" << endl;
}

//----------------------------------------------------------------------------
void vtkPBGLDistributedGraphHelper::Synchronize()
{
  synchronize(this->Internals->process_group);
}

//----------------------------------------------------------------------------
vtkDistributedGraphHelper *vtkPBGLDistributedGraphHelper::Clone()
{
  return vtkPBGLDistributedGraphHelper::New();
}

//----------------------------------------------------------------------------
boost::graph::distributed::mpi_process_group vtkPBGLDistributedGraphHelper::GetProcessGroup()
{
  return this->Internals->process_group.base();
}

//----------------------------------------------------------------------------
void
vtkPBGLDistributedGraphHelper::AddVertexInternal(vtkVariantArray *propertyArr,
                                                 vtkIdType *vertex)
{
  vtkIdType rank
    = this->Graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  vtkAbstractArray *peds = this->Graph->GetVertexData()->GetPedigreeIds();
  vtkIdType owner = -1;

  if (peds != NULL)
    {
    vtkIdType pedIdx = this->Graph->GetVertexData()->SetPedigreeIds(peds);
    vtkVariant pedigreeId = propertyArr->GetValue(pedIdx);
    owner = this->GetVertexOwnerByPedigreeId(pedigreeId);
    }

  if ((peds != NULL) && (owner == rank))
    {
    // This little dance keeps us from having to make
    // vtkPBGLDistributedGraphHelper a friend of vtkGraph. It also
    // makes sure that users don't try to be sneaky about adding
    // vertices to non-mutable vtkGraphs.
    if (vtkMutableDirectedGraph *graph
          = vtkMutableDirectedGraph::SafeDownCast(this->Graph))
      {
        if (vertex)
          {
          *vertex = graph->AddVertex(propertyArr);
          }
        else
          {
          graph->LazyAddVertex(propertyArr);
          }
      }
    else if (vtkMutableUndirectedGraph *graph
               = vtkMutableUndirectedGraph::SafeDownCast(this->Graph))
      {
        if (vertex)
          {
          *vertex = graph->AddVertex(propertyArr);
          }
        else
          {
          graph->LazyAddVertex(propertyArr);
          }
      }
    else
      {
      vtkErrorMacro("Cannot add vertices to a non-mutable, distributed graph");
      }
    return;
    }

  if (vertex)
    {
    // Request immediate addition of the vertex, with a reply.
    send_oob_with_reply(this->Internals->process_group, owner,
                        ADD_VERTEX_PROPS_WITH_REPLY_TAG, propertyArr, *vertex);
    }
  else
    {
    // Request addition of the vertex, eventually.
    send(this->Internals->process_group, owner, ADD_VERTEX_PROPS_NO_REPLY_TAG, propertyArr);
    }
}

//----------------------------------------------------------------------------
void
vtkPBGLDistributedGraphHelper::AddVertexInternal(const vtkVariant& pedigreeId,
                                                 vtkIdType *vertex)
{
  vtkIdType rank
    = this->Graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());

  vtkIdType owner = this->GetVertexOwnerByPedigreeId(pedigreeId);

  if (owner == rank)
    {
    // This little dance keeps us from having to make
    // vtkPBGLDistributedGraphHelper a friend of vtkGraph. It also
    // makes sure that users don't try to be sneaky about adding
    // vertices to non-mutable vtkGraphs.
    if (vtkMutableDirectedGraph *graph
          = vtkMutableDirectedGraph::SafeDownCast(this->Graph))
      {
        if (vertex)
          {
          *vertex = graph->AddVertex(pedigreeId);
          }
        else
          {
          graph->LazyAddVertex(pedigreeId);
          }
      }
    else if (vtkMutableUndirectedGraph *graph
               = vtkMutableUndirectedGraph::SafeDownCast(this->Graph))
      {
        if (vertex)
          {
          *vertex = graph->AddVertex(pedigreeId);
          }
        else
          {
          graph->LazyAddVertex(pedigreeId);
          }
      }
    else
      {
      vtkErrorMacro("Cannot add vertices to a non-mutable, distributed graph");
      }
    return;
    }

  if (vertex)
    {
    // Request immediate addition of the vertex, with a reply.
    send_oob_with_reply(this->Internals->process_group, owner,
                        ADD_VERTEX_WITH_REPLY_TAG, pedigreeId, *vertex);
    }
  else
    {
    // Request addition of the vertex, eventually.
    send(this->Internals->process_group, owner, ADD_VERTEX_NO_REPLY_TAG, pedigreeId);
    }
}

//----------------------------------------------------------------------------
void
vtkPBGLDistributedGraphHelper::AddEdgeInternal(vtkIdType uDistributedId,
                                               vtkIdType vDistributedId,
                                               bool directed,
                                               vtkVariantArray *propertyArr,
                                               vtkEdgeType *edge)
{
  int rank     = this->Graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  int numProcs = this->Graph->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
  int uOwner   = this->GetVertexOwner (uDistributedId);

#if defined(DEBUG)      // WCMCLEN
  cout << "[" << rank << "]\tAddEdgeInternal<uDist,vDist>("
       << "u={"<< this->GetVertexOwner(uDistributedId)
       << ":"  << this->GetVertexIndex(uDistributedId) << "}, "
       << "v={"<< this->GetVertexOwner(vDistributedId)
       << ":"  << this->GetVertexIndex(vDistributedId) << "}, "
       << "prop=" << propertyArr << ", "
       << "edge=" << edge
       << ")" << endl;
  if(propertyArr)
    {
    cout << "[" << rank << "]\t-\tpropertyArr: NumberOfValues="
         << propertyArr->GetNumberOfValues() << endl;
    cout << "[" << rank << "]\t-\tpropertyArr: NumberOfArrays="
         << this->Graph->GetEdgeData()->GetNumberOfArrays() << endl;
    }
  fflush(stdout);
#endif

  if (uOwner == rank)
    {
    // The source of the edge is local.
    vtkGraphInternals* GraphInternals = this->Graph->GetGraphInternals(true);

    // The edge ID involves our rank and the local number of edges.
    vtkIdType edgeId = this->MakeDistributedId(rank, GraphInternals->NumberOfEdges);

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
    GraphInternals->Adjacency[this->GetVertexIndex(uDistributedId)]
      .OutEdges.push_back(vtkOutEdgeType(vDistributedId, edgeId));

    // We've added an edge.
    GraphInternals->NumberOfEdges++;

    int vOwner = this->GetVertexOwner(vDistributedId);
    if (vOwner == rank)
      {
        // The target vertex is local. Add the appropriate back edge.
        if (directed)
          {
          GraphInternals->Adjacency[this->GetVertexIndex(vDistributedId)]
            .InEdges.push_back(vtkInEdgeType(uDistributedId, edgeId));
          }
        else if (uDistributedId != vDistributedId)
          {
          // Avoid storing self-loops twice in undirected graphs
          GraphInternals->Adjacency[this->GetVertexIndex(vDistributedId)]
            .OutEdges.push_back(vtkOutEdgeType(uDistributedId, edgeId));
          }
      }
    else
      {
      #if defined(DEBUG)      // WCMCLEN
      cout << "[" << rank << "]\tsend("
           << "{"<<this->GetVertexOwner(uDistributedId)<<":"<<this->GetVertexIndex(uDistributedId)<<"}, "
           << "{"<<this->GetVertexOwner(vDistributedId)<<":"<<this->GetVertexIndex(vDistributedId)<<"}"
           << ") to process " << vOwner
           << " ADD_*_BACK_EDGE_TAG"
           << endl;
      fflush(stdout);
      #endif

      // WCMCLEN: enabling this seems to cause problems sometimes?
      send_oob(this->Internals->process_group, vOwner,
           directed ? ADD_DIRECTED_BACK_EDGE_TAG
                    : ADD_UNDIRECTED_BACK_EDGE_TAG,
           vtkEdgeType(uDistributedId, vDistributedId, edgeId));
      }

    if (edge)
      {
      *edge = vtkEdgeType(uDistributedId, vDistributedId, edgeId);
      }
    }
  else  // uOwner != rank
    {
    // The source of the edge is non-local.
    if (edge)
      {
      // Send an AddEdge request to the owner of "u", and wait
      // patiently for the reply.
      #if defined(DEBUG)      // WCMCLEN
      cout << "[" << rank << "]\tsend_oob_with_reply("
           << "{" << this->GetVertexOwner(uDistributedId) <<":"<<this->GetVertexIndex(uDistributedId)<<"}, "
           << "{" << this->GetVertexOwner(vDistributedId) <<":"<<this->GetVertexIndex(vDistributedId)<<"}"
           << ") to process " << uOwner
           << " ADD_*_EDGE_WITH_REPLY"
           << endl;
      fflush(stdout);
      #endif
      send_oob_with_reply(this->Internals->process_group, uOwner,
                directed ? ADD_DIRECTED_EDGE_WITH_REPLY_TAG
                         : ADD_UNDIRECTED_EDGE_WITH_REPLY_TAG,
                EdgeIIMessageBundle(uDistributedId,vDistributedId,propertyArr),
                *edge);
      }
    else
      {
      // We're adding a remote edge, but we don't need to wait
      // until the edge has been added. Just send a message to the
      // owner of the source; we don't need (or want) a reply.
      #if defined(DEBUG)      // WCMCLEN
      cout << "[" << rank << "]\tsend("
           << "{"<<this->GetVertexOwner(uDistributedId) <<":"<<this->GetVertexIndex(uDistributedId)<<"}, "
           << "{"<<this->GetVertexOwner(vDistributedId) <<":"<<this->GetVertexIndex(vDistributedId)<<"}"
           << ") to process " << uOwner
           << " ADD_*_EDGE_NO_REPLY"
           << endl;
      fflush(stdout);
      #endif
      send(this->Internals->process_group, uOwner,
           directed? ADD_DIRECTED_EDGE_NO_REPLY_TAG
                   : ADD_UNDIRECTED_EDGE_NO_REPLY_TAG,
           EdgeIIMessageBundle(uDistributedId, vDistributedId, propertyArr));
      }
    }
}

//----------------------------------------------------------------------------
void
vtkPBGLDistributedGraphHelper::AddEdgeInternal(const vtkVariant& uPedigreeId,
                                               vtkIdType vDistributedId,
                                               bool directed,
                                               vtkVariantArray *propertyArr,
                                               vtkEdgeType *edge)
{
  vtkIdType rank   = this->Graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  vtkIdType uOwner = this->GetVertexOwnerByPedigreeId(uPedigreeId);

#if defined(DEBUG)      // WCMCLEN
  cout << "[" << rank << "]\tAddEdgeInternal<uPed, vDist>("
       << "u="<< uPedigreeId.ToString() << ", "
       << "v={"<< this->GetVertexOwner(vDistributedId)
       << ":"  << this->GetVertexIndex(vDistributedId) << "}, "
       << "prop=" << propertyArr << ", "
       << "edge=" << edge
       << ")" << endl;
#endif

  if (uOwner == rank)
    {
    // Resolve the pedigreeId for u immediately and add the edge locally.
    vtkIdType uDistributedId;
    this->AddVertexInternal(uPedigreeId, &uDistributedId);
    uDistributedId = this->MakeDistributedId(rank, uDistributedId);
    this->AddEdgeInternal(uDistributedId, vDistributedId, directed, propertyArr, edge);
    return;
    }

  // Edge is remote: request its addition.
  if (edge)
    {
    #if defined(DEBUG)      // WCMCLEN
    cout << "[" << rank << "]\tsend_oob_with_reply("
         << uPedigreeId.ToString()
         << "{"<<this->GetVertexOwner(vDistributedId) <<":"<<this->GetVertexIndex(vDistributedId)<<"}"
         << ") to process " << uOwner
         << " ADD_*_EDGE_NI_WITH_REPLY"
         << endl;
    fflush(stdout);
    #endif
    send_oob_with_reply(this->Internals->process_group, uOwner,
                        directed ? ADD_DIRECTED_EDGE_NI_WITH_REPLY_TAG
                                 : ADD_UNDIRECTED_EDGE_NI_WITH_REPLY_TAG,
                        EdgeNIMessageBundle(uPedigreeId,vDistributedId,propertyArr),
                        *edge);
    }
  else
    {
    #if defined(DEBUG)      // WCMCLEN
      cout << "[" << rank << "]\tsend("
           << uPedigreeId.ToString()
           << "{"<<this->GetVertexOwner(vDistributedId) <<":"<<this->GetVertexIndex(vDistributedId)<<"}"
           << ") to process " << uOwner
           << " ADD_*_EDGE_NI_NO_REPLY"
           << endl;
      fflush(stdout);
    #endif
    send(this->Internals->process_group, uOwner,
        directed ? ADD_DIRECTED_EDGE_NI_NO_REPLY_TAG
                 : ADD_UNDIRECTED_EDGE_NI_NO_REPLY_TAG,
        EdgeNIMessageBundle(uPedigreeId, vDistributedId, propertyArr));
    }
}

//----------------------------------------------------------------------------
void
vtkPBGLDistributedGraphHelper::AddEdgeInternal(vtkIdType uDistributedId,
                                               const vtkVariant& vPedigreeId,
                                               bool directed,
                                               vtkVariantArray *propertyArr,
                                               vtkEdgeType *edge)
{
  vtkIdType rank
    = this->Graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  vtkIdType vOwner = this->GetVertexOwnerByPedigreeId(vPedigreeId);

#if defined(DEBUG)      // WCMCLEN
  cout << "[" << rank << "]\tAddEdgeInternal<uDist,vPed>("
       << "u={"<< this->GetVertexOwner(uDistributedId)
       << ":"  << this->GetVertexIndex(uDistributedId) << "}, "
       << "v=" << vPedigreeId.ToString() <<  ", "
       << "prop=" << propertyArr << ", "
       << "edge=" << edge
       << ")" << endl;
  fflush(stdout);
#endif

  if (vOwner == rank || edge)
    {
    // Resolve the pedigree ID for v immediately and add the edge.
    vtkIdType vDistributedId;
    this->AddVertexInternal(vPedigreeId, &vDistributedId);
    vDistributedId=this->MakeDistributedId(vOwner,vDistributedId);  // pass vOwner, NOT rank
    this->AddEdgeInternal(uDistributedId, vDistributedId, directed, propertyArr, edge);
    return;
    }

  // v is remote and we don't care when the edge is added. Ask the
  // owner of v to resolve the pedigree ID of v and add the edge.
  #if defined(DEBUG)      // WCMCLEN
  cout << "[" << rank << "]\tsend("
       << "{"<<this->GetVertexOwner(uDistributedId) <<":"<<this->GetVertexIndex(uDistributedId)<<"}"
       << ", " << vPedigreeId.ToString() << ") to process " << vOwner
       << " ADD_*_EDGE_IN_NO_REPLY"
       << endl;
  fflush(stdout);
  #endif
  send(this->Internals->process_group, vOwner,
       directed ? ADD_DIRECTED_EDGE_IN_NO_REPLY_TAG
                : ADD_UNDIRECTED_EDGE_IN_NO_REPLY_TAG,
       EdgeINMessageBundle(uDistributedId,vPedigreeId,propertyArr));
}

//----------------------------------------------------------------------------
void
vtkPBGLDistributedGraphHelper::AddEdgeInternal(const vtkVariant& uPedigreeId,
                                               const vtkVariant& vPedigreeId,
                                               bool directed,
                                               vtkVariantArray *propertyArr,
                                               vtkEdgeType *edge)
{
  vtkIdType rank
    = this->Graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  vtkIdType uOwner = this->GetVertexOwnerByPedigreeId(uPedigreeId);

#if defined(DEBUG)      // WCMCLEN
  cout << "[" << rank << "]\tAddEdgeInternal<uPed,vPed>("
       << "u="<< uPedigreeId.ToString() << ", "
       << "uOwner="<< uOwner << ", "
       << "v="<< vPedigreeId.ToString() << ", "
       << "prop=" << propertyArr << ", "
       << "edge=" << edge
       << ")" << endl;
#endif

  if (uOwner == rank)
    {
    // Resolve the pedigree ID for u immediately and add the edge.
    vtkIdType u;
    this->AddVertexInternal(uPedigreeId, &u);
    vtkIdType uDistributedId = this->MakeDistributedId(rank,u);
#if defined(DEBUG)      // WCMCLEN
  cout << "[" << rank << " uPed,vPed]("
       << "u="<< u << ", "
       << ")" << endl;
#endif
    this->AddEdgeInternal(uDistributedId, vPedigreeId, directed, propertyArr, edge);
    return;
    }

  vtkIdType vOwner = this->GetVertexOwnerByPedigreeId(vPedigreeId);
  if (vOwner == rank || edge)
    {
    // Resolve the pedigree ID for v immediately and add the edge.
    vtkIdType vLocalIndex;
    this->AddVertexInternal(vPedigreeId, &vLocalIndex);
    vtkIdType vDistributedId = this->MakeDistributedId(vOwner,vLocalIndex); // pass vOwner, NOT rank
#if defined(DEBUG)      // WCMCLEN
  cout << "[" << rank << " uPed,vPed]("
       << ")" << endl;
#endif
    this->AddEdgeInternal(uPedigreeId, vDistributedId, directed, propertyArr, edge);
    return;
    }

  // Neither u nor v is local, and we don't care when the edge is
  // added, so ask the owner of v to resolve the pedigree ID of v and
  // add the edge.
  #if defined(DEBUG)      // WCMCLEN
  cout << "[" << rank << "]\tsend("
       << uPedigreeId.ToString() << ", "
       << vPedigreeId.ToString()
       << ") to process " << uOwner
       << " ADD_*_EDGE_NN_NO_REPLY"
       << endl;
  fflush(stdout);
  #endif
  send(this->Internals->process_group, vOwner,
       directed? ADD_DIRECTED_EDGE_NN_NO_REPLY_TAG
               : ADD_UNDIRECTED_EDGE_NN_NO_REPLY_TAG,
       EdgeNNMessageBundle(uPedigreeId, vPedigreeId, propertyArr));

}

//----------------------------------------------------------------------------
vtkIdType
vtkPBGLDistributedGraphHelper::FindVertex(const vtkVariant& pedigreeId)
{
  vtkIdType rank
    = this->Graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  vtkIdType owner = this->GetVertexOwnerByPedigreeId(pedigreeId);
  if (owner == rank)
    {
    // The vertex is local; just ask the local part of the graph.
    return this->Graph->FindVertex(pedigreeId);
    }

  // The vertex is remote; send a message looking for it.
  vtkIdType result;
  send_oob_with_reply(this->Internals->process_group, owner,
                      FIND_VERTEX_TAG, pedigreeId, result);
  return result;
}

//----------------------------------------------------------------------------
void
vtkPBGLDistributedGraphHelper::FindEdgeSourceAndTarget(vtkIdType id,
                                                       vtkIdType *source,
                                                       vtkIdType *target)
{
  vtkIdType rank
    = this->Graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  vtkIdType owner = this->GetEdgeOwner(id);

  if (owner == rank)
    {
    if (source)
      {
      *source = this->Graph->GetSourceVertex(id);
      }
    if (target)
      {
      *target = this->Graph->GetTargetVertex(id);
      }
    return;
    }

  std::pair<vtkIdType, vtkIdType> result;
  send_oob_with_reply(this->Internals->process_group, owner,
                      FIND_EDGE_SOURCE_TARGET_TAG, id, result);

  if (source)
    {
    *source = result.first;
    }
  if (target)
    {
    *target = result.second;
    }
}

//----------------------------------------------------------------------------
void vtkPBGLDistributedGraphHelper::AttachToGraph(vtkGraph *graph)
{
  this->Graph = graph;

  if (graph &&
      (graph->GetNumberOfVertices() != 0 || graph->GetNumberOfEdges() != 0))
    {
    vtkErrorMacro("Cannot attach a distributed graph helper to a non-empty vtkGraph");
    }

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
    this->Internals->process_group.make_distributed_object();
    this->Internals->process_group.trigger_with_reply<vtkVariant>
      (FIND_VERTEX_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals::HandleFindVertex,
                   this->Internals, _3));
    this->Internals->process_group.trigger_with_reply<vtkIdType>
      (FIND_EDGE_SOURCE_TARGET_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals
                     ::HandleFindEdgeSourceTarget,
                   this->Internals, _3));
    this->Internals->process_group.trigger<vtkVariant>
      (ADD_VERTEX_NO_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals::HandleAddVertex,
                   this->Internals, _3));
    this->Internals->process_group.trigger_with_reply<vtkVariant>
      (ADD_VERTEX_WITH_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals::HandleAddVertex,
                   this->Internals, _3));
    this->Internals->process_group.trigger<vtkVariantArray *>
      (ADD_VERTEX_PROPS_NO_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals::HandleAddVertexProps,
                   this->Internals, _3));
    this->Internals->process_group.trigger_with_reply<vtkVariantArray *>
      (ADD_VERTEX_PROPS_WITH_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals::HandleAddVertexProps,
                   this->Internals, _3));

    this->Internals->process_group.trigger<vtkEdgeType>
      (ADD_DIRECTED_BACK_EDGE_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals::HandleAddBackEdge,
                   this->Internals, _3, true));
    this->Internals->process_group.trigger<vtkEdgeType>
      (ADD_UNDIRECTED_BACK_EDGE_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals::HandleAddBackEdge,
                   this->Internals, _3, false));

    // Add edge for (id, id) pairs
    this->Internals->process_group.trigger<EdgeIIMessageBundle>
      (ADD_DIRECTED_EDGE_NO_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals::HandleAddEdge,
                   this->Internals, _3, true));
    this->Internals->process_group.trigger<EdgeIIMessageBundle>
      (ADD_UNDIRECTED_EDGE_NO_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals::HandleAddEdge,
                   this->Internals, _3, false));
    this->Internals->process_group.trigger_with_reply<EdgeIIMessageBundle>
      (ADD_DIRECTED_EDGE_WITH_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals::HandleAddEdge,
                   this->Internals, _3, true));
    this->Internals->process_group.trigger_with_reply<EdgeIIMessageBundle>
      (ADD_UNDIRECTED_EDGE_WITH_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals::HandleAddEdge,
                   this->Internals, _3, false));

    // Add edge for (pedigree, id) pairs
    this->Internals->process_group.trigger<EdgeNIMessageBundle>
      (ADD_DIRECTED_EDGE_NI_NO_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals::HandleAddEdgeNI,
                   this->Internals, _3, true));
    this->Internals->process_group.trigger<EdgeNIMessageBundle>
      (ADD_UNDIRECTED_EDGE_NI_NO_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals::HandleAddEdgeNI,
                   this->Internals, _3, false));
    this->Internals->process_group.trigger_with_reply<EdgeNIMessageBundle>
      (ADD_DIRECTED_EDGE_NI_WITH_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals::HandleAddEdgeNI,
                   this->Internals, _3, true));
    this->Internals->process_group.trigger_with_reply<EdgeNIMessageBundle>
      (ADD_UNDIRECTED_EDGE_NI_WITH_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals::HandleAddEdgeNI,
                   this->Internals, _3, false));

    // Add edge for (id, pedigree) pairs
    this->Internals->process_group.trigger<EdgeINMessageBundle>
      (ADD_DIRECTED_EDGE_IN_NO_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals::HandleAddEdgeIN,
                   this->Internals, _3, true));
    this->Internals->process_group.trigger<EdgeINMessageBundle>
      (ADD_UNDIRECTED_EDGE_IN_NO_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals::HandleAddEdgeIN,
                   this->Internals, _3, false));

    // Add edge for (pedigree, pedigree) pairs
    this->Internals->process_group.trigger<EdgeNNMessageBundle>
      (ADD_DIRECTED_EDGE_NN_NO_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals::HandleAddEdgeNN,
                   this->Internals, _3, true));
    this->Internals->process_group.trigger<EdgeNNMessageBundle>
      (ADD_UNDIRECTED_EDGE_NN_NO_REPLY_TAG,
       boost::bind(&vtkPBGLDistributedGraphHelperInternals::HandleAddEdgeNN,
                   this->Internals, _3, false));
    }

  // vtkDistributedGraphHelper will set up the appropriate masks.
  this->Superclass::AttachToGraph(graph);
}
//----------------------------------------------------------------------------
//-------------  vtkPBGLDistributedGraphHelperInternals methods --------------
//----------------------------------------------------------------------------
vtkIdType
vtkPBGLDistributedGraphHelperInternals::
HandleFindVertex(const vtkVariant& pedigreeId)
{
  return this->Helper->FindVertex(pedigreeId);
}

//----------------------------------------------------------------------------
std::pair<vtkIdType, vtkIdType>
vtkPBGLDistributedGraphHelperInternals::
HandleFindEdgeSourceTarget(vtkIdType id)
{
  std::pair<vtkIdType, vtkIdType> result;
  this->Helper->FindEdgeSourceAndTarget(id, &result.first, &result.second);
  return result;
}

//----------------------------------------------------------------------------
vtkIdType
vtkPBGLDistributedGraphHelperInternals::
HandleAddVertexProps(vtkVariantArray *propArr)
{
  vtkIdType result;
  this->Helper->AddVertexInternal(propArr, &result);
  return result;
}

//----------------------------------------------------------------------------
vtkIdType
vtkPBGLDistributedGraphHelperInternals::
HandleAddVertex(const vtkVariant& pedigreeId)
{
  vtkIdType result;
  this->Helper->AddVertexInternal(pedigreeId, &result);
  return result;
}

//----------------------------------------------------------------------------
void
vtkPBGLDistributedGraphHelperInternals::HandleAddBackEdge
  (vtkEdgeType edge, bool directed)
{
#if defined(DEBUG)      // WCMCLEN
  cout << "["<< process_group.rank << "]\tHandleAddBackEdge"
       << "(" << edge.Source << ", " << edge.Target << ", " << edge.Id << ")"
       << endl;
  fflush(stdout);
#endif

  assert(edge.Source != edge.Target);
  assert(this->Helper->GetVertexOwner(edge.Target)
         == this->Helper->Graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER()));
  vtkGraphInternals *GraphInternals = this->Helper->Graph->GetGraphInternals(true);
  if (directed)
    {
    GraphInternals->Adjacency[this->Helper->GetVertexIndex(edge.Target)]
      .InEdges.push_back(vtkInEdgeType(edge.Source, edge.Id));
    }
  else
    {
    GraphInternals->Adjacency[this->Helper->GetVertexIndex(edge.Target)]
      .OutEdges.push_back(vtkOutEdgeType(edge.Source, edge.Id));
    }
}

//----------------------------------------------------------------------------
vtkEdgeType
vtkPBGLDistributedGraphHelperInternals::HandleAddEdge
  (const EdgeIIMessageBundle& msg, bool directed)
{
#if defined(DEBUG)      // WCMCLEN
  vtkIdType uOwner = Helper->GetVertexOwner(msg.uDistributedId);
  vtkIdType uIndex = Helper->GetVertexIndex(msg.uDistributedId);
  vtkIdType vOwner = Helper->GetVertexOwner(msg.vDistributedId);
  vtkIdType vIndex = Helper->GetVertexIndex(msg.vDistributedId);
  cout << "["<< process_group.rank << "]\tHandleAddEdge"
       << "(u={"   << uOwner << ":" << uIndex << "}"
       << "}, v={" << vOwner << ":" << vIndex << "})"
       << endl;
  fflush(stdout);
#endif
  vtkEdgeType result;
  this->Helper->AddEdgeInternal(msg.uDistributedId, msg.vDistributedId, directed, msg.propertyArr, &result);
  return result;
}

//----------------------------------------------------------------------------
vtkEdgeType
vtkPBGLDistributedGraphHelperInternals::HandleAddEdgeNI
  (const EdgeNIMessageBundle& msg, bool directed)
{
#if defined(DEBUG)      // WCMCLEN
  vtkIdType vOwner = Helper->GetVertexOwner(msg.vDistributedId);
  vtkIdType vIndex = Helper->GetVertexIndex(msg.vDistributedId);
  cout << "["<< process_group.rank << "]\tHandleAddEdgeNI"
       << "( u="  << msg.uPedigreeId.ToString()
       << ", v={" << vOwner << ":" << vIndex << "})"
       << endl;
  fflush(stdout);
#endif
  vtkEdgeType result;
  this->Helper->AddEdgeInternal(msg.uPedigreeId, msg.vDistributedId, directed,
                                msg.propertyArr, &result);
  return result;
}

//----------------------------------------------------------------------------
vtkEdgeType
vtkPBGLDistributedGraphHelperInternals::HandleAddEdgeIN
  (const EdgeINMessageBundle& msg, bool directed)
{
#if defined(DEBUG)      // WCMCLEN
  vtkIdType uOwner = Helper->GetVertexOwner(msg.uDistributedId);
  vtkIdType uIndex = Helper->GetVertexIndex(msg.uDistributedId);
  cout << "["<< process_group.rank << "]\tHandleAddEdgeIN"
       << "(u={" << uOwner <<":"<< uIndex << "}"
       << ", "   << msg.vPedigreeId.ToString() << ")"
       << endl;
  fflush(stdout);
#endif
  vtkEdgeType result;
  this->Helper->AddEdgeInternal(msg.uDistributedId, msg.vPedigreeId, directed, msg.propertyArr, &result);
  return result;
}

//----------------------------------------------------------------------------
vtkEdgeType
vtkPBGLDistributedGraphHelperInternals::HandleAddEdgeNN
  (const EdgeNNMessageBundle& msg, bool directed)
{
#if defined(DEBUG)      // WCMCLEN
  cout << "["<< process_group.rank << "]\tHandleAddEdgeNN"
       << "("  << msg.uPedigreeId.ToString()
       << ", " << msg.vPedigreeId.ToString() << ")"
       << endl;
  fflush(stdout);
#endif
  vtkEdgeType result;
  this->Helper->AddEdgeInternal(msg.uPedigreeId, msg.vPedigreeId, directed,
                                msg.propertyArr, &result);
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

#endif //VTK_LEGACY_REMOVE
