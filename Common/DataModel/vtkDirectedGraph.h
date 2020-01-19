// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkDirectedGraph
 * @brief   A directed graph.
 *
 *
 * vtkDirectedGraph is a collection of vertices along with a collection of
 * directed edges (edges that have a source and target). ShallowCopy()
 * and DeepCopy() (and CheckedShallowCopy(), CheckedDeepCopy())
 * accept instances of vtkTree and vtkMutableDirectedGraph.
 *
 * vtkDirectedGraph is read-only. To create an undirected graph,
 * use an instance of vtkMutableDirectedGraph, then you may set the
 * structure to a vtkDirectedGraph using ShallowCopy().
 *
 * @sa
 * vtkGraph vtkMutableDirectedGraph
 */

#ifndef vtkDirectedGraph_h
#define vtkDirectedGraph_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkGraph.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkDirectedGraph : public vtkGraph
{
public:
  static vtkDirectedGraph* New();
  vtkTypeMacro(vtkDirectedGraph, vtkGraph);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_DIRECTED_GRAPH; }

  ///@{
  /**
   * Retrieve a graph from an information vector.
   */
  static vtkDirectedGraph* GetData(vtkInformation* info);
  static vtkDirectedGraph* GetData(vtkInformationVector* v, int i = 0);
  ///@}

  /**
   * Check the storage, and accept it if it is a valid
   * undirected graph. This is public to allow
   * the ToDirected/UndirectedGraph to work.
   */
  bool IsStructureValid(vtkGraph* g) override;

protected:
  vtkDirectedGraph();
  ~vtkDirectedGraph() override;

private:
  vtkDirectedGraph(const vtkDirectedGraph&) = delete;
  void operator=(const vtkDirectedGraph&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
