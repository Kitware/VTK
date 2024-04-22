// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkHyperTreeGridGenerateGlobalIds
 * @brief Generate global IDs of input HyperTee Grid(s)
 *
 * This filter generate global IDs of the input HyperTree Grid(s), i.e. unique cell IDs
 * among all ranks in a distributed environment. Global IDs are stored in a new cell data
 * array named "GlobalIds".
 */

#ifndef vtkHyperTreeGridGenerateGlobalIds_h
#define vtkHyperTreeGridGenerateGlobalIds_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer
#include "vtkWeakPointer.h"  // For vtkWeakPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkIdTypeArray;
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkHyperTreeGridGenerateGlobalIds : public vtkPassInputTypeAlgorithm
{
public:
  static vtkHyperTreeGridGenerateGlobalIds* New();

  vtkTypeMacro(vtkHyperTreeGridGenerateGlobalIds, vtkPassInputTypeAlgorithm);
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
  vtkHyperTreeGridGenerateGlobalIds();
  ~vtkHyperTreeGridGenerateGlobalIds() override;

  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkHyperTreeGridGenerateGlobalIds(const vtkHyperTreeGridGenerateGlobalIds&) = delete;
  void operator=(const vtkHyperTreeGridGenerateGlobalIds&) = delete;

  vtkWeakPointer<vtkMultiProcessController> Controller;
};

VTK_ABI_NAMESPACE_END
#endif
