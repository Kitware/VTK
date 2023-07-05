// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkConvertToMultiBlockDataSet
 * @brief converts any data type into a vtkMultiBlockDataSet.
 *
 * vtkConvertToMultiBlockDataSet can convert any input dataset type to a
 * vtkMultiBlockDataSet. It packs the input dataset into a single
 * block for non-composite datasets, and for composite datasets
 * it attempts create a multiblock reflecting the input's hierarchical
 * organization. If input a vtkMultiBlockDataSet, then this acts as a simple
 * pass through filters.
 *
 * @sa vtkPConvertToMultiBlockDataSet
 */

#ifndef vtkConvertToMultiBlockDataSet_h
#define vtkConvertToMultiBlockDataSet_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkConvertToMultiBlockDataSet : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkConvertToMultiBlockDataSet* New();
  vtkTypeMacro(vtkConvertToMultiBlockDataSet, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkConvertToMultiBlockDataSet();
  ~vtkConvertToMultiBlockDataSet() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  bool Execute(vtkDataObject* input, vtkMultiBlockDataSet* output);

private:
  vtkConvertToMultiBlockDataSet(const vtkConvertToMultiBlockDataSet&) = delete;
  void operator=(const vtkConvertToMultiBlockDataSet&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
