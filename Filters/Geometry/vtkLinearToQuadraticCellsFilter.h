// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkLinearToQuadraticCellsFilter
 * @brief   degree elevate the cells of a linear unstructured grid.
 *
 *
 * vtkLinearToQuadraticCellsFilter takes an unstructured grid comprised of
 * linear cells and degree elevates each of the cells to quadratic. Additional
 * points are simply interpolated from the existing points (there is no snapping
 * to an external model).
 */

#ifndef vtkLinearToQuadraticCellsFilter_h
#define vtkLinearToQuadraticCellsFilter_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkIncrementalPointLocator;

class VTKFILTERSGEOMETRY_EXPORT vtkLinearToQuadraticCellsFilter
  : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkLinearToQuadraticCellsFilter, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkLinearToQuadraticCellsFilter* New();

  ///@{
  /**
   * Specify a spatial locator for merging points. By default, an
   * instance of vtkMergePoints is used.
   */
  void SetLocator(vtkIncrementalPointLocator* locator);
  vtkGetObjectMacro(Locator, vtkIncrementalPointLocator);
  ///@}

  /**
   * Create default locator. Used to create one when none is specified. The
   * locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   * OutputPointsPrecision is DEFAULT_PRECISION by default.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

  /**
   * Return the mtime also considering the locator.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkLinearToQuadraticCellsFilter();
  ~vtkLinearToQuadraticCellsFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkIncrementalPointLocator* Locator;
  int OutputPointsPrecision;

private:
  vtkLinearToQuadraticCellsFilter(const vtkLinearToQuadraticCellsFilter&) = delete;
  void operator=(const vtkLinearToQuadraticCellsFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
