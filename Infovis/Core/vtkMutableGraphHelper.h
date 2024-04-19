// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMutableGraphHelper
 * @brief   Helper class for building a directed or
 *   directed graph
 *
 *
 * vtkMutableGraphHelper has helper methods AddVertex and AddEdge which
 * add vertices/edges to the underlying mutable graph. This is helpful in
 * filters which need to (re)construct graphs which may be either directed
 * or undirected.
 *
 * @sa
 * vtkGraph vtkMutableDirectedGraph vtkMutableUndirectedGraph
 */

#ifndef vtkMutableGraphHelper_h
#define vtkMutableGraphHelper_h

#include "vtkGraph.h"             // For vtkEdgeType
#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSetAttributes;
class vtkGraph;
class vtkGraphEdge;
class vtkMutableDirectedGraph;
class vtkMutableUndirectedGraph;

class VTKINFOVISCORE_EXPORT vtkMutableGraphHelper : public vtkObject
{
public:
  static vtkMutableGraphHelper* New();
  vtkTypeMacro(vtkMutableGraphHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the underlying graph that you want to modify with this helper.
   * The graph must be an instance of vtkMutableDirectedGraph or
   * vtkMutableUndirectedGraph.
   */
  void SetGraph(vtkGraph* g);
  vtkGraph* GetGraph();
  ///@}

  /**
   * Add an edge to the underlying mutable graph.
   */
  vtkEdgeType AddEdge(vtkIdType u, vtkIdType v);

  vtkGraphEdge* AddGraphEdge(vtkIdType u, vtkIdType v);

  /**
   * Add a vertex to the underlying mutable graph.
   */
  vtkIdType AddVertex();

  /**
   * Remove a vertex from the underlying mutable graph.
   */
  void RemoveVertex(vtkIdType v);

  /**
   * Remove a collection of vertices from the underlying mutable graph.
   */
  void RemoveVertices(vtkIdTypeArray* verts);

  /**
   * Remove an edge from the underlying mutable graph.
   */
  void RemoveEdge(vtkIdType e);

  /**
   * Remove a collection of edges from the underlying mutable graph.
   */
  void RemoveEdges(vtkIdTypeArray* edges);

protected:
  vtkMutableGraphHelper();
  ~vtkMutableGraphHelper() override;

  vtkGetObjectMacro(InternalGraph, vtkGraph);
  void SetInternalGraph(vtkGraph* g);
  vtkGraph* InternalGraph;

  vtkGraphEdge* GraphEdge;

  vtkMutableDirectedGraph* DirectedGraph;
  vtkMutableUndirectedGraph* UndirectedGraph;

private:
  vtkMutableGraphHelper(const vtkMutableGraphHelper&) = delete;
  void operator=(const vtkMutableGraphHelper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
