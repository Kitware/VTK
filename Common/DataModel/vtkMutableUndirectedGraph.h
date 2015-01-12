/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMutableUndirectedGraph.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkMutableUndirectedGraph - An editable undirected graph.
//
// .SECTION Description
// vtkMutableUndirectedGraph is an undirected graph with additional functions
// for adding vertices and edges. ShallowCopy(), DeepCopy(), CheckedShallowCopy(),
// and CheckedDeepCopy() will succeed when the argument is a vtkUndirectedGraph
// or vtkMutableUndirectedGraph.
//
// .SECTION See Also
// vtkUndirectedGraph vtkGraph

#ifndef vtkMutableUndirectedGraph_h
#define vtkMutableUndirectedGraph_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkUndirectedGraph.h"

class vtkEdgeListIterator;
class vtkGraphEdge;

class VTKCOMMONDATAMODEL_EXPORT vtkMutableUndirectedGraph : public vtkUndirectedGraph
{
public:
  static vtkMutableUndirectedGraph *New();
  vtkTypeMacro(vtkMutableUndirectedGraph, vtkUndirectedGraph);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allocates space for the specified number of vertices in the graph's
  // internal data structures.
  // The previous number of vertices is returned on success and -1
  // is returned on failure.
  //
  // This has no effect on the number of vertex coordinate tuples or
  // vertex attribute tuples allocated; you are responsible for
  // guaranteeing these match.
  // Also, this call is not implemented for distributed-memory graphs since
  // the semantics are unclear; calling this function on a graph with a
  // non-NULL DistributedGraphHelper will generate an error message,
  // no allocation will be performed, and a value of -1 will be returned.
  virtual vtkIdType SetNumberOfVertices( vtkIdType numVerts );

  // Description:
  // Adds a vertex to the graph and returns the index of the new vertex.
  //
  // \note In a distributed graph (i.e. a graph whose DistributedHelper
  // is non-null), this routine cannot be used to add a vertex
  // if the vertices in the graph have pedigree IDs, because this routine
  // will always add the vertex locally, which may conflict with the
  // proper location of the vertex based on the distribution of the
  // pedigree IDs.
  vtkIdType AddVertex();

  // Description:
  // Adds a vertex to the graph with associated properties defined in
  // \p propertyArr and returns the index of the new vertex.
  // The number and order of values in \p propertyArr must match up with the
  // arrays in the vertex data retrieved by GetVertexData().
  //
  // If a vertex with the given pedigree ID already exists, its properties will be
  // overwritten with the properties in \p propertyArr and the existing
  // vertex index will be returned.
  //
  // \note In a distributed graph (i.e. a graph whose DistributedHelper
  // is non-null) the vertex added or found might not be local. In this case,
  // AddVertex will wait until the vertex can be added or found
  // remotely, so that the proper vertex index can be returned. If you
  // don't actually need to use the vertex index, consider calling
  // LazyAddVertex, which provides better performance by eliminating
  // the delays associated with returning the vertex index.
  vtkIdType AddVertex(vtkVariantArray *propertyArr);

  // Description:
  // Adds a vertex with the given \p pedigreeID to the graph and
  // returns the index of the new vertex.
  //
  // If a vertex with the given pedigree ID already exists,
  // the existing vertex index will be returned.
  //
  // \note In a distributed graph (i.e. a graph whose DistributedHelper
  // is non-null) the vertex added or found might not be local. In this case,
  // AddVertex will wait until the vertex can be added or found
  // remotely, so that the proper vertex index can be returned. If you
  // don't actually need to use the vertex index, consider calling
  // LazyAddVertex, which provides better performance by eliminating
  // the delays associated with returning the vertex index.
  vtkIdType AddVertex(const vtkVariant& pedigreeId);

  //BTX
  // Description:
  // Adds an undirected edge from \p u to \p v,
  // where \p u and \p v are vertex indices,
  // and returns a \p vtkEdgeType structure describing that edge.
  //
  // \p vtkEdgeType contains fields for \p Source vertex index,
  // \p Target vertex index, and edge index \p Id.
  vtkEdgeType AddEdge(vtkIdType u, vtkIdType v);

