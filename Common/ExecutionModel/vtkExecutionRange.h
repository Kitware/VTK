// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkExecutionRange_h
#define vtkExecutionRange_h

#include "vtkObject.h"

#include "vtkCommonExecutionModelModule.h" // for export macro
#include <vtkDataObject.h>                 // for smart pointer signature
#include <vtkSmartPointer.h>               // for smart pointer signature

VTK_ABI_NAMESPACE_BEGIN

class vtkInformationVector;
class VTKCOMMONEXECUTIONMODEL_EXPORT vtkExecutionRange : public vtkObject
{
public:
  vtkTypeMacro(vtkExecutionRange, vtkObject);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  virtual int RequestDataObject(
    vtkInformationVector** inputVector, vtkInformationVector* outputVector);

  virtual int RequestInformation(
    vtkInformationVector** inputVector, vtkInformationVector* outputVector);

  virtual int RequestUpdateExtent(
    std::size_t iteration, vtkInformationVector** inputVector, vtkInformationVector* outputVector);

  virtual int RequestData(
    std::size_t iteration, vtkInformationVector** inputVector, vtkInformationVector* outputVector);

  virtual std::size_t Size();

protected:
  vtkExecutionRange() = default;
  ~vtkExecutionRange() override = default;

private:
  vtkExecutionRange(const vtkExecutionRange&) = delete;
  void operator=(const vtkExecutionRange&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif // vtkExecutionRange_h
