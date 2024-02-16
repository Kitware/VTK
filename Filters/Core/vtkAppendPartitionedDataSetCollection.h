// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class  vtkAppendPartitionedDataSetCollection
 * @brief  Append partitioned dataset collections.
 *
 * vtkAppendPartitionedDataSetCollection is a filter that appends input partitioned dataset
 * collections with the same number of partitions and assembly (if present) into a single
 * output partitioned dataset collection. Each partitioned dataset of the output partitioned
 * dataset collection will either have 1 partition (merging occurs) or the N partitions,
 * where N is the summation of the number of partitions of the corresponding partitioned
 * datasets of the input partitioned dataset collections. To select the mode of the append filter,
 * use the SetAppendMode method.
 */

#ifndef vtkAppendPartitionedDataSetCollection_h
#define vtkAppendPartitionedDataSetCollection_h

#include "vtkFiltersCoreModule.h"
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSCORE_EXPORT vtkAppendPartitionedDataSetCollection
  : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkAppendPartitionedDataSetCollection* New();
  vtkTypeMacro(vtkAppendPartitionedDataSetCollection, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * AppendModes are used to specify how the append filter should behave.
   *
   * APPEND_PARTITIONS: The filter will append the partitions of each partitioned datasets into
   * a partitioned dataset with N partitions, where N is the summation of the number of partitions
   * of the corresponding partitioned datasets of the input partitioned dataset collections.
   *
   * E.g. if the input A has 1 partitioned dataset with 2 partitions and the input B has
   * 1 partitioned dataset with 3 partitions, the output will have 1 partitioned dataset with
   * 5 partitions.
   *
   * MERGE_PARTITIONS: The filter will merge the partitions of each partitioned datasets into
   * a partitioned dataset with 1 partition.
   *
   * E.g. if the input A has 1 partitioned dataset with 2 partitions and the input B has
   * 1 partitioned dataset with 3 partitions, the output will have 1 partitioned dataset with
   * 1 partition.
   */
  enum AppendModes
  {
    APPEND_PARTITIONS = 0,
    MERGE_PARTITIONS = 1,
  };

  ///@{
  /**
   * Set/Get the mode of the append filter.
   *
   * The default mode is APPEND_PARTITIONED_DATASETS.
   */
  vtkSetClampMacro(AppendMode, int, APPEND_PARTITIONS, MERGE_PARTITIONS);
  void SetAppendModeToAppendPartitions() { this->SetAppendMode(APPEND_PARTITIONS); }
  void SetAppendModeToMergePartitions() { this->SetAppendMode(MERGE_PARTITIONS); }
  vtkGetMacro(AppendMode, int);
  ///@}

  ///@{
  /**
   * Set/Get whether to append the field data of the input partitioned dataset collections.
   *
   * The default is true.
   */
  vtkSetMacro(AppendFieldData, bool);
  vtkGetMacro(AppendFieldData, bool);
  vtkBooleanMacro(AppendFieldData, bool);
  ///@}

protected:
  vtkAppendPartitionedDataSetCollection();
  ~vtkAppendPartitionedDataSetCollection() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * The input is repeatable, so we override the default implementation.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkAppendPartitionedDataSetCollection(const vtkAppendPartitionedDataSetCollection&) = delete;
  void operator=(const vtkAppendPartitionedDataSetCollection&) = delete;

  int AppendMode = AppendModes::APPEND_PARTITIONS;
  bool AppendFieldData = true;
};

VTK_ABI_NAMESPACE_END
#endif // vtkAppendPartitionedDataSetCollection_h
