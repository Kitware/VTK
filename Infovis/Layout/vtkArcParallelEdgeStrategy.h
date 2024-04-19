// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkArcParallelEdgeStrategy
 * @brief   routes parallel edges as arcs
 *
 *
 * Parallel edges are drawn as arcs, and self-loops are drawn as ovals.
 * When only one edge connects two vertices it is drawn as a straight line.
 */

#ifndef vtkArcParallelEdgeStrategy_h
#define vtkArcParallelEdgeStrategy_h

#include "vtkEdgeLayoutStrategy.h"
#include "vtkInfovisLayoutModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkGraph;

class VTKINFOVISLAYOUT_EXPORT vtkArcParallelEdgeStrategy : public vtkEdgeLayoutStrategy
{
public:
  static vtkArcParallelEdgeStrategy* New();
  vtkTypeMacro(vtkArcParallelEdgeStrategy, vtkEdgeLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This is the layout method where the graph that was
   * set in SetGraph() is laid out.
   */
  void Layout() override;

  ///@{
  /**
   * Get/Set the number of subdivisions on each edge.
   */
  vtkGetMacro(NumberOfSubdivisions, int);
  vtkSetMacro(NumberOfSubdivisions, int);
  ///@}

protected:
  vtkArcParallelEdgeStrategy();
  ~vtkArcParallelEdgeStrategy() override;

  int NumberOfSubdivisions;

private:
  vtkArcParallelEdgeStrategy(const vtkArcParallelEdgeStrategy&) = delete;
  void operator=(const vtkArcParallelEdgeStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