  // Description:
  // Adds an undirected edge from \p u to \p v,
  // where \p u and \p v are vertex indices,
  // with associated properties defined in \p propertyArr
  // and returns a \p vtkEdgeType structure describing that edge.
  //
  // The number and order of values in \p propertyArr must match up with the
  // arrays in the edge data retrieved by GetEdgeData().
  //
  // \p vtkEdgeType contains fields for \p Source vertex index,
  // \p Target vertex index, and edge index \p Id.
  vtkEdgeType AddEdge(vtkIdType u, vtkIdType v,
                      vtkVariantArray *propertyArr);

  // Description:
  // Adds an undirected edge from \p u to \p v,
  // where \p u is a vertex pedigree ID and \p v is a vertex index,
  // and returns a \p vtkEdgeType structure describing that edge.
  //
  // The number and order of values in the optional parameter
  // \p propertyArr must match up with the arrays in the edge data
  // retrieved by GetEdgeData().
  //
  // \p vtkEdgeType contains fields for \p Source vertex index,
  // \p Target vertex index, and edge index \p Id.
  vtkEdgeType AddEdge(const vtkVariant& u, vtkIdType v,
                      vtkVariantArray *propertyArr = 0);

  // Description:
  // Adds a directed edge from \p u to \p v,
  // where \p u is a vertex index and \p v is a vertex pedigree ID,
  // and returns a \p vtkEdgeType structure describing that edge.
  //
  // The number and order of values in the optional parameter
  // \p propertyArr must match up with the arrays in the edge data
  // retrieved by GetEdgeData().
  //
  // \p vtkEdgeType contains fields for \p Source vertex index,
  // \p Target vertex index, and edge index \p Id.
  vtkEdgeType AddEdge(vtkIdType u, const vtkVariant& v,
                      vtkVariantArray *propertyArr = 0);

  // Description:
  // Adds a directed edge from \p u to \p v,
  // where \p u and \p v are vertex pedigree IDs,
  // and returns a \p vtkEdgeType structure describing that edge.
  //
  // The number and order of values in the optional parameter
  // \p propertyArr must match up with the arrays in the edge data
  // retrieved by GetEdgeData().
  //
  // \p vtkEdgeType contains fields for \p Source vertex index,
  // \p Target vertex index, and edge index \p Id.
  vtkEdgeType AddEdge(const vtkVariant& u,
                      const vtkVariant& v,
                      vtkVariantArray *propertyArr = 0);

  //ETX

  // Description:
  // Adds a vertex to the graph.
  //
  // This method is lazily evaluated for distributed graphs (i.e. graphs
  // whose DistributedHelper is non-null) the next time Synchronize is
  // called on the helper.
  void LazyAddVertex();

  // Description:
  // Adds a vertex to the graph with associated properties defined in
  // \p propertyArr.
  // The number and order of values in \p propertyArr must match up with the
  // arrays in the vertex data retrieved by GetVertexData().
  //
  // If a vertex with the given pedigree ID already exists, its properties will be
  // overwritten with the properties in \p propertyArr.
  //
  // This method is lazily evaluated for distributed graphs (i.e. graphs
  // whose DistributedHelper is non-null) the next time Synchronize is
  // called on the helper.
  void LazyAddVertex(vtkVariantArray *propertyArr);

  // Description:
  // Adds a vertex with the given \p pedigreeID to the graph.
  //
  // If a vertex with the given pedigree ID already exists,
  // no operation is performed.
  //
  // This method is lazily evaluated for distributed graphs (i.e. graphs
  // whose DistributedHelper is non-null) the next time Synchronize is
  // called on the helper.
  void LazyAddVertex(const vtkVariant& pedigreeId);

