// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPReflectionFilter
 * @brief   parallel version of vtkReflectionFilter
 *
 * vtkPReflectionFilter is a parallel version of vtkReflectionFilter which takes
 * into consideration the full dataset bounds for performing the reflection.
 */

#ifndef vtkPReflectionFilter_h
#define vtkPReflectionFilter_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkReflectionFilter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPReflectionFilter : public vtkReflectionFilter
{
public:
  static vtkPReflectionFilter* New();
  vtkTypeMacro(vtkPReflectionFilter, vtkReflectionFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the parallel controller.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

protected:
  vtkPReflectionFilter();
  ~vtkPReflectionFilter() override;

  /**
   * Internal method to compute bounds.
   */
  int ComputeBounds(vtkDataObject* input, double bounds[6]) override;

  vtkMultiProcessController* Controller;

private:
  vtkPReflectionFilter(const vtkPReflectionFilter&) = delete;
  void operator=(const vtkPReflectionFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
