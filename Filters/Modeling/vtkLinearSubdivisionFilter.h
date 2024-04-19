// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLinearSubdivisionFilter
 * @brief   generate a subdivision surface using the Linear Scheme
 *
 * vtkLinearSubdivisionFilter is a filter that generates output by
 * subdividing its input polydata. Each subdivision iteration create 4
 * new triangles for each triangle in the polydata.
 *
 * @par Thanks:
 * This work was supported by PHS Research Grant No. 1 P41 RR13218-01
 * from the National Center for Research Resources.
 *
 * @sa
 * vtkInterpolatingSubdivisionFilter vtkButterflySubdivisionFilter
 */

#ifndef vtkLinearSubdivisionFilter_h
#define vtkLinearSubdivisionFilter_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkInterpolatingSubdivisionFilter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkIntArray;
class vtkPointData;
class vtkPoints;
class vtkPolyData;

class VTKFILTERSMODELING_EXPORT vtkLinearSubdivisionFilter
  : public vtkInterpolatingSubdivisionFilter
{
public:
  ///@{
  /**
   * Construct object with NumberOfSubdivisions set to 1.
   */
  static vtkLinearSubdivisionFilter* New();
  vtkTypeMacro(vtkLinearSubdivisionFilter, vtkInterpolatingSubdivisionFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

protected:
  vtkLinearSubdivisionFilter() = default;
  ~vtkLinearSubdivisionFilter() override = default;

  int GenerateSubdivisionPoints(vtkPolyData* inputDS, vtkIntArray* edgeData, vtkPoints* outputPts,
    vtkPointData* outputPD) override;

private:
  vtkLinearSubdivisionFilter(const vtkLinearSubdivisionFilter&) = delete;
  void operator=(const vtkLinearSubdivisionFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
