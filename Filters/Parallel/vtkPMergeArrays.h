// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPMergeArrays
 * @brief   Multiple inputs with one output, parallel version
 *
 * Like it's super class, this filter tries to combine all arrays from
 * inputs into one output.
 *
 * @sa
 * vtkMergeArrays
 */

#ifndef vtkPMergeArrays_h
#define vtkPMergeArrays_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkMergeArrays.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSPARALLEL_EXPORT vtkPMergeArrays : public vtkMergeArrays
{
public:
  vtkTypeMacro(vtkPMergeArrays, vtkMergeArrays);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPMergeArrays* New();

protected:
  vtkPMergeArrays();
  ~vtkPMergeArrays() override = default;

  int MergeDataObjectFields(vtkDataObject* input, int idx, vtkDataObject* output) override;

private:
  vtkPMergeArrays(const vtkPMergeArrays&) = delete;
  void operator=(const vtkPMergeArrays&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
