// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStructuredPointsGeometryFilter
 * @brief   obsolete class
 *
 * vtkStructuredPointsGeometryFilter has been renamed to
 * vtkImageDataGeometryFilter
 */

#ifndef vtkStructuredPointsGeometryFilter_h
#define vtkStructuredPointsGeometryFilter_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkImageDataGeometryFilter.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGEOMETRY_EXPORT vtkStructuredPointsGeometryFilter
  : public vtkImageDataGeometryFilter
{
public:
  vtkTypeMacro(vtkStructuredPointsGeometryFilter, vtkImageDataGeometryFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct with initial extent of all the data
   */
  static vtkStructuredPointsGeometryFilter* New();

protected:
  vtkStructuredPointsGeometryFilter();
  ~vtkStructuredPointsGeometryFilter() override = default;

private:
  vtkStructuredPointsGeometryFilter(const vtkStructuredPointsGeometryFilter&) = delete;
  void operator=(const vtkStructuredPointsGeometryFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
