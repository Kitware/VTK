/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistributedGraphHelper.h

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

// .NAME vtkDistributedGraphHelper - helper for the vtkGraph class that allows the graph to be distributed across multiple memory spaces
//
// .SECTION Description
// Including this header allows you to build distributed vtkGraphs

#ifndef __vtkDistributedGraphHelper_h
#define __vtkDistributedGraphHelper_h

#include "vtkObject.h"

class vtkDistributedGraphHelperInternals;
struct vtkEdgeType;
class vtkGraph;

class VTK_PARALLEL_EXPORT vtkDistributedGraphHelper : public vtkObject
{
 public:
  vtkTypeRevisionMacro (vtkDistributedGraphHelper, vtkObject);

  // Description:
  // Synchronizes all of the processors involved in this distributed
  // graph, so that all processors have a consistent view of the
  // distributed graph for the computation that follows. This routine
  // should be invoked after adding new edges into the distributed
  // graph, so that other processors will see those edges (or their
  // corresponding back-edges).
  virtual void Synchronize() = 0;

 protected:
  vtkDistributedGraphHelper();
  virtual ~vtkDistributedGraphHelper();

  // Description:
  // Add an edge (u, v) to the distributed graph. The edge may be directed 
  // undirected.
  virtual vtkEdgeType AddEdgeInternal(vtkIdType u, vtkIdType v, bool directed) = 0;

  // Description:
  // Attach this distributed graph helper to the given graph. This will
  // be called as part of vtkGraph::SetDistributedGraphHelper.
  virtual void AttachToGraph(vtkGraph *graph);

  // Description:
  // The graph to which this distributed graph helper is already attached.
  vtkGraph *Graph;

  friend class vtkGraph;
};

#endif // __vtkDistributedGraphHelper_h
