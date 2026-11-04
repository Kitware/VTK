// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPResolveHangingNodes
 * @brief   resolve the hanging nodes of a 2D or 3D multi-resolution mesh distributed over
 * several processes
 *
 * vtkPResolveHangingNodes is the parallel version of vtkResolveHangingNodes. Each partition,
 * i.e., the data of a process or a partition of a vtkPartitionedDataSet, is resolved between its
 * own cells. At the boundary of a partition, the finer cells that border a coarse cell can be in
 * another partition, so the points that hang on the edges or faces of the coarse cell are not known
 * locally. The partitions of all the processes exchange the points of their own boundary, with one
 * diy block per partition: the end points of the edges or faces without a neighbor that remain
 * after the local resolution. The partitions whose boundary bounds overlap are neighbors, and they
 * share which bins of their bounds their boundary edges or faces occupy (see
 * vtkResolveHangingNodes::BoundaryBins), so each partition receives only the points of its
 * neighbors that are near its own boundary. The received points that lie on an edge or face are
 * inserted in the cell, and added, with their point data, to the points of the output. As in
 * vtkResolveHangingNodes, the partitions of a vtkPartitionedDataSet are resolved together, every
 * vtkPartitionedDataSet of a collection on its own, and a block of a vtkNonOverlappingAMR only with
 * the blocks of the other levels, across the processes.
 *
 * The coordinates of these points are copied, so both sides of the interface between two
 * partitions end up with the same points along it, with the same point data, and ghost cells can
 * be generated afterwards (see vtkGhostCellsGenerator). The numbers of levels computed by all the
 * processes are the same, including on the processes without cells, and the new global ids of the
 * center points of the 3D decomposition are unique over all the processes.
 *
 * All the partitions must have the same point data arrays. With a single process, this filter does
 * the same as vtkResolveHangingNodes. It overrides vtkResolveHangingNodes in the object factory
 * when this module is loaded.
 *
 * @sa vtkResolveHangingNodes vtkGhostCellsGenerator
 */

#ifndef vtkPResolveHangingNodes_h
#define vtkPResolveHangingNodes_h

#include "vtkFiltersParallelDIY2Module.h" // For export macro
#include "vtkResolveHangingNodes.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKFILTERSPARALLELDIY2_EXPORT vtkPResolveHangingNodes : public vtkResolveHangingNodes
{
public:
  static vtkPResolveHangingNodes* New();
  vtkTypeMacro(vtkPResolveHangingNodes, vtkResolveHangingNodes);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * By default this filter uses the global controller,
   * but this method can be used to set another instead.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

protected:
  vtkPResolveHangingNodes();
  ~vtkPResolveHangingNodes() override;

  /**
   * True if there is more than one process.
   */
  bool UseRemotePoints() override;

  /**
   * Reduce the range of the cell measures (areas for 2D, volumes for 3D) and the dimension of the
   * cells over all the processes.
   */
  void GetGlobalMeasureRange(double range[2], int& dimension) override;

  /**
   * Exchange the points of the boundary of the partitions with their neighbors, on this process
   * or on others, with one diy block per partition, see the class documentation. The neighbors are
   * all the other partitions for a partition whose `partitionedDataSetIds` is -1, or only the
   * partitions of the other vtkPartitionedDataSet, see vtkResolveHangingNodes::GetRemotePoints().
   * With a single process, the partitions exchange them as in vtkResolveHangingNodes.
   */
  std::vector<vtkSmartPointer<vtkPolyData>> GetRemotePoints(
    const std::vector<vtkPolyData*>& boundaries,
    const std::vector<int>& partitionedDataSetIds) override;

  /**
   * Reduce the largest point global id over all the processes, and sum the numbers of center points
   * of the processes of lower rank, and of all the processes, see
   * vtkResolveHangingNodes::GetCentroidGlobalIdsOffset().
   */
  void GetCentroidGlobalIdsOffset(vtkIdType& maxGlobalId, vtkIdType numberOfCentroidPoints,
    vtkIdType& offset, vtkIdType& totalNumberOfCentroidPoints) override;

private:
  vtkPResolveHangingNodes(const vtkPResolveHangingNodes&) = delete;
  void operator=(const vtkPResolveHangingNodes&) = delete;

  vtkMultiProcessController* Controller = nullptr;
};

VTK_ABI_NAMESPACE_END
#endif
