/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRedistributeDataSetFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkRedistributeDataSetFilter
 * @brief redistributes input dataset into requested number of partitions
 *
 * vtkRedistributeDataSetFilter is intended for redistributing data in a load
 * balanced fashion. The load balancing attempts to balance the number of cells
 * per target partition approximately. It uses a DIY-based kdtree implementation
 * that builds balances the cell centers among requested number of partitions.
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
 * Besides redistributing the data, the filter can optionally generate global
 * cell ids. This is provided since it relative easy to generate these
 * on when it is known that the data is spatially partitioned as is the case
 * after this filter has executed.
 *
 */
#ifndef vtkRedistributeDataSetFilter_h
#define vtkRedistributeDataSetFilter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersParallelDIY2Module.h" // for export macros
#include "vtkSmartPointer.h"              // for vtkSmartPointer

#include <memory> // for std::shared_ptr
#include <vector> // for std::vector

// clang-format off
#include "vtk_diy2.h"
#include VTK_DIY2(diy/assigner.hpp)
// clang-format on

class vtkMultiProcessController;
class vtkBoundingBox;
class vtkPartitionedDataSet;
class vtkMultiBlockDataSet;
class vtkMultiPieceDataSet;

class VTKFILTERSPARALLELDIY2_EXPORT vtkRedistributeDataSetFilter : public vtkDataObjectAlgorithm
{
public:
  static vtkRedistributeDataSetFilter* New();
  vtkTypeMacro(vtkRedistributeDataSetFilter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the controller to use. By default
   * vtkMultiProcessController::GlobalController will be used.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  enum BoundaryModes
  {
    ASSIGN_TO_ONE_REGION = 0,
    ASSIGN_TO_ALL_INTERSECTING_REGIONS = 1,
    SPLIT_BOUNDARY_CELLS = 2
  };

  //@{
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
  //@}

  //@{
  /**
   * Specify whether to compute the load balancing automatically or use
   * explicitly provided cuts. Set to false (default) to automatically compute
   * the cuts to use for redistributing the dataset.
   */
  vtkSetMacro(UseExplicitCuts, bool);
  vtkGetMacro(UseExplicitCuts, bool);
  vtkBooleanMacro(UseExplicitCuts, bool);
  //@}

  //@{
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
  //@}

  //@{
  /**
   * Specify the DIY assigner used for distributing cuts. If you use this API, you have to be
   * careful and use an assigner matching your setup. For example, if you use explicit cuts (by
   * calling SetExplicitCuts()), you want to assign all the cuts you provide.
   */
  void SetAssigner(std::shared_ptr<diy::Assigner> assigner);
  std::shared_ptr<diy::Assigner> GetAssigner();
  std::shared_ptr<const diy::Assigner> GetAssigner() const;

  //@{
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
  //@}

  //@}
  /**
   * Returns the cuts used by the most recent `RequestData` call. This is only
   * valid after a successful `Update` request.
   */
  const std::vector<vtkBoundingBox>& GetCuts() const { return this->Cuts; }

  //@{
  /**
   * Specify the number of partitions to split the input dataset into.
   * Set to 0 to indicate that the partitions should match the number of
   * ranks (processes) determined using vtkMultiProcessController provided.
   * Setting to a non-zero positive number will result in the filter generating at
   * least as many partitions.
   *
   * This is simply a hint and not an exact number of partitions the data will be
   * split into. Current implementation results in number of partitions equal to
   * the power of 2 greater than or equal to the chosen value.
   *
   * Default is 0.
   *
   * This has no effect when `UseExplicitCuts` is set to true. In that case, the
   * number of partitions is dictated by the number of cuts provided.
   *
   * @sa PreservePartitionsInOutput, UseExplicitCuts
   */
  vtkSetClampMacro(NumberOfPartitions, int, 0, VTK_INT_MAX);
  vtkGetMacro(NumberOfPartitions, int);
  //@}

  //@{
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
  //@}

  //@{
  /**
   * Generate global cell ids if none present in the input. If global cell ids are present
   * in the input then this flag is ignored. Default is true.
   */
  vtkSetMacro(GenerateGlobalCellIds, bool);
  vtkGetMacro(GenerateGlobalCellIds, bool);
  vtkBooleanMacro(GenerateGlobalCellIds, bool);
  //@}

  /**
   * Helper function to expand a collection of bounding boxes to include the
   * `bounds` specified. This will expand any boxes in the `cuts` that abut any
   * of the external faces of the bounding box formed by all the `cuts` to
   * touch the external faces of the `bounds`.
   */
  std::vector<vtkBoundingBox> ExpandCuts(
    const std::vector<vtkBoundingBox>& cuts, const vtkBoundingBox& bounds);

  //@{
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
  //@}

protected:
  vtkRedistributeDataSetFilter();
  ~vtkRedistributeDataSetFilter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

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

  /**
   * This method is called to split a vtkDataSet into multiple datasets by the
   * vector of `vtkBoundingBox` passed in. The returned vtkPartitionedDataSet
   * must have exactly as many partitions as the number of vtkBoundingBoxes
   * in the `cuts` vector with each partition matching the bounding box at the
   * matching index.
   *
   * Note, this method duplicates cells that lie on the boundaries and adds cell
   * arrays that indicate cell ownership and flags boundary cells.
   */
  virtual vtkSmartPointer<vtkPartitionedDataSet> SplitDataSet(
    vtkDataSet* dataset, const std::vector<vtkBoundingBox>& cuts);

private:
  vtkRedistributeDataSetFilter(const vtkRedistributeDataSetFilter&) = delete;
  void operator=(const vtkRedistributeDataSetFilter&) = delete;

  bool Redistribute(vtkDataObject* inputDO, vtkPartitionedDataSet* outputPDS,
    const std::vector<vtkBoundingBox>& cuts, vtkIdType* mb_offset = nullptr);
  bool RedistributeDataSet(
    vtkDataSet* inputDS, vtkPartitionedDataSet* outputPDS, const std::vector<vtkBoundingBox>& cuts);
  int RedistributeMultiBlockDataSet(
    vtkMultiBlockDataSet* input, vtkMultiBlockDataSet* output, vtkIdType* mb_offset = nullptr);
  int RedistributeMultiPieceDataSet(
    vtkMultiPieceDataSet* input, vtkMultiPieceDataSet* output, vtkIdType* mb_offset = nullptr);
  vtkSmartPointer<vtkDataSet> ClipDataSet(vtkDataSet* dataset, const vtkBoundingBox& bbox);

  void MarkGhostCells(vtkPartitionedDataSet* pieces);

  vtkSmartPointer<vtkPartitionedDataSet> AssignGlobalCellIds(
    vtkPartitionedDataSet* input, vtkIdType* mb_offset = nullptr);
  vtkSmartPointer<vtkDataSet> AssignGlobalCellIds(
    vtkDataSet* input, vtkIdType* mb_offset = nullptr);

  void MarkValidDimensions(vtkDataObject* inputDO);

  std::vector<vtkBoundingBox> ExplicitCuts;
  std::vector<vtkBoundingBox> Cuts;
  std::shared_ptr<diy::Assigner> Assigner;

  vtkMultiProcessController* Controller;
  int BoundaryMode;
  int NumberOfPartitions;
  bool PreservePartitionsInOutput;
  bool GenerateGlobalCellIds;
  bool UseExplicitCuts;
  bool ExpandExplicitCuts;
  bool EnableDebugging;
  bool ValidDim[3];
};

#endif
