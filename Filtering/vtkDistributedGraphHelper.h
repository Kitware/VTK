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

// .NAME vtkDistributedGraphHelper - helper for the vtkGraph class
// that allows the graph to be distributed across multiple memory spaces.
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
// processors and the ability to map between "distributed" vertex and
// edge IDs and their component parts (processor and local index). For
// example, the Synchronize() method provides a barrier that allows
// all processors to catch up to the same point in the code before any
// processor can leave that Synchronize() call. For example, one would
// call Synchronize() after adding many edges to a distributed graph,
// so that all processors can handle the addition of inter-processor
// edges and continue, after the Synchronize() call, with a consistent
// view of the distributed graph. For more information about
// manipulating (distributed) graphs, see the vtkGraph documentation.
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
class vtkInformationIntegerKey;

// .NAME vtkVertexPedigreeIdDistributionFunction - The type of a
// function used to determine how to distribute vertex pedigree IDs
// across processors in a vtkGraph. The pedigree ID distribution
// function takes the pedigree ID of the vertex and a user-supplied
// void pointer and returns a hash value V. A vertex with that
// pedigree ID will reside on processor V % P, where P is the number
// of processors. This type is used in conjunction with the
// vtkDistributedGraphHelper class.
typedef vtkIdType (*vtkVertexPedigreeIdDistribution)
          (const vtkVariant& pedigreeId, void* userData);

