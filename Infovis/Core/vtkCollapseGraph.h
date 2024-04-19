// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkCollapseGraph
 * @brief   "Collapses" vertices onto their neighbors.
 *
 *
 * vtkCollapseGraph "collapses" vertices onto their neighbors, while maintaining
 * connectivity.  Two inputs are required - a graph (directed or undirected),
 * and a vertex selection that can be converted to indices.
 *
 * Conceptually, each of the vertices specified in the input selection
 * expands, "swallowing" adacent vertices.  Edges to-or-from the "swallowed"
 * vertices become edges to-or-from the expanding vertices, maintaining the
 * overall graph connectivity.
 *
 * In the case of directed graphs, expanding vertices only swallow vertices that
 * are connected via out edges.  This rule provides intuitive behavior when
 * working with trees, so that "child" vertices collapse into their parents
 * when the parents are part of the input selection.
 *
 * Input port 0: graph
 * Input port 1: selection
 */

#ifndef vtkCollapseGraph_h
#define vtkCollapseGraph_h

#include "vtkGraphAlgorithm.h"
#include "vtkInfovisCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISCORE_EXPORT vtkCollapseGraph : public vtkGraphAlgorithm
{
public:
  static vtkCollapseGraph* New();
  vtkTypeMacro(vtkCollapseGraph, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Convenience function provided for setting the graph input.
  void SetGraphConnection(vtkAlgorithmOutput*);
  /// Convenience function provided for setting the selection input.
  void SetSelectionConnection(vtkAlgorithmOutput*);

protected:
  vtkCollapseGraph();
  ~vtkCollapseGraph() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkCollapseGraph(const vtkCollapseGraph&) = delete;
  void operator=(const vtkCollapseGraph&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
