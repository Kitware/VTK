// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAdaptiveTemporalInterpolator
 * @brief   interpolate datasets between time steps to produce a new dataset
 *
 * vtkAdaptiveTemporalInterpolator extends vtkTemporalInterpolator to
 * interpolate between timesteps when mesh topology appears to be different.
 */

#ifndef vtkAdaptiveTemporalInterpolator_h
#define vtkAdaptiveTemporalInterpolator_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkTemporalInterpolator.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkPointSet;

class VTKFILTERSPARALLEL_EXPORT vtkAdaptiveTemporalInterpolator : public vtkTemporalInterpolator
{
public:
  static vtkAdaptiveTemporalInterpolator* New();
  vtkTypeMacro(vtkAdaptiveTemporalInterpolator, vtkTemporalInterpolator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkAdaptiveTemporalInterpolator();
  ~vtkAdaptiveTemporalInterpolator() override;

  /**
   * Root level interpolation for a concrete dataset object.
   * Point/Cell data and points are interpolated.
   * Needs improving if connectivity is to be handled
   */
  vtkDataSet* InterpolateDataSet(vtkDataSet* in1, vtkDataSet* in2, double ratio) override;

  /**
   * When the mesh topology appears to be different between timesteps,
   * this method is invoked to resample point- and cell-data of one
   * dataset onto the points/cells of the other before interpolation.
   *
   * This will overwrite either \a a or \a b with a reference to the
   * resampled point-set (depending on the value of \a source). The
   * resampled point-set will also be the return value.
   * If \a source is 0, then \a b will be overwritten with a mesh
   * whose points are taken from \a a but whose point-data and cell-data
   * values correspond to \a b.
   * If \a source is non-zero, the opposite is done.
   *
   * Returns a non-null pointer to the resampled point-set on success
   * and nullptr on failure. If a valid pointer is returned, you are
   * responsible for calling Delete() on it.
   */
  vtkPointSet* ResampleDataObject(vtkPointSet*& a, vtkPointSet*& b, int source);

private:
  vtkAdaptiveTemporalInterpolator(const vtkAdaptiveTemporalInterpolator&) = delete;
  void operator=(const vtkAdaptiveTemporalInterpolator&) = delete;

  class ResamplingHelperImpl;
  ResamplingHelperImpl* ResampleImpl;
};

VTK_ABI_NAMESPACE_END
#endif
