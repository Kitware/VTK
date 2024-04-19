// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkPassThroughLayoutStrategy
 * @brief   a layout strategy that does absolutely nothing
 *
 *
 * Yes, this incredible strategy does absoluted nothing to the data
 * so in affect passes through the graph untouched. This strategy
 * is useful in the cases where the graph is already laid out.
 */

#ifndef vtkPassThroughLayoutStrategy_h
#define vtkPassThroughLayoutStrategy_h

#include "vtkGraphLayoutStrategy.h"
#include "vtkInfovisLayoutModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISLAYOUT_EXPORT vtkPassThroughLayoutStrategy : public vtkGraphLayoutStrategy
{
public:
  static vtkPassThroughLayoutStrategy* New();

  vtkTypeMacro(vtkPassThroughLayoutStrategy, vtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This strategy sets up some data structures
   * for faster processing of each Layout() call
   */
  void Initialize() override;

  /**
   * This is the layout method where the graph that was
   * set in SetGraph() is laid out. The method can either
   * entirely layout the graph or iteratively lay out the
   * graph. If you have an iterative layout please implement
   * the IsLayoutComplete() method.
   */
  void Layout() override;

  /**
   * I'm an iterative layout so this method lets the caller
   * know if I'm done laying out the graph
   */
  int IsLayoutComplete() override { return 1; }

protected:
  vtkPassThroughLayoutStrategy();
  ~vtkPassThroughLayoutStrategy() override;

private:
  vtkPassThroughLayoutStrategy(const vtkPassThroughLayoutStrategy&) = delete;
  void operator=(const vtkPassThroughLayoutStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
