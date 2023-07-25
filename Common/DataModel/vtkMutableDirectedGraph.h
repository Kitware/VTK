// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkMutableDirectedGraph
 * @brief   An editable directed graph.
 *
 *
 * vtkMutableDirectedGraph is a directed graph which has additional methods
 * for adding edges and vertices. AddChild() is a convenience method for
 * constructing trees. ShallowCopy(), DeepCopy(), CheckedShallowCopy() and
 * CheckedDeepCopy() will succeed for instances of vtkDirectedGraph,
 * vtkMutableDirectedGraph and vtkTree.
 *
 * @sa
 * vtkDirectedGraph vtkGraph vtkTree
 */

#ifndef vtkMutableDirectedGraph_h
#define vtkMutableDirectedGraph_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDirectedGraph.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkEdgeListIterator;
class vtkGraphEdge;
class vtkVariant;

class VTKCOMMONDATAMODEL_EXPORT vtkMutableDirectedGraph : public vtkDirectedGraph
{
public:
  static vtkMutableDirectedGraph* New();
  vtkTypeMacro(vtkMutableDirectedGraph, vtkDirectedGraph);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Allocates space for the specified number of vertices in the graph's
   * internal data structures.

   * This has no effect on the number of vertex coordinate tuples or
   * vertex attribute tuples allocated; you are responsible for
   * guaranteeing these match.
   * Also, this call is not implemented for distributed-memory graphs since
   * the semantics are unclear; calling this function on a graph with a
   * non-nullptr DistributedGraphHelper will generate an error message and
   * no allocation will be performed.
   */
  virtual vtkIdType SetNumberOfVertices(vtkIdType numVerts);

  /**
   * Adds a vertex to the graph and returns the index of the new vertex.

   * \note In a distributed graph (i.e. a graph whose DistributedHelper
   * is non-null), this routine cannot be used to add a vertex
   * if the vertices in the graph have pedigree IDs, because this routine
   * will always add the vertex locally, which may conflict with the
   * proper location of the vertex based on the distribution of the
   * pedigree IDs.
   */
  vtkIdType AddVertex();

  /**
   * Adds a vertex to the graph with associated properties defined in
   * \p propertyArr and returns the index of the new vertex.
   * The number and order of values in \p propertyArr must match up with the
   * arrays in the vertex data retrieved by GetVertexData().

   * If a vertex with the given pedigree ID already exists, its properties will be
   * overwritten with the properties in \p propertyArr and the existing
   * vertex index will be returned.

   * \note In a distributed graph (i.e. a graph whose DistributedHelper
   * is non-null) the vertex added or found might not be local. In this case,
   * AddVertex will wait until the vertex can be added or found
   * remotely, so that the proper vertex index can be returned. If you
   * don't actually need to use the vertex index, consider calling
   * LazyAddVertex, which provides better performance by eliminating
   * the delays associated with returning the vertex index.
   */
  vtkIdType AddVertex(vtkVariantArray* propertyArr);

  /**
   * Adds a vertex with the given \p pedigreeID to the graph and
   * returns the index of the new vertex.

   * If a vertex with the given pedigree ID already exists,
   * the existing vertex index will be returned.

   * \note In a distributed graph (i.e. a graph whose DistributedHelper
   * is non-null) the vertex added or found might not be local. In this case,
   * AddVertex will wait until the vertex can be added or found
   * remotely, so that the proper vertex index can be returned. If you
   * don't actually need to use the vertex index, consider calling
   * LazyAddVertex, which provides better performance by eliminating
   * the delays associated with returning the vertex index.
   */
  vtkIdType AddVertex(const vtkVariant& pedigreeId);

  /**
   * Adds a directed edge from \p u to \p v,
   * where \p u and \p v are vertex indices,
   * and returns a \p vtkEdgeType structure describing that edge.

   * \p vtkEdgeType contains fields for \p Source vertex index,
   * \p Target vertex index, and edge index \p Id.
   */
  vtkEdgeType AddEdge(vtkIdType u, vtkIdType v);

