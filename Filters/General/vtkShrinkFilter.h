// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkShrinkFilter
 * @brief   shrink cells composing an arbitrary data set
 *
 * vtkShrinkFilter shrinks cells composing an arbitrary data set
 * towards their centroid. The centroid of a cell is computed as the
 * average position of the cell points. Shrinking results in
 * disconnecting the cells from one another. The output of this filter
 * is of general dataset type vtkUnstructuredGrid.
 *
 * @warning
 * It is possible to turn cells inside out or cause self intersection
 * in special cases.
 *
 * @sa
 * vtkShrinkPolyData
 */

#ifndef vtkShrinkFilter_h
#define vtkShrinkFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkShrinkFilter : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkShrinkFilter* New();
  vtkTypeMacro(vtkShrinkFilter, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the fraction of shrink for each cell. The default is 0.5.
   */
  vtkSetClampMacro(ShrinkFactor, double, 0.0, 1.0);
  vtkGetMacro(ShrinkFactor, double);
  ///@}

protected:
  vtkShrinkFilter();
  ~vtkShrinkFilter() override;

  // Override to specify support for any vtkDataSet input type.
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // Main implementation.
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double ShrinkFactor;

private:
  vtkShrinkFilter(const vtkShrinkFilter&) = delete;
  void operator=(const vtkShrinkFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
