// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkSquarifyLayoutStrategy
 * @brief   uses the squarify tree map layout algorithm
 *
 *
 * vtkSquarifyLayoutStrategy partitions the space for child vertices into regions
 * that use all available space and are as close to squares as possible.
 * The algorithm also takes into account the relative vertex size.
 *
 * @par Thanks:
 * The squarified tree map algorithm comes from:
 * Bruls, D.M., C. Huizing, J.J. van Wijk. Squarified Treemaps.
 * In: W. de Leeuw, R. van Liere (eds.), Data Visualization 2000,
 * Proceedings of the joint Eurographics and IEEE TCVG Symposium on Visualization,
 * 2000, Springer, Vienna, p. 33-42.
 */

#ifndef vtkSquarifyLayoutStrategy_h
#define vtkSquarifyLayoutStrategy_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkTreeMapLayoutStrategy.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;

class VTKINFOVISLAYOUT_EXPORT vtkSquarifyLayoutStrategy : public vtkTreeMapLayoutStrategy
{
public:
  static vtkSquarifyLayoutStrategy* New();
  vtkTypeMacro(vtkSquarifyLayoutStrategy, vtkTreeMapLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform the layout of a tree and place the results as 4-tuples in
   * coordsArray (Xmin, Xmax, Ymin, Ymax).
   */
  void Layout(vtkTree* inputTree, vtkDataArray* coordsArray, vtkDataArray* sizeArray) override;

protected:
  vtkSquarifyLayoutStrategy();
  ~vtkSquarifyLayoutStrategy() override;

private:
  void LayoutChildren(vtkTree* tree, vtkDataArray* coordsArray, vtkDataArray* sizeArray,
    vtkIdType nchildren, vtkIdType parent, vtkIdType begin, float minX, float maxX, float minY,
    float maxY);

  vtkSquarifyLayoutStrategy(const vtkSquarifyLayoutStrategy&) = delete;
  void operator=(const vtkSquarifyLayoutStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
