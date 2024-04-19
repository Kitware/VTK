// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkDIYKdTreeUtilities
 * @brief collection of utility functions for DIY-based KdTree algorithm
 *
 * vtkDIYKdTreeUtilities is intended for use by vtkRedistributeDataSetFilter. It
 * encapsulates invocation of DIY algorithms for various steps in the
 * vtkRedistributeDataSetFilter.
 */

#ifndef vtkDIYKdTreeUtilities_h
#define vtkDIYKdTreeUtilities_h

#include "vtkBoundingBox.h"               // for vtkBoundingBox
#include "vtkDIYExplicitAssigner.h"       // for vtkDIYExplicitAssigner
#include "vtkFiltersParallelDIY2Module.h" // for export macros
#include "vtkObject.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer

#include <memory> // for std::shared_ptr
#include <vector> // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;
class vtkDataSet;
class vtkIntArray;
class vtkMultiProcessController;
class vtkPartitionedDataSet;
class vtkPoints;
class vtkUnstructuredGrid;

class VTKFILTERSPARALLELDIY2_EXPORT vtkDIYKdTreeUtilities : public vtkObject
{
public:
  vtkTypeMacro(vtkDIYKdTreeUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Given a dataset (or a composite dataset), this method will generate box
   * cuts in the domain to approximately load balance the points (or
   * cell-centers) into `number_of_partitions` requested. If `controller` is non-null,
   * the operation will be performed taking points on the multiple ranks into consideration.
   *
   * `local_bounds` provides the local domain bounds. If not specified, domain
   * bounds will be computed using the `dobj`.

   * Returns a vector a bounding boxes that can be used to partition the points
   * into load balanced chunks. The size of the vector is greater than or equal
   * to the `number_of_partitions`.
   */
  static std::vector<vtkBoundingBox> GenerateCuts(vtkDataObject* dobj, int number_of_partitions,
    bool use_cell_centers, vtkMultiProcessController* controller = nullptr,
    const double* local_bounds = nullptr);

  /**
   * Another variant to GenerateCuts that simply takes in a vector of
   * dataobjects, each can be a dataset or a composite dataset.
   */
  static std::vector<vtkBoundingBox> GenerateCuts(const std::vector<vtkDataObject*>& dobjs,
    int number_of_partitions, bool use_cell_centers,
    vtkMultiProcessController* controller = nullptr, const double* local_bounds = nullptr);

  /**
   * Given a collection of points, this method will generate box cuts in the
   * domain to approximately load balance the points into `number_of_partitions`
   * requested. If `controller` is non-null, the operation will be performed
   * taking points on the multiple ranks into consideration.
   *
   * `local_bounds` provides the local domain bounds. If not specified, domain
   * bounds will be computed using the points provided.
   *
   * Returns a vector a bounding boxes that can be used to partition the points
   * into load balanced chunks. The size of the vector is greater than or equal
   * to the `number_of_partitions`.
   */
  static std::vector<vtkBoundingBox> GenerateCuts(
    const std::vector<vtkSmartPointer<vtkPoints>>& points, int number_of_partitions,
    vtkMultiProcessController* controller = nullptr, const double* local_bounds = nullptr);

  /**
   * Exchange parts in the partitioned dataset among ranks in the parallel group
   * defined by the `controller`. The parts are assigned to ranks in a
   * contiguous fashion.
   *
   * To determine which partition in the `parts` is targeted for which ranks,
   * the `block_assigner` is used, if specified. If not specified, an assigner
   * will be created internally using the following rules. If the number of
   * partitions is a power of two, then `vtkDIYKdTreeUtilities::CreateAssigner`
   * is used otherwise a `diy::ContiguousAssigner` is created.
   *
   * The returned vtkPartitionedDataSet will also have exactly as many
   * partitions as the input vtkPartitionedDataSet, however only the partitions
   * assigned to this current rank may be non-null.
   *
   * block_assigner is an optional parameter that should be set if the user wants
   * to assign blocks in a custom way. The default assigner is the one returned
   * by vtkDIYKdTreeUtilities::CreateAssigner.
   */
  static vtkSmartPointer<vtkPartitionedDataSet> Exchange(vtkPartitionedDataSet* parts,
    vtkMultiProcessController* controller, std::shared_ptr<diy::Assigner> block_assigner = nullptr);

  /**
   * Generates and adds global cell ids to datasets in `parts`. One this to note
   * that this method does not assign valid global ids to ghost cells. This may
   * not be adequate for general use, however for vtkRedistributeDataSetFilter
   * this is okay since the ghost cells in the input are anyways discarded when
   * the dataset is being split based on the cuts provided. This simplifies the
   * implementation and reduces communication.
   */
  static bool GenerateGlobalCellIds(vtkPartitionedDataSet* parts,
    vtkMultiProcessController* controller, vtkIdType* mb_offset = nullptr);

  /**
   * `GenerateCuts` returns a kd-tree with power of 2 nodes. Oftentimes, we want
   * to generate rank assignments for a fewer number of ranks for the nodes such
   * that each rank gets assigned a complete sub-tree. Use this function to
   * generate such an assignment.  This has following constraints:
   * 1. `num_blocks` must be a power of two.
   * 2. `num_ranks` cannot be greater than num_blocks.
   */
  static std::vector<int> ComputeAssignments(int num_blocks, int num_ranks);

  /**
   * Returns an assigner that assigns power-of-two blocks to an arbitrary number
   * of ranks such that each rank with a non-empty assignment gets a subtree --
   * thus preserving the kd-tree ordering between ranks.
   */
  static vtkDIYExplicitAssigner CreateAssigner(diy::mpi::communicator& comm, int num_blocks);

  /**
   * `GenerateCuts` returns a kd-tree with power of 2 nodes. Use this function
   * to resize the cuts to lower number while still preserving the kd-tree. This
   * is done by merging leaf nodes till the requested size is reached. If `size`
   * is negative or greater than then number of `cuts.size()`, then this
   * function does nothing. Otherwise when the function returns, `cuts.size() ==
   * size`.
   */
  static void ResizeCuts(std::vector<vtkBoundingBox>& cuts, int size);

protected:
  vtkDIYKdTreeUtilities();
  ~vtkDIYKdTreeUtilities() override;

private:
  vtkDIYKdTreeUtilities(const vtkDIYKdTreeUtilities&) = delete;
  void operator=(const vtkDIYKdTreeUtilities&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
