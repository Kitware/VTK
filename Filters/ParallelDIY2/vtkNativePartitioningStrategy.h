// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkNativePartitioningStrategy
 * @brief A partitioning strategy based on load balancing geometric bounding boxes as cuts of a data
 * set
 *
 * This strategy is the traditional strategy used in the vtkRedistributeDataSetFilter which has been
 * refactored into this class. It is based on cuting up the data set into equally balanced (in terms
 * of cell number) bounding boxes that then get distributed to all ranks.
 *
 * The load balancing attempts to balance the number of cells
 * per target partition approximately. It uses a DIY-based kdtree implementation
 * that balances the cell centers among requested number of partitions.
 * Current implementation only supports power-of-2 target partition. If a
 * non-power of two value is specified for `NumberOfPartitions`, then the load
 * balancing simply uses the power-of-two greater than the requested value. The
 * bounding boxes for the kdtree leaf nodes are then used to redistribute the
 * data.
 *
 * Alternatively a collection of bounding boxes may be provided that can be used
 * to distribute the data instead of computing them (see `UseExplicitCuts` and
 * `SetExplicitCuts`). When explicit cuts are specified, it is possible use
 * those cuts strictly or to expand boxes on the edge to fit the domain of the
 * input dataset. This can be controlled by `ExpandExplicitCutsForInputDomain`.
 */
#ifndef vtkNativePartitioningStrategy_h
#define vtkNativePartitioningStrategy_h

#include "vtkPartitioningStrategy.h"
#include "vtkRedistributeDataSetFilter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkBoundingBox;
class vtkDataObjectTree;
class VTKFILTERSPARALLELDIY2_EXPORT vtkNativePartitioningStrategy final
  : public vtkPartitioningStrategy
{
public:
  static vtkNativePartitioningStrategy* New();
  vtkTypeMacro(vtkNativePartitioningStrategy, vtkPartitioningStrategy);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  /**
   * Implementation of parent API
   */
  std::vector<PartitionInformation> ComputePartition(vtkPartitionedDataSetCollection*) override;

  ///@{
  /**
   * Specify whether to compute the load balancing automatically or use
   * explicitly provided cuts. Set to false (default) to automatically compute
   * the cuts to use for redistributing the dataset.
   */
  vtkSetMacro(UseExplicitCuts, bool);
  vtkGetMacro(UseExplicitCuts, bool);
  vtkBooleanMacro(UseExplicitCuts, bool);
  ///@}

  ///@{
  /**
   * Specify the cuts to use when `UseExplicitCuts` is true.
   */
  void SetExplicitCuts(const std::vector<vtkBoundingBox>& boxes);
  const std::vector<vtkBoundingBox>& GetExplicitCuts() const { return this->ExplicitCuts; }
  void RemoveAllExplicitCuts();
  void AddExplicitCut(const vtkBoundingBox& bbox);
  void AddExplicitCut(const double bbox[6]);
  int GetNumberOfExplicitCuts() const;
  const vtkBoundingBox& GetExplicitCut(int index) const;
  ///@}

  ///@{
  /**
   * When using explicit cuts, it possible that the bounding box defined by all
   * the cuts is smaller than the input's bounds. In that case, the filter can
   * automatically expand the edge boxes to include the input bounds to avoid
   * clipping of the input dataset on the external faces of the combined
   * bounding box.
   *
   * Default is true, that is explicit cuts will automatically be expanded.
   *
   */
  vtkSetMacro(ExpandExplicitCuts, bool);
  vtkGetMacro(ExpandExplicitCuts, bool);
  vtkBooleanMacro(ExpandExplicitCuts, bool);
  ///@}

  /**
   * Helper function to expand a collection of bounding boxes to include the
   * `bounds` specified. This will expand any boxes in the `cuts` that abut any
   * of the external faces of the bounding box formed by all the `cuts` to
   * touch the external faces of the `bounds`.
   */
  std::vector<vtkBoundingBox> ExpandCuts(
    const std::vector<vtkBoundingBox>& cuts, const vtkBoundingBox& bounds);

  ///@{
  /**
   * Returns the cuts used by the most recent `ComputePartition` call. This is only
   * valid after a successful `ComputePartition` request.
   */
  const std::vector<vtkBoundingBox>& GetCuts() const { return this->Cuts; }
  ///@}

  ///@{
  /**
   * When UseExplicitCuts is false, and input is a
   * `vtkPartitionedDataSetCollection`, set this to true to generate cuts for
   * load balancing using all the datasets in the
   * vtkPartitionedDataSetCollection.
   *
   * Default is true.
   */
  vtkSetMacro(LoadBalanceAcrossAllBlocks, bool);
  vtkGetMacro(LoadBalanceAcrossAllBlocks, bool);
  vtkBooleanMacro(LoadBalanceAcrossAllBlocks, bool);
  ///@}

  /**
   * This method is called to generate the partitions for the input dataset.
   * Subclasses should override this to generate partitions using preferred data
   * redistribution strategy.
   *
   * The `data` will either be a `vtkPartitionedDataSet` or a `vtkDataSet`. In
   * case of `vtkPartitionedDataSet`, the method is expected to redistribute all
   * datasets (partitions) in the `vtkPartitionedDataSet` taken as a whole.
   */
  virtual std::vector<vtkBoundingBox> GenerateCuts(vtkDataObject* data);

protected:
  vtkNativePartitioningStrategy() = default;
  ~vtkNativePartitioningStrategy() override = default;

private:
  vtkNativePartitioningStrategy(const vtkNativePartitioningStrategy&) = delete;
  void operator=(const vtkNativePartitioningStrategy&) = delete;

  bool InitializeCuts(vtkDataObjectTree* input);

  std::vector<vtkBoundingBox> ExplicitCuts;
  std::vector<vtkBoundingBox> Cuts;
  bool UseExplicitCuts = false;
  bool ExpandExplicitCuts = true;

  bool LoadBalanceAcrossAllBlocks = true;
};
VTK_ABI_NAMESPACE_END

#endif // vtkNativePartitioningStrategy_h
