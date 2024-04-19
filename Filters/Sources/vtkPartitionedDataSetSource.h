// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPartitionedDataSetSource
 * @brief a source that produces a vtkPartitionedDataSet.
 *
 * vtkPartitionedDataSetSource generates a vtkPartitionedDataSet which is
 * composed of partitions of a given vtkParametricFunction.
 * The resulting partitioned dataset is split among ranks in an even fashion
 * by default.
 *
 * The user can pass the parametric function to be used using SetParametricFunction.
 * Otherwise it will default to vtkParametricKlein as its Parametric function.
 *
 * The partitioning scheme for the produced vtkPartitionedDataSet can be controlled
 * with the methods: SetNumberOfPartitiones, EnableRank, DisableRank, EnableAllRanks,
 * DisableAllRanks.
 *
 * @see vtkParametricFunction
 * @see vtkPartitionedDataSet
 */

#ifndef vtkPartitionedDataSetSource_h
#define vtkPartitionedDataSetSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPartitionedDataSetAlgorithm.h"

#include <map> // For std::map

VTK_ABI_NAMESPACE_BEGIN
class vtkParametricFunction;
class vtkPartitionedDataSet;

class VTKFILTERSSOURCES_EXPORT vtkPartitionedDataSetSource : public vtkPartitionedDataSetAlgorithm
{
public:
  static vtkPartitionedDataSetSource* New();
  vtkTypeMacro(vtkPartitionedDataSetSource, vtkPartitionedDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Enable/Disable ranks.
   *
   * By default every rank is enabled, this default policy will be
   * changed if DisableAllRanks is used, and again reverted when
   * EnableAllRanks is used.
   *
   * EnableRank/DisableRank are used to specify individual exceptions
   * of the default policy.
   */
  void EnableRank(int rank);
  void EnableAllRanks();
  void DisableRank(int rank);
  void DisableAllRanks();
  bool IsEnabledRank(int rank);
  ///@}

  ///@{
  /**
   * Set/Get the number of partitions of the resulting PartitionedDataSet.
   * If not specified, the number of partitions will be the number of enabled
   * ranks.
   *
   * SetNumberOfPartitions(0) means auto in this context, the implementation
   * will decided the optimal number of partitions which by default will be
   * one partition per each rank.
   */
  vtkSetClampMacro(NumberOfPartitions, int, 0, VTK_INT_MAX);
  vtkGetMacro(NumberOfPartitions, int);
  ///@}

  ///@{
  /**
   * Set/Get the parametric function to be used for this source.
   */
  void SetParametricFunction(vtkParametricFunction*);
  vtkGetObjectMacro(ParametricFunction, vtkParametricFunction);
  ///@}

protected:
  vtkPartitionedDataSetSource();
  ~vtkPartitionedDataSetSource() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPartitionedDataSetSource(const vtkPartitionedDataSetSource&) = delete;
  void operator=(const vtkPartitionedDataSetSource&) = delete;

  bool RanksEnabledByDefault = true;
  int NumberOfPartitions = 0;
  std::map<int, int> Allocations;
  vtkParametricFunction* ParametricFunction = nullptr;
};

VTK_ABI_NAMESPACE_END
#endif
