// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkMergeGraphs
 * @brief   combines two graphs
 *
 *
 * vtkMergeGraphs combines information from two graphs into one.
 * Both graphs must have pedigree ids assigned to the vertices.
 * The output will contain the vertices/edges in the first graph, in
 * addition to:
 *
 *  - vertices in the second graph whose pedigree id does not
 *    match a vertex in the first input
 *
 *  - edges in the second graph
 *
 * The output will contain the same attribute structure as the input;
 * fields associated only with the second input graph will not be passed
 * to the output. When possible, the vertex/edge data for new vertices and
 * edges will be populated with matching attributes on the second graph.
 * To be considered a matching attribute, the array must have the same name,
 * type, and number of components.
 *
 * @warning
 * This filter is not "domain-aware". Pedigree ids are assumed to be globally
 * unique, regardless of their domain.
 */

#ifndef vtkMergeGraphs_h
#define vtkMergeGraphs_h

#include "vtkGraphAlgorithm.h"
#include "vtkInfovisCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;
class vtkMutableGraphHelper;
class vtkStringArray;
class vtkTable;

class VTKINFOVISCORE_EXPORT vtkMergeGraphs : public vtkGraphAlgorithm
{
public:
  static vtkMergeGraphs* New();
  vtkTypeMacro(vtkMergeGraphs, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This is the core functionality of the algorithm. Adds edges
   * and vertices from g2 into g1.
   */
  int ExtendGraph(vtkMutableGraphHelper* g1, vtkGraph* g2);

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
  vtkMergeGraphs();
  ~vtkMergeGraphs() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  bool UseEdgeWindow;
  char* EdgeWindowArrayName;
  double EdgeWindow;

private:
  vtkMergeGraphs(const vtkMergeGraphs&) = delete;
  void operator=(const vtkMergeGraphs&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
