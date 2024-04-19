// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkExecutionRange
 * @brief Define a range for the `vtkForEach`.
 *
 * vtkExecutionRange is an interface used to define the range of a vtkForEach.
 * It uses the same interface as a vtkAlgorithm because it roles it to split
 * the execution regarding a given parameter. It can be a timestep, a block ID
 * or any other parameter.
 *
 * @sa vtkForEach, vtkEndFor
 */

#ifndef vtkExecutionRange_h
#define vtkExecutionRange_h

#include "vtkObject.h"

#include "vtkCommonExecutionModelModule.h" // for export macro
#include "vtkSmartPointer.h"               // for smart pointer signature

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
