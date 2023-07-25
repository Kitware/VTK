// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkSliceAndDiceLayoutStrategy
 * @brief   a horizontal and vertical slicing tree map layout
 *
 *
 * Lays out a tree-map alternating between horizontal and vertical slices,
 * taking into account the relative size of each vertex.
 *
 * @par Thanks:
 * Slice and dice algorithm comes from:
 * Shneiderman, B. 1992. Tree visualization with tree-maps: 2-d space-filling approach.
 * ACM Trans. Graph. 11, 1 (Jan. 1992), 92-99.
 */

#ifndef vtkSliceAndDiceLayoutStrategy_h
#define vtkSliceAndDiceLayoutStrategy_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkTreeMapLayoutStrategy.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISLAYOUT_EXPORT vtkSliceAndDiceLayoutStrategy : public vtkTreeMapLayoutStrategy
{
public:
  static vtkSliceAndDiceLayoutStrategy* New();

  vtkTypeMacro(vtkSliceAndDiceLayoutStrategy, vtkTreeMapLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform the layout of a tree and place the results as 4-tuples in
   * coordsArray (Xmin, Xmax, Ymin, Ymax).
   */
  void Layout(vtkTree* inputTree, vtkDataArray* coordsArray, vtkDataArray* sizeArray) override;

protected:
  vtkSliceAndDiceLayoutStrategy();
  ~vtkSliceAndDiceLayoutStrategy() override;

private:
  vtkSliceAndDiceLayoutStrategy(const vtkSliceAndDiceLayoutStrategy&) = delete;
  void operator=(const vtkSliceAndDiceLayoutStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
