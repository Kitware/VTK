/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLDistributedGraphHelper.h

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
// .NAME vtkPBGLDistributedGraphHelper - helper for the vtkGraph class that provides support for the Parallel Boost Graph Library
//
// .SECTION Description
// vtkPBGLDistributedGraphHelper is a helper class that allows a
// vtkGraph object to be distributed across several different
// processors using the Parallel Boost Graph Library (Parallel BGL, or
// PBGL). When attached to a vtkGraph instance,
// vtkPBGLDistributedGraphHelper provides the necessary communication
// support to build and manipulate distributed graphs. To enable the
// use of this class, you will need to configure VTK with the
// VTK_USE_PARALLEL_BGL option.
// @deprecated Not maintained as of VTK 6.2 and will be removed eventually.
//
// .SEEALSO
// vtkGraph vtkDistributedGraphHelper

#ifndef vtkPBGLDistributedGraphHelper_h
#define vtkPBGLDistributedGraphHelper_h

#include "vtkInfovisParallelModule.h" // For export macro
#include "vtkDistributedGraphHelper.h"

class vtkPBGLDistributedGraphHelperInternals;

//BTX
namespace boost { namespace graph { namespace distributed {
  class mpi_process_group;
} } } /// end namespace boost::graph::distributed
//ETX

#if !defined(VTK_LEGACY_REMOVE)
class VTKINFOVISPARALLEL_EXPORT vtkPBGLDistributedGraphHelper : public vtkDistributedGraphHelper
{
 public:
  vtkTypeMacro(vtkPBGLDistributedGraphHelper,vtkDistributedGraphHelper);

  // Description:
  // Creates an empty Parallel BGL distributed graph helper
  static vtkPBGLDistributedGraphHelper* New();

  // Description:
  // Synchronizes all of the processors involved in this distributed
  // graph, so that all processors have a consistent view of the
  // distributed graph for the computation that follows. This routine
  // should be invoked after adding new edges into the distributed
  // graph, so that other processors will see those edges (or their
  // corresponding back-edges).
  void Synchronize();

  // Description:
  // Clones this distributed graph helper.
  vtkDistributedGraphHelper *Clone();

  //BTX
  // Description:
  // Return the process group associated with this distributed graph.
  boost::graph::distributed::mpi_process_group GetProcessGroup();
  //ETX

  // Description:
  // The Parallel BGL-specific internal information for this distributed
  // graph. TODO: Make this protected
  vtkPBGLDistributedGraphHelperInternals *Internals;

  void PrintSelf(ostream& os, vtkIndent indent);

 protected:
  vtkPBGLDistributedGraphHelper();
  ~vtkPBGLDistributedGraphHelper();

  //BTX
  enum Tags
  {
    // Find a vertex by pedigree ID. This always has a reply.
    FIND_VERTEX_TAG,

    // Find the source and target by edge ID. This always has a reply.
    FIND_EDGE_SOURCE_TARGET_TAG,

    // Add a vertex with the given pedigree ID.
    ADD_VERTEX_NO_REPLY_TAG,
    ADD_VERTEX_WITH_REPLY_TAG,

    // Add a vertex with the given property array.
    ADD_VERTEX_PROPS_NO_REPLY_TAG,
    ADD_VERTEX_PROPS_WITH_REPLY_TAG,

    // Add a back edge; the forward edge has already been added.
    ADD_DIRECTED_BACK_EDGE_TAG,
    ADD_UNDIRECTED_BACK_EDGE_TAG,

    // Add an edge; don't reply.
    ADD_DIRECTED_EDGE_NO_REPLY_TAG,
    ADD_UNDIRECTED_EDGE_NO_REPLY_TAG,

    // Add an edge; return the edge ID.
    ADD_DIRECTED_EDGE_WITH_REPLY_TAG,
    ADD_UNDIRECTED_EDGE_WITH_REPLY_TAG,

    // Add an edge via (pedigree, id); don't reply.
    ADD_DIRECTED_EDGE_NI_NO_REPLY_TAG,
    ADD_UNDIRECTED_EDGE_NI_NO_REPLY_TAG,

    // Add an edge via (pedigree, id); return the edge ID.
    ADD_DIRECTED_EDGE_NI_WITH_REPLY_TAG,
    ADD_UNDIRECTED_EDGE_NI_WITH_REPLY_TAG,

