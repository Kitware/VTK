// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkRedistributeDataSetFilter
 * @brief redistributes input dataset into requested number of partitions
 *
 * vtkRedistributeDataSetFilter is intended for redistributing data in a load
 * balanced fashion.
 *
 * The filter allows users to pick how cells along the boundary of the cuts
 * either automatically generated or explicitly specified are to be distributed
 * using `BoundaryMode`. One can choose to assign those cells uniquely to one of
 * those regions or duplicate then on all regions or split the cells (using
 * vtkTableBasedClipDataSet filter). When cells are
 * duplicated along the boundary,  the filter will mark the duplicated cells as
 * `vtkDataSetAttributes::DUPLICATECELL` correctly on all but one of the
 * partitions using the ghost cell array (@sa `vtkDataSetAttributes::GhostArrayName`).
 *
 * @warning Generated duplicate ghost cells do not span entire layers of ghosts.
 * They are sparse, only appearing where cells overlap at the new boundaries between
 * partitions. If one wants to have full layers of ghost cells, one should use
 * `vtkGhostCellsGenerator`.
 *
 * Besides redistributing the data, the filter can optionally generate global
 * cell ids. This is provided since it relative easy to generate these
 * on when it is known that the data is spatially partitioned as is the case
 * after this filter has executed.
 *
 * @section vtkRedistributeDataSetFilter-SupportedDataTypes  Supported Data Types
 *
 * vtkRedistributeDataSetFilter is primarily intended for unstructured datasets
 * i.e. vtkUnstructuredGrid, vtkPolyData and composite datasets comprising of
 * the same. It will work when applied to structured datasets as well, however,
 * it results in conversion of the dataset to an unstructured grid -- which is
 * often not suitable.
 *
 * For composite datasets, the filter supports `vtkPartitionedDataSet` and
 * `vtkPartitionedDataSetCollection`. When input is a
 * `vtkPartitionedDataSetCollection`, you can set `LoadBalanceAcrossAllBlocks`
 * to true to build the load balancing KdTree using all vtkPartitionedDataSets
 * in the collection. Default is load balance each `vtkPartitionedDataSet`
 * separately.
 *
 * For `vtkMultiBlockDataSet`, the filter internally uses
 * `vtkDataAssemblyUtilities` to convert the
 * vtkMultiBlockDataSet to a vtkPartitionedDataSetCollection and back.
 */
#ifndef vtkRedistributeDataSetFilter_h
#define vtkRedistributeDataSetFilter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersParallelDIY2Module.h" // for export macros
#include "vtkPartitioningStrategy.h"      // for PartitionInformation
#include "vtkSmartPointer.h"              // for vtkSmartPointer

#include <memory> // for std::shared_ptr
#include <vector> // for std::vector

