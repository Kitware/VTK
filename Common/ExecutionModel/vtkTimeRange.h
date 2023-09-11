// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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

private:
  std::size_t NumberOfTimeSteps = 0;
  std::vector<double> TimeValues;
};

VTK_ABI_NAMESPACE_END

#endif // vtkTimeRange_h
