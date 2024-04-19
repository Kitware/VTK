// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUniformHyperTreeGridSource
 * @brief   Create a synthetic grid of uniform hypertrees.
 *
 * This class uses input parameters, most notably a string descriptor,
 * to generate a vtkHyperTreeGrid instance representing the corresponding
 * tree-based AMR grid with uniform root cell sizes along each axis.
 *
 * @sa
 * vtkHyperTreeGridSource vtkUniformHyperTreeGrid
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, NexGen Analytics 2017
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkUniformHyperTreeGridSource_h
#define vtkUniformHyperTreeGridSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkHyperTreeGridSource.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSSOURCES_EXPORT vtkUniformHyperTreeGridSource : public vtkHyperTreeGridSource
{
public:
  vtkTypeMacro(vtkUniformHyperTreeGridSource, vtkHyperTreeGridSource);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkUniformHyperTreeGridSource* New();

protected:
  vtkUniformHyperTreeGridSource();
  ~vtkUniformHyperTreeGridSource() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillOutputPortInformation(int, vtkInformation*) override;

private:
  vtkUniformHyperTreeGridSource(const vtkUniformHyperTreeGridSource&) = delete;
  void operator=(const vtkUniformHyperTreeGridSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
