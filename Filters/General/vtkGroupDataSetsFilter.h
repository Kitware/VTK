// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkGroupDataSetsFilter
 * @brief groups inputs into a chosen composite dataset.
 *
 * vtkGroupDataSetsFilter is a filter that can combine multiple input datasets
 * into a vtkMultBlockDataSet, vtkPartitionedDataSet, or a
 * vtkPartitionedDataSetCollection.
 *
 * The inputs are added a individual blocks in the output and can be named
 * assigned block-names using `SetInputName`.
 *
 * This is a more generic version of `vtkMultiBlockDataGroupFilter` and should
 * be preferred.
 */

#ifndef vtkGroupDataSetsFilter_h
#define vtkGroupDataSetsFilter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersGeneralModule.h" // For export macro
#include <memory>                    // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkGroupDataSetsFilter : public vtkDataObjectAlgorithm
{
public:
  static vtkGroupDataSetsFilter* New();
  vtkTypeMacro(vtkGroupDataSetsFilter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the output type. Supported values are
   * `VTK_PARTITIONED_DATA_SET_COLLECTION`, `VTK_PARTITIONED_DATA_SET`, and
   * `VTK_MULTIBLOCK_DATA_SET`.
   *
   * Default it VTK_PARTITIONED_DATA_SET_COLLECTION.
   */
  vtkSetMacro(OutputType, int);
  vtkGetMacro(OutputType, int);
  void SetOutputTypeToPartitionedDataSet();
  void SetOutputTypeToPartitionedDataSetCollection();
  void SetOutputTypeToMultiBlockDataSet();
  ///@}

  ///@{
  /**
   * API to assign names for inputs. If not specified, the filter automatically
   * creates sensible names based on the chosen output type.
   *
   * Names are not useful or relevant if output type is
   * `VTK_PARTITIONED_DATA_SET` and hence are ignored for that type.
   */
  void SetInputName(int index, const char* name);
  const char* GetInputName(int index) const;
  ///@}

  /**
   * Clears all assigned input names.
   */
  void ClearInputNames();

protected:
  vtkGroupDataSetsFilter();
  ~vtkGroupDataSetsFilter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkGroupDataSetsFilter(const vtkGroupDataSetsFilter&) = delete;
  void operator=(const vtkGroupDataSetsFilter&) = delete;

  int OutputType;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif
