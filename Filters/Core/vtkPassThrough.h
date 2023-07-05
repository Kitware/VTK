// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPassThrough
 * @brief Pass input input data through to the output
 *
 * vtkPassThrough simply passes input data to the output. By default, the input
 * is shallow-copied (using `vtkDataObject::ShallowCopy`). If `DeepCopyInput` is true,
 * then the input is deep-copied (using `vtkDataObject::DeepCopy`).
 *
 * The output type is always the same as the input object type.
 */

#ifndef vtkPassThrough_h
#define vtkPassThrough_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkPassThrough : public vtkPassInputTypeAlgorithm
{
public:
  static vtkPassThrough* New();
  vtkTypeMacro(vtkPassThrough, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Specify the first input port as optional
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  ///@{
  /**
   * Whether or not to deep copy the input. This can be useful if you
   * want to create a copy of a data object. You can then disconnect
   * this filter's input connections and it will act like a source.
   * Defaults to OFF.
   */
  vtkSetMacro(DeepCopyInput, vtkTypeBool);
  vtkGetMacro(DeepCopyInput, vtkTypeBool);
  vtkBooleanMacro(DeepCopyInput, vtkTypeBool);
  ///@}

  /**
   * Allow the filter to execute without error when no input connection is
   * specified. In this case, and empty vtkPolyData dataset will be created.
   * By default, this setting is false.
   * @{
   */
  vtkSetMacro(AllowNullInput, bool);
  vtkGetMacro(AllowNullInput, bool);
  vtkBooleanMacro(AllowNullInput, bool);
  /**@}*/

protected:
  vtkPassThrough();
  ~vtkPassThrough() override;

  int RequestDataObject(
    vtkInformation* request, vtkInformationVector** inVec, vtkInformationVector* outVec) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkTypeBool DeepCopyInput;
  bool AllowNullInput;

private:
  vtkPassThrough(const vtkPassThrough&) = delete;
  void operator=(const vtkPassThrough&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
