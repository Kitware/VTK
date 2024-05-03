// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridGenerateProcessIds
 * @brief   Sets ProcessIds attribute for CellData.
 *
 * vtkHyperTreeGridGenerateProcessIds is meant to fill in the ProcessIds attribute array,
 * to know which processor owns which cells. The ProcessIds array's name is "ProcessIds".
 */

#ifndef vtkHyperTreeGridGenerateProcessIds_h
#define vtkHyperTreeGridGenerateProcessIds_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"
#include "vtkWeakPointer.h" // For vtkWeakPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkIdTypeArray;
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkHyperTreeGridGenerateProcessIds
  : public vtkPassInputTypeAlgorithm
{
public:
  static vtkHyperTreeGridGenerateProcessIds* New();

  vtkTypeMacro(vtkHyperTreeGridGenerateProcessIds, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * By default this filter uses the global controller,
   * but this method can be used to set another instead.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkMultiProcessController* GetController();
  ///@}

protected:
  vtkHyperTreeGridGenerateProcessIds();
  ~vtkHyperTreeGridGenerateProcessIds() override;

  // Append the pieces.
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

private:
  vtkHyperTreeGridGenerateProcessIds(const vtkHyperTreeGridGenerateProcessIds&) = delete;
  void operator=(const vtkHyperTreeGridGenerateProcessIds&) = delete;

  vtkWeakPointer<vtkMultiProcessController> Controller;
};

VTK_ABI_NAMESPACE_END
#endif
