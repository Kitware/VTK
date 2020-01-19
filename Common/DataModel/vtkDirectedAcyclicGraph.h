// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkDirectedAcyclicGraph
 * @brief   A rooted tree data structure.
 *
 *
 * vtkDirectedAcyclicGraph is a connected directed graph with no cycles. A tree is a type of
 * directed graph, so works with all graph algorithms.
 *
 * vtkDirectedAcyclicGraph is a read-only data structure.
 * To construct a tree, create an instance of vtkMutableDirectedGraph.
 * Add vertices and edges with AddVertex() and AddEdge(). You may alternately
 * start by adding a single vertex as the root then call graph->AddChild(parent)
 * which adds a new vertex and connects the parent to the child.
 * The tree MUST have all edges in the proper direction, from parent to child.
 * After building the tree, call tree->CheckedShallowCopy(graph) to copy the
 * structure into a vtkDirectedAcyclicGraph. This method will return false if the graph is
 * an invalid tree.
 *
 * vtkDirectedAcyclicGraph provides some convenience methods for obtaining the parent and
 * children of a vertex, for finding the root, and determining if a vertex
 * is a leaf (a vertex with no children).
 *
 * @sa
 * vtkDirectedGraph vtkMutableDirectedGraph vtkGraph
 */

#ifndef vtkDirectedAcyclicGraph_h
#define vtkDirectedAcyclicGraph_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDirectedGraph.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkIdTypeArray;

class VTKCOMMONDATAMODEL_EXPORT vtkDirectedAcyclicGraph : public vtkDirectedGraph
{
public:
  static vtkDirectedAcyclicGraph* New();
  vtkTypeMacro(vtkDirectedAcyclicGraph, vtkDirectedGraph);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_DIRECTED_ACYCLIC_GRAPH; }

  ///@{
  /**
   * Retrieve a graph from an information vector.
   */
  static vtkDirectedAcyclicGraph* GetData(vtkInformation* info);
  static vtkDirectedAcyclicGraph* GetData(vtkInformationVector* v, int i = 0);
  ///@}

protected:
  vtkDirectedAcyclicGraph();
  ~vtkDirectedAcyclicGraph() override;

  /**
   * Check the storage, and accept it if it is a valid
   * tree.
   */
  bool IsStructureValid(vtkGraph* g) override;

private:
  vtkDirectedAcyclicGraph(const vtkDirectedAcyclicGraph&) = delete;
  void operator=(const vtkDirectedAcyclicGraph&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
