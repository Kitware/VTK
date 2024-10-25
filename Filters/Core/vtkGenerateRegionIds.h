// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkGenerateRegionIds
 * @brief Generate cell array containing the region id.
 *
 * vtkGenerateRegionIds parse the input 2D mesh to compute regions information.
 * It generates a cell array containing the region id the cell belongs to.
 * In that, it has some similiraty with vtkConnectivityFilter.
 *
 * A region is defined as a collection of connected 2D cells
 * with a maximal angle between their normals. Two cells are connected
 * if they share at least one point.
 *
 * The algorithm is similar to vtkFeatureEdges:
 * For a pair of neighboring cells, it computes their relative angle and compare it to a threshold
 * (vtkFeatureEdges::FeatureAngle or vtkGenerateRegionIds::MaxAngle).
 * Then vtkGenerateRegionIds uses a "below" criteria to assign the same Id in a Cell Array,
 * while vtkFeatureEdges uses an "above" criteria to generate edges and a new polydata.
 *
 * You can also see a Region as a Surface delimited by Feature Edges.
 *
 * @note: vtkGenerateRegionIds requires cell Normals in order to work.
 * If not provided, the Normals array will be computed and added to the dataset.
 *
 * @see vtkFeatureEdges, vtkConnectivityFilter
 */

#ifndef vtkGenerateRegionIds_h
#define vtkGenerateRegionIds_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include <set>    // for std::set
#include <string> // for std::string

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSCORE_EXPORT vtkGenerateRegionIds : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkGenerateRegionIds, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkGenerateRegionIds* New();

  ///@{
  /**
   * Set / Get the threshold angle (in degree) for region detection.
   * Given 2 edge-connected cells, they are considered on the same region
   * if the angle between their normals is lower or equal to this value.
   *
   * Default is 30 degrees.
   */
  vtkSetClampMacro(MaxAngle, double, 0, VTK_DOUBLE_MAX);
  vtkGetMacro(MaxAngle, double);
  ///@}

  ///@{
  /**
   * Set / Get the region Ids array name.
   * Default is vtkRegionIds.
   */
  vtkSetMacro(RegionIdsArrayName, std::string);
  vtkGetMacro(RegionIdsArrayName, std::string);
  ///@}

protected:
  vtkGenerateRegionIds() = default;
  ~vtkGenerateRegionIds() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkGenerateRegionIds(const vtkGenerateRegionIds&) = delete;
  void operator=(const vtkGenerateRegionIds&) = delete;

  /**
   * ShallowCopy input in output.
   * Add Cell Normals.
   * Add vtkregionIds array, initialized with -1.
   * Return the new regionIds array.
   */
  vtkIdTypeArray* InitializeOutput(vtkPolyData* input, vtkPolyData* output);

  /**
   * Return true if the cells normal angle cosinus is greater than threshold.
   */
  bool SameRegion(vtkDataArray* normals, double threshold, vtkIdType first, vtkIdType second);

  /**
   * Return a set of neighbors for given cell.
   * Neighbors are defined as every cell that share at least one point.
   */
  std::set<vtkIdType> GetCellNeighbors(vtkPolyData* polydata, vtkIdType cellId);

  double MaxAngle = 30;
  std::string RegionIdsArrayName = "vtkRegionIds";
};

VTK_ABI_NAMESPACE_END
#endif
