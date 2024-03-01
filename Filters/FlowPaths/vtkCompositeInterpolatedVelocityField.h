// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCompositeInterpolatedVelocityField
 * @brief   An abstract class for
 *  obtaining the interpolated velocity values at a point
 *
 *  vtkCompositeInterpolatedVelocityField acts as a continuous velocity field
 *  by performing cell interpolation on one or more underlying vtkDataSets. That is,
 *  composite datasets are combined to create a continuous velocity field. The default
 *  strategy is to use the closest point strategy.
 *
 * @warning
 *  vtkCompositeInterpolatedVelocityField is not thread safe. A new instance
 *  should be created by each thread.
 *
 * @sa
 *  vtkAbstractInterpolatedVelocityField vtkAMRInterpolatedVelocityField
 *  vtkGenericInterpolatedVelocityField vtkTemporalInterpolatedVelocityField
 *  vtkFunctionSet vtkStreamTracer
 */

#ifndef vtkCompositeInterpolatedVelocityField_h
#define vtkCompositeInterpolatedVelocityField_h

#include "vtkAbstractInterpolatedVelocityField.h"
#include "vtkFiltersFlowPathsModule.h" // For export macro

#include <array>  // For array
#include <vector> // For vector

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;

class VTKFILTERSFLOWPATHS_EXPORT vtkCompositeInterpolatedVelocityField
  : public vtkAbstractInterpolatedVelocityField
{
public:
  ///@{
  /**
   * Standard methods for type information and printing.
   */
  vtkTypeMacro(vtkCompositeInterpolatedVelocityField, vtkAbstractInterpolatedVelocityField);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Construct a vtkCompositeInterpolatedVelocityField class.
   */
  static vtkCompositeInterpolatedVelocityField* New();

  /**
   * Add a dataset for implicit velocity function evaluation. If more than
   * one dataset is added, the evaluation point is searched in all until a
   * match is found. THIS FUNCTION DOES NOT CHANGE THE REFERENCE COUNT OF
   * dataset FOR THREAD SAFETY REASONS. MaxCellSize can be passed to avoid
   * recomputing GetMaxCellSize().
   */
  virtual void AddDataSet(vtkDataSet* dataset, size_t maxCellSize = 0);

  using Superclass::FunctionValues;
  /**
   * Evaluate the velocity field f at point (x, y, z).
   */
  int FunctionValues(double* x, double* f) override;

  /**
   * Check if point x is inside the dataset.
   */
  int InsideTest(double* x);

  /**
   * Project the provided point on current cell, current dataset.
   * The found cell is expected to be planar and contains at least
   * three non-aligned points. If not, the point will not be snapped.
   *
   * Return 1 and fill pProj if snap has been performed,
   * return 0 otherwise.
   */
  virtual int SnapPointOnCell(double* pOrigin, double* pProj);

  /**
   * Set the cell id cached by the last evaluation within a specified dataset.
   */
  void SetLastCellId(vtkIdType c, int dataindex) override;

  /**
   * Set the cell id cached by the last evaluation.
   */
  void SetLastCellId(vtkIdType c) override { this->Superclass::SetLastCellId(c); }

  ///@{
  /**
   * Get the most recently visited dataset and its id. The dataset is used
   * for a guess regarding where the next point will be, without searching
   * through all datasets. When setting the last dataset, care is needed as
   * no reference counting or checks are performed. This feature is intended
   * for custom interpolators only that cache datasets independently.
   */
  vtkGetMacro(LastDataSetIndex, int);
  ///@}

  ///@{
  /**
   * Get Cache DataSet hits and misses.
   */
  vtkGetMacro(CacheDataSetHit, int);
  vtkGetMacro(CacheDataSetMiss, int);
  ///@}

  /**
   * Copy essential parameters between instances of this class. See
   * vtkAbstractInterpolatedVelocityField for more information.
   */
  void CopyParameters(vtkAbstractInterpolatedVelocityField* from) override;

protected:
  vtkCompositeInterpolatedVelocityField();
  ~vtkCompositeInterpolatedVelocityField() override;

  friend class vtkTemporalInterpolatedVelocityField;
  /**
   * Evaluate the velocity field f at point (x, y, z) in a specified dataset
   * by either involving vtkPointLocator, via vtkPointSet::FindCell(), in
   * locating the next cell (for datasets of type vtkPointSet) or simply
   * invoking vtkImageData::FindCell() or vtkRectilinearGrid::FindCell() to
   * fulfill the same task if the point is outside the current cell.
   */
  int FunctionValues(vtkDataSet* ds, double* x, double* f) override
  {
    return this->Superclass::FunctionValues(ds, x, f);
  }

  int CacheDataSetHit;
  int CacheDataSetMiss;
  int LastDataSetIndex;
  struct DataSetBoundsInformation
  {
    vtkDataSet* DataSet;
    std::array<double, 6> Bounds{};
    DataSetBoundsInformation();
    DataSetBoundsInformation(vtkDataSet* ds);
  };
  std::vector<DataSetBoundsInformation> DataSetsBoundsInfo;

private:
  vtkCompositeInterpolatedVelocityField(const vtkCompositeInterpolatedVelocityField&) = delete;
  void operator=(const vtkCompositeInterpolatedVelocityField&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
