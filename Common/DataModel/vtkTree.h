// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkTree
 * @brief   A rooted tree data structure.
 *
 *
 * vtkTree is a connected directed graph with no cycles. A tree is a type of
 * directed graph, so works with all graph algorithms.
 *
 * vtkTree is a read-only data structure.
 * To construct a tree, create an instance of vtkMutableDirectedGraph.
 * Add vertices and edges with AddVertex() and AddEdge(). You may alternately
 * start by adding a single vertex as the root then call graph->AddChild(parent)
 * which adds a new vertex and connects the parent to the child.
 * The tree MUST have all edges in the proper direction, from parent to child.
 * After building the tree, call tree->CheckedShallowCopy(graph) to copy the
 * structure into a vtkTree. This method will return false if the graph is
 * an invalid tree.
 *
 * vtkTree provides some convenience methods for obtaining the parent and
 * children of a vertex, for finding the root, and determining if a vertex
 * is a leaf (a vertex with no children).
 *
 * @sa
 * vtkDirectedGraph vtkMutableDirectedGraph vtkGraph
 */

#ifndef vtkTree_h
#define vtkTree_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDirectedAcyclicGraph.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkIdTypeArray;

class VTKCOMMONDATAMODEL_EXPORT vtkTree : public vtkDirectedAcyclicGraph
{
public:
  static vtkTree* New();
  vtkTypeMacro(vtkTree, vtkDirectedAcyclicGraph);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_TREE; }

  ///@{
  /**
   * Get the root vertex of the tree.
   */
  vtkGetMacro(Root, vtkIdType);
  ///@}

  /**
   * Get the number of children of a vertex.
   */
  vtkIdType GetNumberOfChildren(vtkIdType v) { return this->GetOutDegree(v); }

  /**
   * Get the i-th child of a parent vertex.
   */
  vtkIdType GetChild(vtkIdType v, vtkIdType i);

  /**
   * Get the child vertices of a vertex.
   * This is a convenience method that functions exactly like
   * GetAdjacentVertices.
   */
  void GetChildren(vtkIdType v, vtkAdjacentVertexIterator* it) { this->GetAdjacentVertices(v, it); }

  /**
   * Get the parent of a vertex.
   */
  vtkIdType GetParent(vtkIdType v);

  /**
   * Get the edge connecting the vertex to its parent.
   */
  vtkEdgeType GetParentEdge(vtkIdType v);

  /**
   * Get the level of the vertex in the tree.  The root vertex has level 0.
   * Returns -1 if the vertex id is < 0 or greater than the number of vertices
   * in the tree.
   */
  vtkIdType GetLevel(vtkIdType v);

  /**
   * Return whether the vertex is a leaf (i.e. it has no children).
   */
  bool IsLeaf(vtkIdType vertex);

  ///@{
  /**
   * Retrieve a graph from an information vector.
   */
  static vtkTree* GetData(vtkInformation* info);
  static vtkTree* GetData(vtkInformationVector* v, int i = 0);
  ///@}

  /**
   * Reorder the children of a parent vertex.
   * The children array must contain all the children of parent,
   * just in a different order.
   * This does not change the topology of the tree.
   */
  virtual void ReorderChildren(vtkIdType parent, vtkIdTypeArray* children);

protected:
  vtkTree();
  ~vtkTree() override;

  /**
   * Check the storage, and accept it if it is a valid
   * tree.
   */
  bool IsStructureValid(vtkGraph* g) override;

  /**
   * The root of the tree.
   */
  vtkIdType Root;

private:
  vtkTree(const vtkTree&) = delete;
  void operator=(const vtkTree&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