  /**
   * Adds a directed edge from \p u to \p v,
   * where \p u and \p v are vertex indices,
   * with associated properties defined in \p propertyArr
   * and returns a \p vtkEdgeType structure describing that edge.

   * The number and order of values in \p propertyArr must match up with the
   * arrays in the edge data retrieved by GetEdgeData().

   * \p vtkEdgeType contains fields for \p Source vertex index,
   * \p Target vertex index, and edge index \p Id.
   */
  vtkEdgeType AddEdge(vtkIdType u, vtkIdType v, vtkVariantArray* propertyArr);

  /**
   * Adds a directed edge from \p u to \p v,
   * where \p u is a vertex pedigree ID and \p v is a vertex index,
   * and returns a \p vtkEdgeType structure describing that edge.

   * The number and order of values in the optional parameter
   * \p propertyArr must match up with the arrays in the edge data
   * retrieved by GetEdgeData().

   * \p vtkEdgeType contains fields for \p Source vertex index,
   * \p Target vertex index, and edge index \p Id.
   */
  vtkEdgeType AddEdge(const vtkVariant& u, vtkIdType v, vtkVariantArray* propertyArr = nullptr);

  /**
   * Adds a directed edge from \p u to \p v,
   * where \p u is a vertex index and \p v is a vertex pedigree ID,
   * and returns a \p vtkEdgeType structure describing that edge.

   * The number and order of values in the optional parameter
   * \p propertyArr must match up with the arrays in the edge data
   * retrieved by GetEdgeData().

   * \p vtkEdgeType contains fields for \p Source vertex index,
   * \p Target vertex index, and edge index \p Id.
   */
  vtkEdgeType AddEdge(vtkIdType u, const vtkVariant& v, vtkVariantArray* propertyArr = nullptr);

  /**
   * Adds a directed edge from \p u to \p v,
   * where \p u and \p v are vertex pedigree IDs,
   * and returns a \p vtkEdgeType structure describing that edge.

   * The number and order of values in the optional parameter
   * \p propertyArr must match up with the arrays in the edge data
   * retrieved by GetEdgeData().

   * \p vtkEdgeType contains fields for \p Source vertex index,
   * \p Target vertex index, and edge index \p Id.
   */
  vtkEdgeType AddEdge(
    const vtkVariant& u, const vtkVariant& v, vtkVariantArray* propertyArr = nullptr);

  /**
   * Adds a vertex to the graph.

   * This method is lazily evaluated for distributed graphs (i.e. graphs
   * whose DistributedHelper is non-null) the next time Synchronize is
   * called on the helper.
   */
  void LazyAddVertex();

  /**
   * Adds a vertex to the graph with associated properties defined in
   * \p propertyArr.
   * The number and order of values in \p propertyArr must match up with the
   * arrays in the vertex data retrieved by GetVertexData().

   * If a vertex with the given pedigree ID already exists, its properties will be
   * overwritten with the properties in \p propertyArr.

   * This method is lazily evaluated for distributed graphs (i.e. graphs
   * whose DistributedHelper is non-null) the next time Synchronize is
   * called on the helper.
   */
  void LazyAddVertex(vtkVariantArray* propertyArr);

  /**
   * Adds a vertex with the given \p pedigreeID to the graph.

   * If a vertex with the given pedigree ID already exists,
   * no operation is performed.

   * This method is lazily evaluated for distributed graphs (i.e. graphs
   * whose DistributedHelper is non-null) the next time Synchronize is
   * called on the helper.
   */
  void LazyAddVertex(const vtkVariant& pedigreeId);

  /**
   * Adds a directed edge from \p u to \p v,
   * where \p u and \p v are vertex indices.

   * The number and order of values in the optional parameter
   * \p propertyArr must match up with the arrays in the edge data
   * retrieved by GetEdgeData().

   * This method is lazily evaluated for distributed graphs (i.e. graphs
   * whose DistributedHelper is non-null) the next time Synchronize is
   * called on the helper.
   */
  void LazyAddEdge(vtkIdType u, vtkIdType v, vtkVariantArray* propertyArr = nullptr);

