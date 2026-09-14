// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkKdTreePointLocator
 * @brief   class to quickly locate points in 2D and 3D
 *
 * vtkKdTreePointLocator is a vtkAbstractPointLocator that builds and
 * queries a kd-tree using the nanoflann library
 * (https://github.com/jlblancoc/nanoflann).
 *
 * Both 2D and 3D points are supported. When all the points are in a plane of
 * constant z (the z bounds are equal), a 2D kd-tree over the x and y
 * coordinates is built, which is never split on z. Its distances are then in
 * the x-y plane, as those of vtkStaticPointLocator2D: the z coordinate of the
 * query points, and of the petals, is ignored.
 *
 * The points are indexed with int when there are fewer than VTK_INT_MAX of
 * them, and with vtkIdType otherwise, as in vtkStaticPointLocator, which
 * halves the memory of the index of the tree.
 *
 * @par Thanks:
 * The kd-tree is provided by nanoflann:
 * Blanco, J.L. and Rai, P.K., nanoflann: a C++ header-only fork of FLANN, a
 * library for Nearest Neighbor (NN) with KD-trees, 2014,
 * https://github.com/jlblancoc/nanoflann
 *
 * @sa
 * vtkAbstractPointLocator vtkStaticPointLocator vtkStaticPointLocator2D
 */

#ifndef vtkKdTreePointLocator_h
#define vtkKdTreePointLocator_h

#include "vtkAbstractPointLocator.h"
#include "vtkCommonDataModelModule.h" // For export macro

#include <memory> // For std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;

class VTKCOMMONDATAMODEL_EXPORT vtkKdTreePointLocator : public vtkAbstractPointLocator
{
public:
  vtkTypeMacro(vtkKdTreePointLocator, vtkAbstractPointLocator);
  static vtkKdTreePointLocator* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Given a position x, return the id of the point closest to it. Alternative
   * method requires separate x-y-z values.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  vtkIdType FindClosestPoint(const double x[3]) override;

  /**
   * Given a position x and a radius r, return the id of the point
   * closest to the point in that radius.
   * dist2 returns the squared distance to the point.
   */
  vtkIdType FindClosestPointWithinRadius(double radius, const double x[3], double& dist2) override;

  /**
   * Find the closest N points to a position. This returns the closest
   * N points to a position. A faster method could be created that returned
   * N close points to a position, but necessarily the exact N closest.
   * The returned points are sorted from closest to farthest.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  void FindClosestNPoints(int N, const double x[3], vtkIdList* result) override;

  /**
   * Find all points within a specified radius R of position x.
   * The result is not sorted in any specific manner.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  void FindPointsWithinRadius(double R, const double x[3], vtkIdList* result) override;

  ///@{
  /**
   * Specify the maximum number of points a kd-tree leaf node may hold before
   * nanoflann stops subdividing it. Smaller values build a deeper tree
   * (more traversal, less per-leaf brute-force scanning); larger values do
   * the opposite. Defaults to nanoflann's own default of 10, which is tuned
   * for point-query performance.
   */
  vtkSetClampMacro(NumberOfPointsPerLeaf, int, 1, VTK_INT_MAX);
  vtkGetMacro(NumberOfPointsPerLeaf, int);
  ///@}

  ///@{
  /**
   * See vtkLocator interface documentation.
   * These methods are not thread safe.
   */
  void FreeSearchStructure() override;
  void BuildLocator() override;
  void ForceBuildLocator() override;
  void GenerateRepresentation(int level, vtkPolyData* pd) override;
  ///@}

  /**
   * Return the dimension of the kd-tree: 2 when all the points are in a plane
   * of constant z, and 3 otherwise (0 if the locator is not built). The
   * petals of FindNPointsInShell() and FindPointsInShell() have this
   * dimension plus one components: spheres (x,y,z,r2) of a 3D tree, circles
   * (x,y,r2) of a 2D one.
   */
  int GetDimension();

protected:
  vtkKdTreePointLocator();
  ~vtkKdTreePointLocator() override;

  void BuildLocatorInternal() override;

  int NumberOfPointsPerLeaf = 10;

  struct vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

private:
  vtkKdTreePointLocator(const vtkKdTreePointLocator&) = delete;
  void operator=(const vtkKdTreePointLocator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
