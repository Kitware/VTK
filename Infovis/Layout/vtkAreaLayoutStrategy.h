// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkAreaLayoutStrategy
 * @brief   abstract superclass for all area layout strategies
 *
 *
 * All subclasses of this class perform a area layout on a tree.
 * This involves assigning a region to each vertex in the tree,
 * and placing that information in a data array with four components per
 * tuple representing (innerRadius, outerRadius, startAngle, endAngle).
 *
 * Instances of subclasses of this class may be assigned as the layout
 * strategy to vtkAreaLayout
 *
 * @par Thanks:
 * Thanks to Jason Shepherd from Sandia National Laboratories
 * for help developing this class.
 */

#ifndef vtkAreaLayoutStrategy_h
#define vtkAreaLayoutStrategy_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkTree;
class vtkDataArray;

class VTKINFOVISLAYOUT_EXPORT vtkAreaLayoutStrategy : public vtkObject
{
public:
  vtkTypeMacro(vtkAreaLayoutStrategy, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform the layout of the input tree, and store the sector
   * bounds of each vertex as a tuple in a data array.
   * For radial layout, this is
   * (innerRadius, outerRadius, startAngle, endAngle).
   * For rectangular layout, this is
   * (xmin, xmax, ymin, ymax).

   * The sizeArray may be nullptr, or may contain the desired
   * size of each vertex in the tree.
   */
  virtual void Layout(vtkTree* inputTree, vtkDataArray* areaArray, vtkDataArray* sizeArray) = 0;

  // Modify edgeRoutingTree to have point locations appropriate
  // for routing edges on a graph overlaid on the tree.
  // Layout() is called before this method, so inputTree will contain the
  // layout locations.
  // If you do not override this method,
  // the edgeRoutingTree vertex locations are the same as the input tree.
  virtual void LayoutEdgePoints(
    vtkTree* inputTree, vtkDataArray* areaArray, vtkDataArray* sizeArray, vtkTree* edgeRoutingTree);

  /**
   * Returns the vertex id that contains pnt (or -1 if no one contains it)
   */
  virtual vtkIdType FindVertex(vtkTree* tree, vtkDataArray* array, float pnt[2]) = 0;

  // Description:
  // The amount that the regions are shrunk as a value from
  // 0.0 (full size) to 1.0 (shrink to nothing).
  vtkSetClampMacro(ShrinkPercentage, double, 0.0, 1.0);
  vtkGetMacro(ShrinkPercentage, double);

protected:
  vtkAreaLayoutStrategy();
  ~vtkAreaLayoutStrategy() override;

  double ShrinkPercentage;

private:
  vtkAreaLayoutStrategy(const vtkAreaLayoutStrategy&) = delete;
  void operator=(const vtkAreaLayoutStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
