// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkStreamGraph
 * @brief   combines two graphs
 *
 *
 * vtkStreamGraph iteratively collects information from the input graph
 * and combines it in the output graph. It internally maintains a graph
 * instance that is incrementally updated every time the filter is called.
 *
 * Each update, vtkMergeGraphs is used to combine this filter's input with the
 * internal graph.
 *
 * If you can use an edge window array to filter out old edges based on a
 * moving threshold.
 */

#ifndef vtkStreamGraph_h
#define vtkStreamGraph_h

#include "vtkGraphAlgorithm.h"
#include "vtkInfovisCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;
class vtkMergeGraphs;
class vtkMutableDirectedGraph;
class vtkMutableGraphHelper;
class vtkStringArray;
class vtkTable;

class VTKINFOVISCORE_EXPORT vtkStreamGraph : public vtkGraphAlgorithm
{
public:
  static vtkStreamGraph* New();
  vtkTypeMacro(vtkStreamGraph, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Whether to use an edge window array. The default is to
   * not use a window array.
   */
  vtkSetMacro(UseEdgeWindow, bool);
  vtkGetMacro(UseEdgeWindow, bool);
  vtkBooleanMacro(UseEdgeWindow, bool);
  ///@}

  ///@{
  /**
   * The edge window array. The default array name is "time".
   */
  vtkSetStringMacro(EdgeWindowArrayName);
  vtkGetStringMacro(EdgeWindowArrayName);
  ///@}

  ///@{
  /**
   * The time window amount. Edges with values lower
   * than the maximum value minus this window will be
   * removed from the graph. The default edge window is
   * 10000.
   */
  vtkSetMacro(EdgeWindow, double);
  vtkGetMacro(EdgeWindow, double);
  ///@}

protected:
  vtkStreamGraph();
  ~vtkStreamGraph() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkMutableGraphHelper* CurrentGraph;
  vtkMergeGraphs* MergeGraphs;
  bool UseEdgeWindow;
  double EdgeWindow;
  char* EdgeWindowArrayName;

private:
  vtkStreamGraph(const vtkStreamGraph&) = delete;
  void operator=(const vtkStreamGraph&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
