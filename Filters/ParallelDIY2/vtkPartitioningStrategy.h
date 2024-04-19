// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPartitioningStrategy
 * @brief A strategy interface for partitioning meshes
 *
 * This abstract class is meant to act as an interface for homogeneizing the use of different
 * partitioning algorithms in the vtkRedistributeDataSetFilter.
 *
 * This class offers two main architectural contributions:
 *
 * 1) Format: a vtkPartitioningStrategy::PartitionInformation structure that defines a protocol for
 * submitting partition information to the vtkRedistributeDataSetFilter for subsequent communication
 * and process dispatch
 *
 * 2) Processing Signature: a pure virtual method/interface
 * vtkPartitioningStrategy::ComputePartition for implementing partitioning algorithms and providing
 * std::vectors of PartitionInformation (one for each current partition in the
 * vtkPartitionedDataSetCollection) to the vtkRedistributeDataSetFilter
 *
 * @sa
 * vtkRedistributeDataSetFilter
 */

#ifndef vtkPartitioningStrategy_h
#define vtkPartitioningStrategy_h

#include "vtkFiltersParallelDIY2Module.h" // for export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h" // for member variables

VTK_ABI_NAMESPACE_BEGIN
class vtkIdTypeArray;
class vtkMultiProcessController;
class vtkPartitionedDataSetCollection;
class VTKFILTERSPARALLELDIY2_EXPORT vtkPartitioningStrategy : public vtkObject
{
public:
  vtkTypeMacro(vtkPartitioningStrategy, vtkObject);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  /**
   * An enum defining which principal entity is being partitioned
   */
  enum PartitionedEntity
  {
    POINTS = 0,
    CELLS = 1
  };

  /**
   * \struct PartitionInformation
   * \brief Encapsulation structure for describing the result of a partitioning calculation
   *
   * This structures defines a format for transmitting partition information from the
   * partitioning algorithm to the communication and dispatching phase of the
   * vtkRedistributeDataSetFilter. Its elements are:
   * - TargetEntity: an enum signaling which principal entities are being partitioned by the
   * partitioning algorithm
   * - TargetPartitions: the main structure containing the partitioning information result of the
   * algorithm
   * - BoundaryNeighborPartitions: an array of pairs augmenting the TargetPartitions information
   * with partition boundary information
   * - NumberOfPartitions: the total number of partitions for the data set
   *
   * The TargetPartitions array is an array with as many tuples as there are entities in the data
   * set and only 1 component. Its value denotes the rank which owns a given entity in the
   * calculated partition. For example, if no redistribution is required, than the TargetPartitions
   * array has constant value equal to the local process rank. The BoundaryNeighborPartitions is an
   * array with as many tuples as there are entities at the boundaries of the new partitions locally
   * and 2 components. Each tuple is thus an (entity index, process index) pair describing whether a
   * given entity lies adjacent to a partition boundary and therefore might be included in some
   * ghost information during communication and dispatching.
   */
  struct PartitionInformation
  {
  public:
    /**
     * Principal entity on which the partitioning is defined
     */
    PartitionedEntity TargetEntity = CELLS;
    /**
     * An array defining on which rank each currently local principal entity should be shipped to
     */
    vtkNew<vtkIdTypeArray> TargetPartitions;
    /**
     * A 2 component array defining boundary entity indexes and their neighbor partition index
     */
    vtkNew<vtkIdTypeArray> BoundaryNeighborPartitions;
    /**
     * The total number of partitions
     */
    vtkIdType NumberOfPartitions = 0;
  };

  /**
   * Main method for subclasses to implement in order to define their partitioning method (should be
   * called on all ranks due to distributed communication)
   *
   * Returns a vector of PartitionInformation structures (one for each block partition data set in
   * the collection)
   */
  virtual std::vector<PartitionInformation> ComputePartition(vtkPartitionedDataSetCollection*) = 0;

  ///@{
  /**
   * Get/Set the controller to use. By default
   * vtkMultiProcessController::GlobalController will be used.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  ///@{
  /**
   * Get/Set number of partitions (if < 0 use number of MPI ranks)
   */
  vtkGetMacro(NumberOfPartitions, vtkIdType);
  vtkSetMacro(NumberOfPartitions, vtkIdType);
  ///@}

protected:
  vtkPartitioningStrategy();
  ~vtkPartitioningStrategy() override;

  vtkMultiProcessController* Controller = nullptr;

  vtkIdType NumberOfPartitions = -1;

private:
  vtkPartitioningStrategy(const vtkPartitioningStrategy&) = delete;
  void operator=(const vtkPartitioningStrategy&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif // vtkPartitioningStrategy_h
