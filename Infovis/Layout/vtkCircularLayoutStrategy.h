// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkCircularLayoutStrategy
 * @brief   Places vertices around a circle
 *
 *
 * Assigns points to the vertices around a circle with unit radius.
 */

#ifndef vtkCircularLayoutStrategy_h
#define vtkCircularLayoutStrategy_h

#include "vtkGraphLayoutStrategy.h"
#include "vtkInfovisLayoutModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISLAYOUT_EXPORT vtkCircularLayoutStrategy : public vtkGraphLayoutStrategy
{
public:
  static vtkCircularLayoutStrategy* New();

  vtkTypeMacro(vtkCircularLayoutStrategy, vtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform the layout.
   */
  void Layout() override;

protected:
  vtkCircularLayoutStrategy();
  ~vtkCircularLayoutStrategy() override;

private:
  vtkCircularLayoutStrategy(const vtkCircularLayoutStrategy&) = delete;
  void operator=(const vtkCircularLayoutStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
