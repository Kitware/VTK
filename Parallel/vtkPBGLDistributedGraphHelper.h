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
// .NAME vtkPBGLDistributedGraphHelper - helper for the vtkGraph class that provides support for the Parallel Boost Graph Library (http://www.osl.iu.edu/research/pbgl)
//
// .SECTION Description
// Including this header allows you to build distributed vtkGraphs for use with the Parallel BGL.

#ifndef __vtkPBGLDistributedGraphHelper_h
#define __vtkPBGLDistributedGraphHelper_h

#include "vtkDistributedGraphHelper.h"

class vtkPBGLDistributedGraphHelperInternals;

class VTK_PARALLEL_EXPORT vtkPBGLDistributedGraphHelper : public vtkDistributedGraphHelper
{
 public:
  vtkTypeRevisionMacro (vtkPBGLDistributedGraphHelper, 
                        vtkDistributedGraphHelper);
  
  // Description:
  // Creates an empty Parallel BGL distributed graph helper
  static vtkPBGLDistributedGraphHelper* New();

  // Description:
  // Adds an edge (u, v) and returns the new edge. The graph edge may or may 
  // not be directed, depending on the given flag.
  vtkEdgeType AddEdgeInternal(vtkIdType u, vtkIdType v, bool directed);
 
  // Description:
  // The Parallel BGL-specific internal information for this distributed 
  // graph. TODO: Make this protected
  vtkPBGLDistributedGraphHelperInternals *Internals;

 protected:
  vtkPBGLDistributedGraphHelper();
  ~vtkPBGLDistributedGraphHelper();

  // BTX
  enum Tags
  {
    ADD_BACK_EDGE_TAG
  };

  // Description:
  // Attach this distributed graph helper to the given graph. This will
  // be called as part of vtkGraph::SetDistributedGraphHelper.
  void AttachToGraph(vtkGraph *graph);

  // Description:
  // Handle an incoming message for the distributed graph.
  void HandleMessage(int source, int tag);

  // Description:
  // Handle a ADD_BACK_EDGE_TAG message.
  void AddBackEdge(vtkEdgeType edge, bool directed);
  // ETX
};

#endif // __vtkPBGLDistributedGraphHelper_h