  /**
   * Adds a directed edge from \p u to \p v,
   * where \p u is a vertex pedigree ID and \p v is a vertex index.

   * The number and order of values in the optional parameter
   * \p propertyArr must match up with the arrays in the edge data
   * retrieved by GetEdgeData().

   * This method is lazily evaluated for distributed graphs (i.e. graphs
   * whose DistributedHelper is non-null) the next time Synchronize is
   * called on the helper.
   */
  void LazyAddEdge(const vtkVariant& u, vtkIdType v, vtkVariantArray* propertyArr = nullptr);

  /**
   * Adds a directed edge from \p u to \p v,
   * where \p u is a vertex index and \p v is a vertex pedigree ID.

   * The number and order of values in the optional parameter
   * \p propertyArr must match up with the arrays in the edge data
   * retrieved by GetEdgeData().

   * This method is lazily evaluated for distributed graphs (i.e. graphs
   * whose DistributedHelper is non-null) the next time Synchronize is
   * called on the helper.
   */
  void LazyAddEdge(vtkIdType u, const vtkVariant& v, vtkVariantArray* propertyArr = nullptr);

  /**
   * Adds a directed edge from \p u to \p v,
   * where \p u and \p v are vertex pedigree IDs.

   * The number and order of values in the optional parameter
   * \p propertyArr must match up with the arrays in the edge data
   * retrieved by GetEdgeData().

   * This method is lazily evaluated for distributed graphs (i.e. graphs
   * whose DistributedHelper is non-null) the next time Synchronize is
   * called on the helper.
   */
  void LazyAddEdge(
    const vtkVariant& u, const vtkVariant& v, vtkVariantArray* propertyArr = nullptr);

  /**
   * Variant of AddEdge() that returns a heavyweight \p vtkGraphEdge object.
   * The graph owns the reference of the edge and will replace
   * its contents on the next call to AddGraphEdge().

   * \note This is a less efficient method for use with wrappers.
   * In C++ you should use the faster AddEdge().
   */
  vtkGraphEdge* AddGraphEdge(vtkIdType u, vtkIdType v);

  /**
   * Convenience method for creating trees.
   * Returns the newly created vertex id.
   * Shortcut for
   * \code
   * vtkIdType v = g->AddVertex();
   * g->AddEdge(parent, v);
   * \endcode
   * If non-null, \p propertyArr provides edge properties
   * for the newly-created edge. The values in \p propertyArr must match
   * up with the arrays in the edge data returned by GetEdgeData().
   */
  vtkIdType AddChild(vtkIdType parent, vtkVariantArray* propertyArr);
  vtkIdType AddChild(vtkIdType parent) { return this->AddChild(parent, nullptr); }

  /**
   * Removes the vertex from the graph along with any connected edges.
   * Note: This invalidates the last vertex index, which is reassigned to v.
   */
  void RemoveVertex(vtkIdType v);

  /**
   * Removes the edge from the graph.
   * Note: This invalidates the last edge index, which is reassigned to e.
   */
  void RemoveEdge(vtkIdType e);

  /**
   * Removes a collection of vertices from the graph along with any connected edges.
   */
  void RemoveVertices(vtkIdTypeArray* arr);

  /**
   * Removes a collection of edges from the graph.
   */
  void RemoveEdges(vtkIdTypeArray* arr);

protected:
  vtkMutableDirectedGraph();
  ~vtkMutableDirectedGraph() override;

  /**
   * Graph edge that is reused of \p AddGraphEdge calls.
   */
  vtkGraphEdge* GraphEdge;

private:
  vtkMutableDirectedGraph(const vtkMutableDirectedGraph&) = delete;
  void operator=(const vtkMutableDirectedGraph&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
