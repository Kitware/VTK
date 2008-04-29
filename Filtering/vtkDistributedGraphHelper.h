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
// A distributed graph helper can be attached to an empty vtkGraph
// object to turn the vtkGraph into a distributed graph, whose
// vertices and edges are distributed across several different
// processors. vtkDistributedGraphHelper is an abstract class. Use a
// subclass of vtkDistributedGraphHelper, such as
// vtkPBGLDistributedGraphHelper, to build distributed graphs.
//
// The distributed graph helper provides facilities used by vtkGraph
// to communicate with other processors that store other parts of the
// same distributed graph. The only user-level functionality provided
// by vtkDistributedGraphHelper involves this communication among
// processors. For example, the Synchronize() method provides a
// barrier that allows all processors to catch up to the same point in
// the code before any processor can leave that Synchronize()
// call. For example, one would call Synchronize() after adding many
// edges to a distributed graph, so that all processors can handle the
// addition of inter-processor edges and continue, after the
// Synchronize() call, with a consistent view of the distributed
// graph. For more information about manipulating (distributed)
// graphs, see the vtkGraph documentation.
//
// .SECTION See Also
// vtkGraph
#ifndef __vtkDistributedGraphHelper_h
#define __vtkDistributedGraphHelper_h

#include "vtkObject.h"

class vtkDistributedGraphHelperInternals;
struct vtkEdgeType;
class vtkGraph;
class vtkVariant;
class vtkVariantArray;

// .NAME vtkVertexNameDistributionFunction - The type of a function
// used to determine how to distribute vertex names across processors
// in a vtkGraph. The name distribution function takes the name of the
// vertex and a user-supplied void pointer and returns a hash value
// V. A vertex with that name will reside on processor V % P, where P
// is the number of processors. This type is used in conjunction with
// the vtkDistributedGraphHelper class.
typedef vtkIdType (*vtkVertexNameDistribution)(const vtkVariant& name,
                                               void* userData);

class VTK_FILTERING_EXPORT vtkDistributedGraphHelper : public vtkObject
{
 public:
  vtkTypeRevisionMacro (vtkDistributedGraphHelper, vtkObject);

  // Description:
  // Set the name -> processor distribution function that determines
  // how vertices are distributed when they are associated with some
  // kind of name, e.g., a unique label such as a URL or IP
  // address. If a NULL function pointer is provided, the default
  // hashed distribution will be used.
  void SetVertexNameDistribution(vtkVertexNameDistribution Func, 
                                 void *userData);

  // Description:
  // Determine which processor owns the vertex with the given name.
  vtkIdType GetVertexOwnerByName(const vtkVariant& name);

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
  // Add a vertex with the given name to the distributed graph. If
  // vertex is non-NULL, it will receive the newly-created vertex.
  virtual void AddVertexInternal(const vtkVariant& name, vtkIdType *vertex) = 0;

  // Description:
  // Add an edge (u, v) to the distributed graph. The edge may be directed 
  // undirected. If edge is non-null, it will receive the newly-created edge.
  virtual void AddEdgeInternal(vtkIdType u, vtkIdType v, bool directed, vtkEdgeType *edge) = 0;

  // Description:
  // Adds an edge (u, v) and returns the new edge. The graph edge may or may 
  // not be directed, depending on the given flag. If edge is non-null, it will
  // receive the newly-created edge. uName is the name of vertex u, which will
  // be added if no vertex by that name exists.
  virtual void AddEdgeInternal(const vtkVariant& uName, vtkIdType v, 
                               bool directed, vtkEdgeType *edge) = 0;

  // Description:
  // Adds an edge (u, v) and returns the new edge. The graph edge may or may 
  // not be directed, depending on the given flag. If edge is non-null, it will
  // receive the newly-created edge. vName is the name of vertex u, which will
  // be added if no vertex by that name exists.
  virtual void AddEdgeInternal(vtkIdType u, const vtkVariant& vName, 
                               bool directed, vtkEdgeType *edge) = 0;

  // Description:
  // Adds an edge (u, v) and returns the new edge. The graph edge may or may 
  // not be directed, depending on the given flag. If edge is non-null, it will
  // receive the newly-created edge. uName is the name of vertex u and vName 
  // is the name of vertex u, each of which will be added if no vertex by that
  // name exists.
  virtual void AddEdgeInternal(const vtkVariant& uName, const vtkVariant& vName, 
                               bool directed, vtkEdgeType *edge) = 0;
  
  // Description:
  // Add an edge (u, v), with properties, to the distributed graph. The edge may be directed 
  // undirected. If edge is non-null, it will receive the newly-created edge.
  virtual void AddEdgeInternal(vtkIdType u, vtkIdType v, bool directed, vtkEdgeType *edge, vtkVariantArray *variantValueArr) = 0;

  // Description:
  // Try to find the vertex with the given name. Returns true and
  // fills in the vertex ID if the vertex is found, and returns false
  // otherwise;
  virtual bool FindVertex(const vtkVariant& name, vtkIdType *vertex) = 0;

  // Description:
  // Attach this distributed graph helper to the given graph. This will
  // be called as part of vtkGraph::SetDistributedGraphHelper.
  virtual void AttachToGraph(vtkGraph *graph);

  // Description:
  // The graph to which this distributed graph helper is already attached.
  vtkGraph *Graph;

  //BTX
  vtkVertexNameDistribution VertexDistribution;
  void *VertexDistributionUserData;
  //ETX

 private:
  vtkDistributedGraphHelper(const vtkDistributedGraphHelper&); // Not implemented
  void operator=(const vtkDistributedGraphHelper&); // Not implemented

  //BTX
  friend class vtkGraph;
  //ETX
};

#endif // __vtkDistributedGraphHelper_h
