// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkTimeRange
 * @brief vtkExecutionRange using time to dispatch in a vtkForEach sub-pipeline
 *
 * vtkTimeRange is an execution range for the vtkForEach, that split execution by time steps.
 * The resulting sub-pipeline will be executed once for each time step of the input dataset.
 *
 * @sa vtkForEach, vtkExecutionRange, vtkTimeRange
 */

#ifndef vtkTimeRange_h
#define vtkTimeRange_h

#include "vtkExecutionRange.h"

#include "vtkCommonExecutionModelModule.h" // for export macro
#include <vtkDataObject.h>                 // for smart pointer signature
#include <vtkSmartPointer.h>               // for smart pointer signature

#include <vector> // for TimeValues member

VTK_ABI_NAMESPACE_BEGIN

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkTimeRange : public vtkExecutionRange
{
public:
  static vtkTimeRange* New();
  vtkTypeMacro(vtkTimeRange, vtkExecutionRange);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  int RequestInformation(
    vtkInformationVector** inputVector, vtkInformationVector* outputVector) override;

  int RequestUpdateExtent(std::size_t iteration, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  std::size_t Size() override;

protected:
  vtkTimeRange() = default;
  ~vtkTimeRange() override = default;

private:
  vtkTimeRange(const vtkTimeRange&) = delete;
  void operator=(const vtkTimeRange&) = delete;

  std::size_t NumberOfTimeSteps = 0;
  std::vector<double> TimeValues;
};

VTK_ABI_NAMESPACE_END

#endif // vtkTimeRange_h
