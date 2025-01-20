// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPAxisAlignedReflectionFilter
 * @brief   distributed version of vtkAxisAlignedReflectionFilter
 *
 * vtkPAxisAlignedReflectionFilter is a distributed version of vtkAxisAlignedReflectionFilter which
 * takes into consideration the full dataset bounds for performing the reflection.
 */

#ifndef vtkPAxisAlignedReflectionFilter_h
#define vtkPAxisAlignedReflectionFilter_h

#include "vtkAxisAlignedReflectionFilter.h"
#include "vtkFiltersParallelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPAxisAlignedReflectionFilter
  : public vtkAxisAlignedReflectionFilter
{
public:
  static vtkPAxisAlignedReflectionFilter* New();
  vtkTypeMacro(vtkPAxisAlignedReflectionFilter, vtkAxisAlignedReflectionFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the parallel controller.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

protected:
  vtkPAxisAlignedReflectionFilter();
  ~vtkPAxisAlignedReflectionFilter() override;

  /**
   * Compute the bounds of the input data object.
   */
  void ComputeBounds(vtkDataObject* input, double bounds[6]) override;

private:
  vtkPAxisAlignedReflectionFilter(const vtkPAxisAlignedReflectionFilter&) = delete;
  void operator=(const vtkPAxisAlignedReflectionFilter&) = delete;

  vtkMultiProcessController* Controller = nullptr;
};

VTK_ABI_NAMESPACE_END
#endif
