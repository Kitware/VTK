// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAbstractPointLocator
 * @brief   abstract class to quickly locate points in 3-space
 *
 * vtkAbstractPointLocator is an abstract spatial search object to quickly locate points
 * in 3D. vtkAbstractPointLocator works by dividing a specified region of space into
 * "rectangular" buckets, and then keeping a list of points that
 * lie in each bucket. Typical operation involves giving a position in 3D
 * and finding the closest point.  The points are provided from the specified
 * dataset input.
 *
 * The locators also support the shell queries (FindPointsInShell() and
 * FindNPointsInShell()), optionally restricted to the petals of a Voronoi
 * flower, which drive the point insertion of the Voronoi filters. The Voronoi
 * flower and its petals are described in:
 * W. Schroeder, D. Thompson, S. Tsalikis. A Parallel Meshless Voronoi Method
 * for Generalized SurfaceNets. IEEE Transactions on Visualization and Computer
 * Graphics, vol. 32, no. 7, pp. 7118-7130, 2026. DOI: 10.1109/TVCG.2026.3694501.
 *
 * @sa
 * vtkPointLocator vtkStaticPointLocator vtkMergePoints
 */

#ifndef vtkAbstractPointLocator_h
#define vtkAbstractPointLocator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkLocator.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;
class vtkIdList;
struct vtkDist2TupleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkAbstractPointLocator : public vtkLocator
{
public:
  ///@{
  /**
   * Standard type and print methods.
   */
  vtkTypeMacro(vtkAbstractPointLocator, vtkLocator);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify absolute tolerance (in world coordinates) for performing
   * geometric operations.
   *
   * The default is 0.001.
   */
  vtkSetClampMacro(Tolerance, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Tolerance, double);
  ///@}

  ///@{
  /**
   * Given a position x, return the id of the point closest to it. Alternative
   * method requires separate x-y-z values.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  virtual vtkIdType FindClosestPoint(const double x[3]) = 0;
  vtkIdType FindClosestPoint(double x, double y, double z);
  ///@}

  /**
   * Given a position x and a radius r, return the id of the point
   * closest to the point in that radius.
   * dist2 returns the squared distance to the point.
   */
  virtual vtkIdType FindClosestPointWithinRadius(
    double radius, const double x[3], double& dist2) = 0;

  ///@{
  /**
   * Find the closest N points to a position. This returns the closest
   * N points to a position. A faster method could be created that returned
   * N close points to a position, but necessarily the exact N closest.
   * The returned points are sorted from closest to farthest.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  virtual void FindClosestNPoints(int N, const double x[3], vtkIdList* result) = 0;
  void FindClosestNPoints(int N, double x, double y, double z, vtkIdList* result);
  ///@}

  ///@{
  /**
   * Find all points within a specified radius R of position x.
   * The result is not sorted in any specific manner.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  virtual void FindPointsWithinRadius(double R, const double x[3], vtkIdList* result) = 0;
  void FindPointsWithinRadius(double R, double x, double y, double z, vtkIdList* result);
  ///@}

  /**
   * Find all the points which are strictly greater than minDist2 and at most
   * maxDist2 away from the query point x (both are squares of distances; if
   * minDist2==0.0, coincident points are excluded -- use a negative value to
   * include them), optionally restricted to points that also lie inside (or
   * on) at least one of the petals: spheres (x,y,z,r2) for a 3D locator, or
   * circles (x,y,r2) in the x-y plane for a 2D one, so that a petal has as many
   * components as the dimension of the locator plus one (the default
   * implementation accepts both). The petals are the spheres (circles) of the Voronoi
   * flower of a point, whose union bounds the region that can still change its Voronoi
   * cell (see vtkVoronoiFlower3D, vtkVoronoiFlower2D and the class documentation). Results
   * are optionally sorted from closest to farthest.
   * Consecutive calls with adjacent shells (the maxDist2 of a call being the minDist2 of the next)
   * return disjoint sets of points, and miss none. The default implementation filters the result of
   * FindPointsWithinRadius(); subclasses can skip the regions outside of the shell or of the
   * petals. This method is thread safe if BuildLocator() is directly or indirectly called from a
   * single thread first.
   */
  virtual void FindPointsInShell(const double x[3], vtkDist2TupleArray& results, double minDist2,
    double maxDist2, bool sort = true, vtkDoubleArray* petals = nullptr);

  ///@{
  /**
   * Provide an accessor to the bounds. Valid after the locator is built.
   */
  virtual double* GetBounds() { return this->Bounds; }
  virtual void GetBounds(double*);
  ///@}

  ///@{
  /**
   * Return the total number of buckets in the locator. This has meaning only
   * after the locator is constructed.
   */
  vtkGetMacro(NumberOfBuckets, vtkIdType);
  ///@}

protected:
  vtkAbstractPointLocator();
  ~vtkAbstractPointLocator() override;

  double Bounds[6];          // bounds of points
  double Tolerance;          // for performing merging
  vtkIdType NumberOfBuckets; // total size of locator

private:
  vtkAbstractPointLocator(const vtkAbstractPointLocator&) = delete;
  void operator=(const vtkAbstractPointLocator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
