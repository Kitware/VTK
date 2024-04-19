// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkShrinkPolyData
 * @brief   shrink cells composing PolyData
 *
 * vtkShrinkPolyData shrinks cells composing a polygonal dataset (e.g.,
 * vertices, lines, polygons, and triangle strips) towards their centroid.
 * The centroid of a cell is computed as the average position of the
 * cell points. Shrinking results in disconnecting the cells from
 * one another. The output dataset type of this filter is polygonal data.
 *
 * During execution the filter passes its input cell data to its
 * output. Point data attributes are copied to the points created during the
 * shrinking process.
 *
 * @warning
 * It is possible to turn cells inside out or cause self intersection
 * in special cases.
 * Users should use the vtkTriangleFilter to triangulate meshes that
 * contain triangle strips.
 *
 * @sa
 * vtkShrinkFilter
 */

#ifndef vtkShrinkPolyData_h
#define vtkShrinkPolyData_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkShrinkPolyData : public vtkPolyDataAlgorithm
{
public:
  static vtkShrinkPolyData* New();
  vtkTypeMacro(vtkShrinkPolyData, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the fraction of shrink for each cell.
   */
  vtkSetClampMacro(ShrinkFactor, double, 0.0, 1.0);
  ///@}

  ///@{
  /**
   * Get the fraction of shrink for each cell.
   */
  vtkGetMacro(ShrinkFactor, double);
  ///@}

protected:
  vtkShrinkPolyData(double sf = 0.5);
  ~vtkShrinkPolyData() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  double ShrinkFactor;

private:
  vtkShrinkPolyData(const vtkShrinkPolyData&) = delete;
  void operator=(const vtkShrinkPolyData&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