  // Description:
  // Adds an undirected edge from \p u to \p v,
  // where \p u and \p v are vertex indices.
  //
  // This method is lazily evaluated for distributed graphs (i.e. graphs
  // whose DistributedHelper is non-null) the next time Synchronize is
  // called on the helper.
  void LazyAddEdge(vtkIdType u, vtkIdType v);

  // Description:
  // Adds an undirected edge from \p u to \p v,
  // where \p u and \p v are vertex indices.
  //
  // The number and order of values in
  // \p propertyArr must match up with the arrays in the edge data
  // retrieved by GetEdgeData().
  //
  // This method is lazily evaluated for distributed graphs (i.e. graphs
  // whose DistributedHelper is non-null) the next time Synchronize is
  // called on the helper.
  void LazyAddEdge(vtkIdType u, vtkIdType v, vtkVariantArray *propertyArr);

  // Description:
  // Adds an undirected edge from \p u to \p v,
  // where \p u is a vertex pedigree ID and \p v is a vertex index.
  //
  // The number and order of values in the optional parameter
  // \p propertyArr must match up with the arrays in the edge data
  // retrieved by GetEdgeData().
  //
  // This method is lazily evaluated for distributed graphs (i.e. graphs
  // whose DistributedHelper is non-null) the next time Synchronize is
  // called on the helper.
  void LazyAddEdge(const vtkVariant& u, vtkIdType v,
                   vtkVariantArray *propertyArr = 0);

  // Description:
  // Adds an undirected edge from \p u to \p v,
  // where \p u is a vertex index and \p v is a vertex pedigree ID.
  //
  // The number and order of values in the optional parameter
  // \p propertyArr must match up with the arrays in the edge data
  // retrieved by GetEdgeData().
  //
  // This method is lazily evaluated for distributed graphs (i.e. graphs
  // whose DistributedHelper is non-null) the next time Synchronize is
  // called on the helper.
  void LazyAddEdge(vtkIdType u, const vtkVariant& v,
                   vtkVariantArray *propertyArr = 0);

  // Description:
  // Adds an undirected edge from \p u to \p v,
  // where \p u and \p v are vertex pedigree IDs.
  //
  // The number and order of values in the optional parameter
  // \p propertyArr must match up with the arrays in the edge data
  // retrieved by GetEdgeData().
  //
  // This method is lazily evaluated for distributed graphs (i.e. graphs
  // whose DistributedHelper is non-null) the next time Synchronize is
  // called on the helper.
  void LazyAddEdge(const vtkVariant& u,
                   const vtkVariant& v,
                   vtkVariantArray *propertyArr = 0);

  // Description:
  // Variant of AddEdge() that returns a heavyweight \p vtkGraphEdge object.
  // The graph owns the reference of the edge and will replace
  // its contents on the next call to AddGraphEdge().
  //
  // \note This is a less efficient method for use with wrappers.
  // In C++ you should use the faster AddEdge().
  vtkGraphEdge *AddGraphEdge(vtkIdType u, vtkIdType v);

  // Description:
  // Removes the vertex from the graph along with any connected edges.
  // Note: This invalidates the last vertex index, which is reassigned to v.
  void RemoveVertex(vtkIdType v);

  // Description:
  // Removes the edge from the graph.
  // Note: This invalidates the last edge index, which is reassigned to e.
  void RemoveEdge(vtkIdType e);

  // Description:
  // Removes a collection of vertices from the graph along with any connected edges.
  void RemoveVertices(vtkIdTypeArray* arr);

  // Description:
  // Removes a collection of edges from the graph.
  void RemoveEdges(vtkIdTypeArray* arr);

protected:
  vtkMutableUndirectedGraph();
  ~vtkMutableUndirectedGraph();

  // Description:
  // Graph edge that is reused of AddGraphEdge calls.
  vtkGraphEdge *GraphEdge;

private:
  vtkMutableUndirectedGraph(const vtkMutableUndirectedGraph&);  // Not implemented.
  void operator=(const vtkMutableUndirectedGraph&);  // Not implemented.
};

#endif
