// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtentRCBPartitioner
 * @brief   This method partitions a global extent to N partitions where N is a user
 *  supplied parameter.
 */

#ifndef vtkExtentRCBPartitioner_h
#define vtkExtentRCBPartitioner_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkObject.h"
#include <cassert> // For assert
#include <string>  // For std::string
#include <vector>  // For STL vector

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONEXECUTIONMODEL_EXPORT vtkExtentRCBPartitioner : public vtkObject
{
public:
  static vtkExtentRCBPartitioner* New();
  vtkTypeMacro(vtkExtentRCBPartitioner, vtkObject);
  void PrintSelf(ostream& oss, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the number of requested partitions
   */
  void SetNumberOfPartitions(int N)
  {
    assert("pre: Number of partitions requested must be > 0" && (N >= 0));
    this->Reset();
    this->NumberOfPartitions = N;
  }
  ///@}

  ///@{
  /**
   * Set/Get the global extent array to be partitioned.
   * The global extent is packed as follows:
   * [imin,imax,jmin,jmax,kmin,kmax]
   */
  void SetGlobalExtent(int imin, int imax, int jmin, int jmax, int kmin, int kmax)
  {
    this->Reset();
    this->GlobalExtent[0] = imin;
    this->GlobalExtent[1] = imax;
    this->GlobalExtent[2] = jmin;
    this->GlobalExtent[3] = jmax;
    this->GlobalExtent[4] = kmin;
    this->GlobalExtent[5] = kmax;
  }
  void SetGlobalExtent(int ext[6])
  {
    this->SetGlobalExtent(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
  }
  ///@}

  ///@{
  /**
   * On/Off DuplicateNodes between partitions. Default is On.
   */
  vtkSetMacro(DuplicateNodes, vtkTypeBool);
  vtkGetMacro(DuplicateNodes, vtkTypeBool);
  vtkBooleanMacro(DuplicateNodes, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get macro for the number of ghost layers.
   */
  vtkSetMacro(NumberOfGhostLayers, int);
  vtkGetMacro(NumberOfGhostLayers, int);
  ///@}

  ///@{
  /**
   * Returns the number of extents.
   */
  vtkGetMacro(NumExtents, int);
  ///@}

  /**
   * Partitions the extent
   */
  void Partition();

  /**
   * Returns the extent of the partition corresponding to the given ID.
   */
  void GetPartitionExtent(int idx, int ext[6]);

protected:
  vtkExtentRCBPartitioner();
  ~vtkExtentRCBPartitioner() override;

  ///@{
  /**
   * Resets the partitioner to the initial state, all previous partition
   * extents are cleared.
   */
  void Reset()
  {
    this->PartitionExtents.clear();
    this->NumExtents = 0;
    this->ExtentIsPartitioned = false;
  }
  ///@}

  /**
   * Given an extent, this method will create ghost layers on each side of
   * the boundary in each dimension. The ghost layers however will be
   * restricted to the given global extent.
   */
  void ExtendGhostLayers(int ext[6]);

  ///@{
  /**
   * Givent an extent and the min/max of the dimension we are looking at, this
   * method will produce a ghosted extent which is clamped within the given
   * global extent
   */
  void GetGhostedExtent(int ext[6], int minIdx, int maxIdx)
  {
    ext[minIdx] -= this->NumberOfGhostLayers;
    ext[maxIdx] += this->NumberOfGhostLayers;
    ext[minIdx] =
      (ext[minIdx] < this->GlobalExtent[minIdx]) ? this->GlobalExtent[minIdx] : ext[minIdx];
    ext[maxIdx] =
      (ext[maxIdx] > this->GlobalExtent[maxIdx]) ? this->GlobalExtent[maxIdx] : ext[maxIdx];
  }
  ///@}

  /**
   * Gets the structured data-description based on the givenn global extent
   */
  void AcquireDataDescription();

  /**
   * Returns the extent at the position corresponding to idx.
   */
  void GetExtent(int idx, int ext[6]);

  /**
   * Adds the extent to the end of the list of partitioned extents
   */
  void AddExtent(int ext[6]);

  /**
   * Replaces the extent at the position indicated by idx with the provided
   * extent.
   */
  void ReplaceExtent(int idx, int ext[6]);

  /**
   * Splits the extent along the given dimension.
   */
  void SplitExtent(int parent[6], int s1[6], int s2[6], int splitDimension);

  /**
   * Returns the total number of extents. It's always the 2^N where
   * N is the number of subdivisions.
   */
  int GetNumberOfTotalExtents();

  /**
   * Computes the total number of nodes for the given structured grid extent
   */
  int GetNumberOfNodes(int ext[6]);

  /**
   * Computes the total number of cells for the given structured grid extent
   */
  int GetNumberOfCells(int ext[6]);

  /**
   * Returns the length of the longest dimension
   */
  int GetLongestDimensionLength(int ext[6]);

  /**
   * Returns the longest edge
   */
  int GetLongestDimension(int ext[6]);

  /**
   * A convenience method for debugging purposes.
   */
  void PrintExtent(const std::string& name, int ext[6]);

  int NumberOfGhostLayers;
  int DataDescription;
  int GlobalExtent[6];
  int NumberOfPartitions;
  int NumExtents;

  vtkTypeBool DuplicateNodes; // indicates whether nodes are duplicated between
                              // partitions, so that they are abutting. This is
                              // set to true by default. If disabled, the resulting
                              // partitions will have gaps.

  bool ExtentIsPartitioned;

  std::vector<int> PartitionExtents;

private:
  vtkExtentRCBPartitioner(const vtkExtentRCBPartitioner&) = delete;
  void operator=(const vtkExtentRCBPartitioner&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* VTKEXTENTRCBPARTITIONER_H_ */
