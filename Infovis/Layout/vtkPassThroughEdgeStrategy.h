// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPassThroughEdgeStrategy
 * @brief   passes edge routing information through
 *
 *
 * Simply passes existing edge layout information from the input to the
 * output without making changes.
 */

#ifndef vtkPassThroughEdgeStrategy_h
#define vtkPassThroughEdgeStrategy_h

#include "vtkEdgeLayoutStrategy.h"
#include "vtkInfovisLayoutModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISLAYOUT_EXPORT vtkPassThroughEdgeStrategy : public vtkEdgeLayoutStrategy
{
public:
  static vtkPassThroughEdgeStrategy* New();
  vtkTypeMacro(vtkPassThroughEdgeStrategy, vtkEdgeLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This is the layout method where the graph that was
   * set in SetGraph() is laid out.
   */
  void Layout() override;

protected:
  vtkPassThroughEdgeStrategy();
  ~vtkPassThroughEdgeStrategy() override;

private:
  vtkPassThroughEdgeStrategy(const vtkPassThroughEdgeStrategy&) = delete;
  void operator=(const vtkPassThroughEdgeStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
