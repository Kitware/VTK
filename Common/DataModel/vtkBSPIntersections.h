// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkBSPIntersections
 * @brief   Perform calculations (mostly intersection
 *   calculations) on regions of a 3D binary spatial partitioning.
 *
 *
 *    Given an axis aligned binary spatial partitioning described by a
 *    vtkBSPCuts object, perform intersection queries on various
 *    geometric entities with regions of the spatial partitioning.
 *
 * @sa
 *    vtkBSPCuts  vtkKdTree
 */

#ifndef vtkBSPIntersections_h
#define vtkBSPIntersections_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkTimeStamp;
class vtkCell;
class vtkKdNode;
class vtkBSPCuts;

class VTKCOMMONDATAMODEL_EXPORT vtkBSPIntersections : public vtkObject
{
public:
  vtkTypeMacro(vtkBSPIntersections, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkBSPIntersections* New();

  /**
   * Define the binary spatial partitioning.
   */

  void SetCuts(vtkBSPCuts* cuts);
  vtkGetObjectMacro(Cuts, vtkBSPCuts);

  /**
   * Get the bounds of the whole space (xmin, xmax, ymin, ymax, zmin, zmax)
   * Return 0 if OK, 1 on error.
   */

  int GetBounds(double* bounds);

  /**
   * The number of regions in the binary spatial partitioning
   */

  int GetNumberOfRegions();

  /**
   * Get the spatial bounds of a particular region
   * Return 0 if OK, 1 on error.
   */

  int GetRegionBounds(int regionID, double bounds[6]);

  /**
   * Get the bounds of the data within the k-d tree region, possibly
   * smaller than the bounds of the region.
   * Return 0 if OK, 1 on error.
   */

  int GetRegionDataBounds(int regionID, double bounds[6]);

  ///@{
  /**
   * Determine whether a region of the spatial decomposition
   * intersects an axis aligned box.
   */
  int IntersectsBox(int regionId, double* x);
  int IntersectsBox(int regionId, double x0, double x1, double y0, double y1, double z0, double z1);
  ///@}

  ///@{
  /**
   * Compute a list of the Ids of all regions that
   * intersect the specified axis aligned box.
   * Returns: the number of ids in the list.
   */
  int IntersectsBox(int* ids, int len, double* x);
  int IntersectsBox(
    int* ids, int len, double x0, double x1, double y0, double y1, double z0, double z1);
  ///@}

  /**
   * Determine whether a region of the spatial decomposition
   * intersects a sphere, given the center of the sphere
   * and the square of it's radius.
   */
  int IntersectsSphere2(int regionId, double x, double y, double z, double rSquared);

  /**
   * Compute a list of the Ids of all regions that
   * intersect the specified sphere.  The sphere is given
   * by it's center and the square of it's radius.
   * Returns: the number of ids in the list.
   */
  int IntersectsSphere2(int* ids, int len, double x, double y, double z, double rSquared);

  /**
   * Determine whether a region of the spatial decomposition
   * intersects the given cell.  If you already
   * know the region that the cell centroid lies in, provide
   * that as the last argument to make the computation quicker.
   */
  int IntersectsCell(int regionId, vtkCell* cell, int cellRegion = -1);

  /**
   * Compute a list of the Ids of all regions that
   * intersect the given cell.  If you already
   * know the region that the cell centroid lies in, provide
   * that as the last argument to make the computation quicker.
   * Returns the number of regions the cell intersects.
   */
  int IntersectsCell(int* ids, int len, vtkCell* cell, int cellRegion = -1);

  /**
   * When computing the intersection of k-d tree regions with other
   * objects, we use the spatial bounds of the region.  To use the
   * tighter bound of the bounding box of the data within the region,
   * set this variable ON.  (Specifying data bounds in the vtkBSPCuts
   * object is optional.  If data bounds were not specified, this
   * option has no meaning.)
   */

  vtkGetMacro(ComputeIntersectionsUsingDataBounds, int);
  void SetComputeIntersectionsUsingDataBounds(int c);
  void ComputeIntersectionsUsingDataBoundsOn();
  void ComputeIntersectionsUsingDataBoundsOff();

protected:
  vtkBSPIntersections();
  ~vtkBSPIntersections() override;

  vtkGetMacro(RegionListBuildTime, vtkMTimeType);

  int BuildRegionList();

  vtkKdNode** GetRegionList() { return this->RegionList; }

  double CellBoundsCache[6]; // to speed cell intersection queries

  enum
  {
    XDIM = 0, // don't change these values
    YDIM = 1,
    ZDIM = 2
  };

private:
  static int NumberOfLeafNodes(vtkKdNode* kd);
  static void SetIDRanges(vtkKdNode* kd, int& min, int& max);

  int SelfRegister(vtkKdNode* kd);

  static void SetCellBounds(vtkCell* cell, double* bounds);

  int IntersectsBox_(vtkKdNode* node, int* ids, int len, double x0, double x1, double y0, double y1,
    double z0, double z1);

  int IntersectsSphere2_(
    vtkKdNode* node, int* ids, int len, double x, double y, double z, double rSquared);

  int IntersectsCell_(vtkKdNode* node, int* ids, int len, vtkCell* cell, int cellRegion = -1);

  vtkBSPCuts* Cuts;

  int NumberOfRegions;
  vtkKdNode** RegionList;

  vtkTimeStamp RegionListBuildTime;

  int ComputeIntersectionsUsingDataBounds;

  vtkBSPIntersections(const vtkBSPIntersections&) = delete;
  void operator=(const vtkBSPIntersections&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
