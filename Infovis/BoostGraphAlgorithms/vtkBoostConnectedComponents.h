// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkBoostConnectedComponents
 * @brief   Find the connected components of a graph
 *
 *
 * vtkBoostConnectedComponents discovers the connected regions of a vtkGraph.
 * Each vertex is assigned a component ID in the vertex array "component".
 * If the graph is undirected, this is the natural connected components
 * of the graph.  If the graph is directed, this filter discovers the
 * strongly connected components of the graph (i.e. the maximal sets of
 * vertices where there is a directed path between any pair of vertices
 * within each set).
 */

#ifndef vtkBoostConnectedComponents_h
#define vtkBoostConnectedComponents_h

#include "vtkGraphAlgorithm.h"
#include "vtkInfovisBoostGraphAlgorithmsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT vtkBoostConnectedComponents : public vtkGraphAlgorithm
{
public:
  static vtkBoostConnectedComponents* New();
  vtkTypeMacro(vtkBoostConnectedComponents, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkBoostConnectedComponents();
  ~vtkBoostConnectedComponents() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkBoostConnectedComponents(const vtkBoostConnectedComponents&) = delete;
  void operator=(const vtkBoostConnectedComponents&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