// clang-format off
#include "vtk_diy2.h" // for DIY2 APIs
#include VTK_DIY2(diy/assigner.hpp)
// clang-format on

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;
class vtkBoundingBox;
class vtkPartitionedDataSet;
class vtkMultiBlockDataSet;
class vtkMultiPieceDataSet;
class vtkDataObjectTree;
class VTKFILTERSPARALLELDIY2_EXPORT vtkRedistributeDataSetFilter : public vtkDataObjectAlgorithm
{
public:
  static vtkRedistributeDataSetFilter* New();
  vtkTypeMacro(vtkRedistributeDataSetFilter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Necessary to override this in order to take into account modifications to strategy
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Get/Set the controller to use. By default
   * vtkMultiProcessController::GlobalController will be used.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  enum BoundaryModes
  {
    ASSIGN_TO_ONE_REGION = 0,
    ASSIGN_TO_ALL_INTERSECTING_REGIONS = 1,
    SPLIT_BOUNDARY_CELLS = 2
  };

  ///@{
  /**
   * Specify how cells on the boundaries are handled.
   *
   * \li `ASSIGN_TO_ONE_REGION` results in a cell on the boundary uniquely added
   *      to one of the ranks containing the region intersecting the cell.
   * \li `ASSIGN_TO_ALL_INTERSECTING_REGIONS` results in a cell on the boundary
   *      added to all ranks containing the region intersecting the cell.
   * \li `SPLIT_BOUNDARY_CELLS` results in cells along the boundary being
   *      clipped along the region boundaries.
   *
   * Default is `ASSIGN_TO_ONE_REGION`.
   */
  vtkSetClampMacro(BoundaryMode, int, ASSIGN_TO_ONE_REGION, SPLIT_BOUNDARY_CELLS);
  vtkGetMacro(BoundaryMode, int);
  void SetBoundaryModeToAssignToOneRegion() { this->SetBoundaryMode(ASSIGN_TO_ONE_REGION); }
  void SetBoundaryModeToAssignToAllIntersectingRegions()
  {
    this->SetBoundaryMode(ASSIGN_TO_ALL_INTERSECTING_REGIONS);
  }
  void SetBoundaryModeToSplitBoundaryCells() { this->SetBoundaryMode(SPLIT_BOUNDARY_CELLS); }
  ///@}

  ///@{
  /**
   * Specify whether to compute the load balancing automatically or use
   * explicitly provided cuts. Set to false (default) to automatically compute
   * the cuts to use for redistributing the dataset.
   */
  void SetUseExplicitCuts(bool);
  bool GetUseExplicitCuts() const;
  vtkBooleanMacro(UseExplicitCuts, bool);
  ///@}

  ///@{
  /**
   * Specify the cuts to use when `UseExplicitCuts` is true.
   */
  void SetExplicitCuts(const std::vector<vtkBoundingBox>& boxes);
  const std::vector<vtkBoundingBox>& GetExplicitCuts() const;
  void RemoveAllExplicitCuts();
  void AddExplicitCut(const vtkBoundingBox& bbox);
  void AddExplicitCut(const double bbox[6]);
  int GetNumberOfExplicitCuts() const;
  const vtkBoundingBox& GetExplicitCut(int index) const;
  ///@}

  ///@{
  /**
   * Specify the DIY assigner used for distributing cuts. If you use this API, you have to be
   * careful and use an assigner matching your setup. For example, if you use explicit cuts (by
   * calling SetExplicitCuts()), you want to assign all the cuts you provide.
   */
  void SetAssigner(std::shared_ptr<diy::Assigner> assigner);
  std::shared_ptr<diy::Assigner> GetAssigner();
  std::shared_ptr<const diy::Assigner> GetAssigner() const;

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
  void SetExpandExplicitCuts(bool);
  bool GetExpandExplicitCuts() const;
  vtkBooleanMacro(ExpandExplicitCuts, bool);
  ///@}

  ///@{
  /**
   * Returns the cuts used by the most recent `RequestData` call. This is only
   * valid after a successful `Update` request.
   */
  const std::vector<vtkBoundingBox>& GetCuts() const;
  ///@}

  ///@{
  /**
   * Specify the number of partitions to split the input dataset into.
   * Set to -1 to indicate that the partitions should match the number of
   * ranks (processes) determined using vtkMultiProcessController provided.
   * Setting to a non-zero positive number will result in the filter generating at
   * least as many partitions.
   *
   * This is simply a hint and not an exact number of partitions the data will be
   * split into.
   *
   * Default is -1.
   *
   * @sa PreservePartitionsInOutput
   */
  void SetNumberOfPartitions(vtkIdType);
  vtkIdType GetNumberOfPartitions() const;
  ///@}

  ///@{
  /**
   * When set to true (default is false), this filter will generate a vtkPartitionedDataSet as the
   * output. The advantage of doing that is each partition that the input dataset was split
   * into can be individually accessed. Otherwise, when the number of partitions generated is
   * greater than the number of ranks, a rank with more than one partition will use
   * `vtkAppendFilter` to merge the multiple partitions into a single unstructured grid.
   *
   * The output dataset type is always vtkUnstructuredGrid when
   * PreservePartitionsInOutput is false and always a vtkPartitionedDataSet when
   * PreservePartitionsInOutput is true.
   *
   * Default is false i.e. the filter will generate a single vtkUnstructuredGrid.
   */
  vtkSetMacro(PreservePartitionsInOutput, bool);
  vtkGetMacro(PreservePartitionsInOutput, bool);
  vtkBooleanMacro(PreservePartitionsInOutput, bool);
  ///@}

  ///@{
  /**
   * Generate global cell ids if none present in the input. If global cell ids are present
   * in the input then this flag is ignored. Default is true.
   */
  vtkSetMacro(GenerateGlobalCellIds, bool);
  vtkGetMacro(GenerateGlobalCellIds, bool);
  vtkBooleanMacro(GenerateGlobalCellIds, bool);
  ///@}

  ///@{
  /**
   * Enable/disable debugging mode. In this mode internal arrays are preserved
   * and ghost cells are not explicitly marked as such so that they can be inspected
   * without risk of being dropped or removed by the pipeline.
   *
   * Default is false.
   */
  vtkSetMacro(EnableDebugging, bool);
  vtkGetMacro(EnableDebugging, bool);
  vtkBooleanMacro(EnableDebugging, bool);
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
  void SetLoadBalanceAcrossAllBlocks(bool);
  bool GetLoadBalanceAcrossAllBlocks();
  vtkBooleanMacro(LoadBalanceAcrossAllBlocks, bool);
  ///@}

  ///@{
  /**
   * Setter/Getter for Strategy
   */
  vtkPartitioningStrategy* GetStrategy();
  void SetStrategy(vtkPartitioningStrategy*);
  ///@}

protected:
  vtkRedistributeDataSetFilter();
  ~vtkRedistributeDataSetFilter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /*
   * A method with this signature used to exist. With the refactoring of this filter to accept
   * different partitioning strategies, this method no longer had any meaning in the generic
   * sense.
   *
   * If you inherited this filter and overrid this method, please implement a new partitioning
   * strategy instead.
   */
  // virtual vtkSmartPointer<vtkPartitionedDataSet> SplitDataSet(
  // vtkDataSet* dataset, const std::vector<vtkBoundingBox>& cuts);

private:
  vtkRedistributeDataSetFilter(const vtkRedistributeDataSetFilter&) = delete;
  void operator=(const vtkRedistributeDataSetFilter&) = delete;

  /**
   * This method is called to split a vtkDataSet into multiple datasets by the
   * vector of partition information passed in. The returned vtkPartitionedDataSet
   * must have exactly as many partitions as the number of information elements
   * in the `info` vector.
   *
   * Note, this method may duplicate cells that lie on the boundaries and add cell
   * arrays that indicate cell ownership and flag boundary cells.
   */
  virtual vtkSmartPointer<vtkPartitionedDataSet> SplitDataSet(
    vtkDataSet* dataset, const vtkPartitioningStrategy::PartitionInformation& info);

  bool Redistribute(vtkPartitionedDataSetCollection* inputCollection,
    vtkPartitionedDataSetCollection* outputCollection,
    const std::vector<vtkPartitioningStrategy::PartitionInformation>& info,
    bool preserve_input_hierarchy);

  bool RedistributePTD(vtkPartitionedDataSet*, vtkPartitionedDataSet*,
    const std::vector<vtkPartitioningStrategy::PartitionInformation>&, unsigned int*, vtkIdType*);

  bool RedistributeDataSet(vtkDataSet* inputDS, vtkPartitionedDataSet* outputPDS,
    const vtkPartitioningStrategy::PartitionInformation& info);
  vtkSmartPointer<vtkDataSet> ClipDataSet(vtkDataSet* dataset, const vtkBoundingBox& bbox);

  void MarkGhostCells(vtkPartitionedDataSet* pieces);

  vtkSmartPointer<vtkPartitionedDataSet> AssignGlobalCellIds(
    vtkPartitionedDataSet* input, vtkIdType* mb_offset = nullptr);
  vtkSmartPointer<vtkDataSet> AssignGlobalCellIds(
    vtkDataSet* input, vtkIdType* mb_offset = nullptr);

  void MarkValidDimensions(const vtkBoundingBox& gbounds);

  std::shared_ptr<diy::Assigner> Assigner;

  vtkMultiProcessController* Controller;
  int BoundaryMode;
  bool PreservePartitionsInOutput;
  bool GenerateGlobalCellIds;
  bool EnableDebugging;
  bool ValidDim[3];

  vtkSmartPointer<vtkPartitioningStrategy> Strategy;
};

VTK_ABI_NAMESPACE_END
#endif
