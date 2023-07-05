// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkExtractGhostCells
 * @brief Extracts ghost cells from the input and untag them (they become visible).
 *
 * This filter takes a `vtkDataSet` as input, removes any non-ghost cell,
 * and renames the ghost cell array in the output to what `OutputGhostArrayName` is set to
 * so it is no longer treated as a ghost type array.
 * By default, `OutputGhostArrayName` is set to `GhostType`.
 */

#ifndef vtkExtractGhostCells_h
#define vtkExtractGhostCells_h

#include "vtkFiltersGeneralModule.h" // for export macros
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkExtractGhostCells : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkExtractGhostCells* New();
  vtkTypeMacro(vtkExtractGhostCells, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set / Get the name of the ghost cell array in the output.
   */
  vtkSetStringMacro(OutputGhostArrayName);
  vtkGetStringMacro(OutputGhostArrayName);
  ///@}

protected:
  vtkExtractGhostCells();
  ~vtkExtractGhostCells() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  char* OutputGhostArrayName;

private:
  vtkExtractGhostCells(const vtkExtractGhostCells&) = delete;
  void operator=(const vtkExtractGhostCells&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