    // Add an edge via (id, pedigree); don't reply.
    ADD_DIRECTED_EDGE_IN_NO_REPLY_TAG,
    ADD_UNDIRECTED_EDGE_IN_NO_REPLY_TAG,

    // Add an edge via (pedigree, pedigree); don't reply.
    ADD_DIRECTED_EDGE_NN_NO_REPLY_TAG,
    ADD_UNDIRECTED_EDGE_NN_NO_REPLY_TAG

  };

  // Description:
  // Add a vertex, optionally with properties, to the distributed graph.
  // If vertex is non-NULL, it will be set
  // to the newly-added (or found) vertex. Note that if propertyArr is
  // non-NULL and the vertex data contains pedigree IDs, a vertex will
  // only be added if there is no vertex with that pedigree ID.
  void AddVertexInternal(vtkVariantArray *propertyArr,
                         vtkIdType *vertex);

  // Description:
  // Add a vertex with the given pedigree ID to the distributed graph. If
  // vertex is non-NULL, it will receive the newly-created vertex.
  void AddVertexInternal(const vtkVariant& pedigreeId, vtkIdType *vertex);

  // Description:
  // Add an edge (u, v) to the distributed graph. The edge may be directed
  // undirected. If edge is non-null, it will receive the newly-created edge.
  // If propertyArr is non-null, it specifies the properties that will be
  // attached to the newly-created edge.
  void AddEdgeInternal(vtkIdType u, vtkIdType v, bool directed,
                       vtkVariantArray *propertyArr,
                       vtkEdgeType *edge);

  // Description:
  // Adds an edge (u, v) and returns the new edge. The graph edge may
  // or may not be directed, depending on the given flag. If edge is
  // non-null, it will receive the newly-created edge. uPedigreeId is
  // the pedigree ID of vertex u, which will be added if no vertex by
  // that pedigree ID exists. If propertyArr is non-null, it specifies
  // the properties that will be attached to the newly-created edge.
  void AddEdgeInternal(const vtkVariant& uPedigreeId, vtkIdType v,
                       bool directed, vtkVariantArray *propertyArr,
                       vtkEdgeType *edge);

  // Description:
  // Adds an edge (u, v) and returns the new edge. The graph edge may
  // or may not be directed, depending on the given flag. If edge is
  // non-null, it will receive the newly-created edge. vPedigreeId is
  // the pedigree ID of vertex u, which will be added if no vertex
  // with that pedigree ID exists. If propertyArr is non-null, it specifies
  // the properties that will be attached to the newly-created edge.
  void AddEdgeInternal(vtkIdType u, const vtkVariant& vPedigreeId,
                       bool directed, vtkVariantArray *propertyArr,
                       vtkEdgeType *edge);

  // Description:
  // Adds an edge (u, v) and returns the new edge. The graph edge may
  // or may not be directed, depending on the given flag. If edge is
  // non-null, it will receive the newly-created edge. uPedigreeId is
  // the pedigree ID of vertex u and vPedigreeId is the pedigree ID of
  // vertex u, each of which will be added if no vertex by that
  // pedigree ID exists. If propertyArr is non-null, it specifies
  // the properties that will be attached to the newly-created edge.
  void AddEdgeInternal(const vtkVariant& uPedigreeId,
                       const vtkVariant& vPedigreeId,
                       bool directed, vtkVariantArray *propertyArr,
                       vtkEdgeType *edge);
  // Description:
  // Try to find the vertex with the given pedigree ID. Returns true and
  // fills in the vertex ID if the vertex is found, and returns false
  // otherwise;
  vtkIdType FindVertex(const vtkVariant& pedigreeId);

  // Description:
  // Determine the source and target of the edge with the given
  // ID. Used internally by vtkGraph::GetSourceVertex and
  // vtkGraph::GetTargetVertex.
  void FindEdgeSourceAndTarget(vtkIdType id,
                               vtkIdType *source, vtkIdType *target);

  // Description:
  // Attach this distributed graph helper to the given graph. This will
  // be called as part of vtkGraph::SetDistributedGraphHelper.
  void AttachToGraph(vtkGraph *graph);
  //ETX

 private:
  vtkPBGLDistributedGraphHelper(const vtkPBGLDistributedGraphHelper&); // Not implemented
  void operator=(const vtkPBGLDistributedGraphHelper&); // Not implemented

  //BTX
  friend class vtkPBGLDistributedGraphHelperInternals;
  //ETX
};

#endif //VTK_LEGACY_REMOVE
#endif // vtkPBGLDistributedGraphHelper_h