class VTK_FILTERING_EXPORT vtkDistributedGraphHelper : public vtkObject
{
 public:
  vtkTypeMacro(vtkDistributedGraphHelper,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns owner of vertex v, by extracting top ceil(log2 P) bits of v.
  vtkIdType GetVertexOwner(vtkIdType v) const;

  // Description:
  // Returns local index of vertex v, by masking off top ceil(log2 P) bits of v.
  vtkIdType GetVertexIndex(vtkIdType v) const;

  // Description:
  // Returns owner of edge with ID e_id, by extracting top ceil(log2 P) bits of e_id.
  vtkIdType GetEdgeOwner(vtkIdType e_id) const;

  // Description:
  // Returns local index of edge with ID e_id, by masking off top ceil(log2 P)
  // bits of e_id.
  vtkIdType GetEdgeIndex(vtkIdType e_id) const;

  // Description:
  // Builds a distributed ID consisting of the given owner and the local ID.
  vtkIdType MakeDistributedId(int owner, vtkIdType local);

  //BTX
  // Description:
  // Set the pedigreeId -> processor distribution function that determines
  // how vertices are distributed when they are associated with
  // pedigree ID, which must be a unique label such as a URL or IP
  // address. If a NULL function pointer is provided, the default
  // hashed distribution will be used.
  void SetVertexPedigreeIdDistribution(vtkVertexPedigreeIdDistribution Func,
                                       void *userData);
  //ETX

  // Description:
  // Determine which processor owns the vertex with the given pedigree ID.
  vtkIdType GetVertexOwnerByPedigreeId(const vtkVariant& pedigreeId);

  // Description:
  // Synchronizes all of the processors involved in this distributed
  // graph, so that all processors have a consistent view of the
  // distributed graph for the computation that follows. This routine
  // should be invoked after adding new edges into the distributed
  // graph, so that other processors will see those edges (or their
  // corresponding back-edges).
  virtual void Synchronize() = 0;

  // Description:
  // Clones the distributed graph helper, returning another
  // distributed graph helper of the same kind that can be used in
  // another vtkGraph.
  virtual vtkDistributedGraphHelper *Clone() = 0;

  // Description:
  // Information Keys that distributed graphs can append to attribute arrays
  // to flag them as containing distributed IDs.  These can be used to let
  // routines that migrate vertices (either repartitioning or collecting graphs
  // to single nodes) to also modify the ids contained in the attribute arrays
  // to maintain consistency.
  static vtkInformationIntegerKey * DISTRIBUTEDVERTEXIDS();
  static vtkInformationIntegerKey * DISTRIBUTEDEDGEIDS();

 protected:
  vtkDistributedGraphHelper();
  virtual ~vtkDistributedGraphHelper();

  // Description:
  // Add a vertex, optionally with properties, to the distributed graph.
  // If vertex is non-NULL, it will be set
  // to the newly-added (or found) vertex. Note that if propertyArr is
  // non-NULL and the vertex data contains pedigree IDs, a vertex will
  // only be added if there is no vertex with that pedigree ID.
  virtual void AddVertexInternal(vtkVariantArray *propertyArr,
                         vtkIdType *vertex) = 0;

  // Description:
  // Add a vertex with the given pedigreeId to the distributed graph. If
  // vertex is non-NULL, it will receive the newly-created vertex.
  virtual void AddVertexInternal(const vtkVariant& pedigreeId, vtkIdType *vertex) = 0;

  // Description:
  // Add an edge (u, v) to the distributed graph. The edge may be directed
  // undirected. If edge is non-null, it will receive the newly-created edge.
  // If propertyArr is non-null, it specifies the properties that will be
  // attached to the newly-created edge.
  virtual void AddEdgeInternal(vtkIdType u, vtkIdType v, bool directed,
                               vtkVariantArray *propertyArr,
                               vtkEdgeType *edge) = 0;

  // Description:
  // Adds an edge (u, v) and returns the new edge. The graph edge may
  // or may not be directed, depending on the given flag. If edge is
  // non-null, it will receive the newly-created edge. uPedigreeId is
  // the pedigree ID of vertex u, which will be added if no vertex by
  // that pedigree ID exists. If propertyArr is non-null, it specifies
  // the properties that will be attached to the newly-created edge.
  virtual void AddEdgeInternal(const vtkVariant& uPedigreeId, vtkIdType v,
                               bool directed, vtkVariantArray *propertyArr,
                               vtkEdgeType *edge) = 0;

  // Description:
  // Adds an edge (u, v) and returns the new edge. The graph edge may
  // or may not be directed, depending on the given flag. If edge is
  // non-null, it will receive the newly-created edge. vPedigreeId is
  // the pedigree ID of vertex u, which will be added if no vertex
  // with that pedigree ID exists. If propertyArr is non-null, it specifies
  // the properties that will be attached to the newly-created edge.
  virtual void AddEdgeInternal(vtkIdType u, const vtkVariant& vPedigreeId,
                               bool directed, vtkVariantArray *propertyArr,
                               vtkEdgeType *edge) = 0;

  // Description:
  // Adds an edge (u, v) and returns the new edge. The graph edge may
  // or may not be directed, depending on the given flag. If edge is
  // non-null, it will receive the newly-created edge. uPedigreeId is
  // the pedigree ID of vertex u and vPedigreeId is the pedigree ID of
  // vertex u, each of which will be added if no vertex by that
  // pedigree ID exists. If propertyArr is non-null, it specifies
  // the properties that will be attached to the newly-created edge.
  virtual void AddEdgeInternal(const vtkVariant& uPedigreeId,
                               const vtkVariant& vPedigreeId,
                               bool directed, vtkVariantArray *propertyArr,
                               vtkEdgeType *edge) = 0;

  // Description:
  // Try to find the vertex with the given pedigree ID. Returns the
  // vertex ID if the vertex is found, or -1 if there is no vertex
  // with that pedigree ID.
  virtual vtkIdType FindVertex(const vtkVariant& pedigreeId) = 0;

  // Description:
  // Determine the source and target of the edge with the given
  // ID. Used internally by vtkGraph::GetSourceVertex and
  // vtkGraph::GetTargetVertex.
  virtual void
  FindEdgeSourceAndTarget(vtkIdType id,
                          vtkIdType *source, vtkIdType *target) = 0;

  // Description:
  // Attach this distributed graph helper to the given graph. This will
  // be called as part of vtkGraph::SetDistributedGraphHelper.
  virtual void AttachToGraph(vtkGraph *graph);

  // Description:
  // The graph to which this distributed graph helper is already attached.
  vtkGraph *Graph;

  //BTX
  // Description:
  // The distribution function used to map a pedigree ID to a processor.
  vtkVertexPedigreeIdDistribution VertexDistribution;

  // Description:
  // Extra, user-specified data to be passed into the distribution function.
  void *VertexDistributionUserData;

  // Description:
  // Bit mask to speed up decoding graph info {owner,index}
  vtkIdType signBitMask;

  // Description:
  // Bit mask to speed up decoding graph info {owner,index}
  vtkIdType highBitShiftMask;

  // Description:
  // Number of bits required to represent # of processors (owner)
  int procBits;

  // Description:
  // Number of bits required to represent {vertex,edge} index
  int indexBits;
  //ETX

 private:
  vtkDistributedGraphHelper(const vtkDistributedGraphHelper&); // Not implemented
  void operator=(const vtkDistributedGraphHelper&); // Not implemented

  //BTX
  friend class vtkGraph;
  //ETX
};

#endif // __vtkDistributedGraphHelper_h
