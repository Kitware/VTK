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
//
// .SEEALSO
// vtkGraph vtkDistributedGraphHelper

#ifndef __vtkPBGLDistributedGraphHelper_h
#define __vtkPBGLDistributedGraphHelper_h

#include "vtkDistributedGraphHelper.h"
#include <vtkstd/utility>

class vtkPBGLDistributedGraphHelperInternals;

//BTX
namespace boost { namespace graph { namespace distributed {
  class mpi_process_group;
} } } /// end namespace boost::graph::distributed
//ETX

class VTK_PARALLEL_EXPORT vtkPBGLDistributedGraphHelper : public vtkDistributedGraphHelper
{
 public:
  vtkTypeRevisionMacro (vtkPBGLDistributedGraphHelper, 
                        vtkDistributedGraphHelper);
  
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

  //BTX
  // Description:
  // Return the process group associated with this distributed graph.
  boost::graph::distributed::mpi_process_group GetProcessGroup();
  //ETX

  // Description:
  // The Parallel BGL-specific internal information for this distributed 
  // graph. TODO: Make this protected
  vtkPBGLDistributedGraphHelperInternals *Internals;

 protected:
  vtkPBGLDistributedGraphHelper();
  ~vtkPBGLDistributedGraphHelper();

  //BTX
  enum Tags
  {
    // Add a back edge; the forward edge has already been added.
    ADD_DIRECTED_BACK_EDGE_TAG,
    ADD_UNDIRECTED_BACK_EDGE_TAG,
    // Add an edge; don't reply.
    ADD_DIRECTED_EDGE_NO_REPLY_TAG,
    ADD_UNDIRECTED_EDGE_NO_REPLY_TAG,
    // Add an edge; return the edge descriptor.
    ADD_DIRECTED_EDGE_WITH_REPLY_TAG,
    ADD_UNDIRECTED_EDGE_WITH_REPLY_TAG
  };

  // Description:
  // Adds an edge (u, v) and returns the new edge. The graph edge may or may 
  // not be directed, depending on the given flag. If edge is non-null, it will
  // receive the newly-created edge.
  void AddEdgeInternal(vtkIdType u, vtkIdType v, bool directed, vtkEdgeType *edge);
  
  // Description:
  // Adds an edge (u, v), with properties, and returns the new edge. The graph edge may or may 
  // not be directed, depending on the given flag. If edge is non-null, it will
  // receive the newly-created edge.
  void AddEdgeInternal(vtkIdType u, vtkIdType v, bool directed, vtkEdgeType *edge, vtkVariantArray *variantValueArr);
 
  // Description:
  // Attach this distributed graph helper to the given graph. This will
  // be called as part of vtkGraph::SetDistributedGraphHelper.
  void AttachToGraph(vtkGraph *graph);

  // Description:
  // Handle a ADD_DIRECTED_BACK_EDGE_TAG or ADD_UNDIRECTED_BACK_END_TAG 
  // message.
  void HandleAddBackEdge(vtkEdgeType edge, bool directed);

  // Description: 
  // Handle an ADD_DIRECTED_EDGE_WITH_REPLY_TAG or 
  //  ADD_UNDIRECTED_EDGE_WITH_REPLY_TAG message
  vtkEdgeType 
  HandleAddEdge(const vtkstd::pair<vtkIdType, vtkIdType>& msg, bool directed);
  //ETX
};

#endif // __vtkPBGLDistributedGraphHelper_h
