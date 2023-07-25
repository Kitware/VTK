// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMultiBlockDataGroupFilter
 * @brief   collects multiple inputs into one multi-group dataset
 *
 * vtkMultiBlockDataGroupFilter is an M to 1 filter that merges multiple
 * input into one multi-group dataset. It will assign each input to
 * one group of the multi-group dataset and will assign each update piece
 * as a sub-block. For example, if there are two inputs and four update
 * pieces, the output contains two groups with four datasets each.
 *
 * `vtkGroupDataSetsFilter` is a newer filter that can be used for similar
 * use-cases and is more flexible. It is recommended that new code uses
 * vtkGroupDataSetsFilter instead of this one.
 */

#ifndef vtkMultiBlockDataGroupFilter_h
#define vtkMultiBlockDataGroupFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkMultiBlockDataGroupFilter : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkMultiBlockDataGroupFilter, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with PointIds and CellIds on; and ids being generated
   * as scalars.
   */
  static vtkMultiBlockDataGroupFilter* New();

  ///@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use AddInputConnection() to
   * setup a pipeline connection.
   */
  void AddInputData(vtkDataObject*);
  void AddInputData(int, vtkDataObject*);
  ///@}

protected:
  vtkMultiBlockDataGroupFilter();
  ~vtkMultiBlockDataGroupFilter() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkMultiBlockDataGroupFilter(const vtkMultiBlockDataGroupFilter&) = delete;
  void operator=(const vtkMultiBlockDataGroupFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
