// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPProjectSphereFilter
 * @brief   A filter to 'unroll' a sphere.  The
 * unroll longitude is -180.
 *
 *
 */

#ifndef vtkPProjectSphereFilter_h
#define vtkPProjectSphereFilter_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkProjectSphereFilter.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSPARALLEL_EXPORT vtkPProjectSphereFilter : public vtkProjectSphereFilter
{
public:
  vtkTypeMacro(vtkPProjectSphereFilter, vtkProjectSphereFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPProjectSphereFilter* New();

protected:
  vtkPProjectSphereFilter();
  ~vtkPProjectSphereFilter() override;

  /**
   * Parallel part of the algorithm to figure out the closest point
   * to the centerline (i.e. line connecting -90 latitude to 90 latitude)
   * if we don't build cells using points at the poles.
   */
  void ComputePointsClosestToCenterLine(double, vtkIdList*) override;

  /**
   * If TranslateZ is true then this is the method that computes
   * the amount to translate.
   */
  double GetZTranslation(vtkPointSet* input) override;

private:
  vtkPProjectSphereFilter(const vtkPProjectSphereFilter&) = delete;
  void operator=(const vtkPProjectSphereFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPProjectSphereFilter_h
