// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStaticPointLocator
 * @brief   quickly locate points in 3-space
 *
 * vtkStaticPointLocator is a spatial search object to quickly locate points
 * in 3D.  vtkStaticPointLocator works by dividing a specified region of
 * space into a regular array of cuboid buckets (or bins), and then keeping a
 * list of points that lie in each bucket. Typical operation involves giving
 * a position in 3D and finding the closest point; or finding the N closest
 * points.
 *
 * vtkStaticPointLocator is an accelerated version of vtkPointLocator. It is
 * threaded (via vtkSMPTools), and supports one-time static construction
 * (i.e., incremental point insertion is not supported). If you need to
 * incrementally insert points, use the vtkPointLocator or its kin to do so.
 *
 * @warning
 * Iterators (e.g., vtkShellBinIterator) over locator bins are available for
 * advanced usage. This can be used for example to locate points in proximity
 * to a query point.
 *
 * @warning
 * The internals of this class are templated. It may run slow if the code is
 * not optimized during compilation. Build in Release or
 * ReleaseWithDebugInfo.  Also note that while the vtkStaticPointLocator is
 * internally templated, it provides a non-templated API - this is done to
 * ensure that the class can be easily used by interpreted, wrapped languages
 * (e.g., Python).
 *
 * @warning
 * Make sure that you review the documentation for the superclasses
 * vtkAbstactPointLocator and vtkLocator. In particular the Automatic
 * data member can be used to automatically determine divisions based
 * on the average number of points per bucket.
 *
 * @warning
 * Other types of spatial locators have been developed such as octrees and
 * kd-trees. These are often more efficient for the operations described
 * here.
 *
 * @warning
 * Frequently vtkStaticPointLocator is used in lieu of vtkPointLocator.
 * They are very similar in terms of algorithmic approach, however
 * vtkStaticCellLocator is threaded and is typically much faster for
 * a large number of points (on the order of 3-5x faster). For small numbers
 * of points, vtkPointLocator is just as fast as vtkStaticPointLocator.
 *
 * @sa
 * vtkPointLocator vtkStaticPointLocator2D vtkCellLocator vtkLocator
 * vtkAbstractPointLocator vtkLocatorInterface vtkShellBinIterator
 * vtkAnnularBinIterator vtkVoronoiCore3D vtkVoronoi3D
 */

#ifndef vtkStaticPointLocator_h
#define vtkStaticPointLocator_h

#include "vtkAbstractPointLocator.h"
#include "vtkCommonDataModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;
class vtkIdList;
struct vtkBucketList;
class vtkDataArray;
struct vtkDist2TupleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkStaticPointLocator : public vtkAbstractPointLocator
{
public:
  /**
   * Construct with automatic computation of divisions, averaging
   * 5 points per bucket.
   */
  static vtkStaticPointLocator* New();

  ///@{
  /**
   * Standard type and print methods.
   */
  vtkTypeMacro(vtkStaticPointLocator, vtkAbstractPointLocator);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify the average number of points in each bucket. This data member is
   * used in conjunction with the Automatic data member (if enabled) to
   * determine the number of locator x-y-z divisions.
   */
  vtkSetClampMacro(NumberOfPointsPerBucket, int, 1, VTK_INT_MAX);
  vtkGetMacro(NumberOfPointsPerBucket, int);
  ///@}

  ///@{
  /**
   * Set the number of divisions in x-y-z directions. If the Automatic data
   * member is enabled, the Divisions are set according to the
   * NumberOfPointsPerBucket and MaxNumberOfBuckets data members. The number
   * of divisions must be >= 1 in each direction.
   */
  vtkSetVector3Macro(Divisions, int);
  vtkGetVectorMacro(Divisions, int, 3);
  ///@}

  // Reuse any superclass signatures that we don't override.
  using vtkAbstractPointLocator::FindClosestNPoints;
  using vtkAbstractPointLocator::FindClosestPoint;
  using vtkAbstractPointLocator::FindPointsWithinRadius;
  using vtkAbstractPointLocator::GetBounds;

  /**
   * Given a position x, return the id of the point closest to it, or (-1) if
   * no point found. An alternative method (defined in the superclass)
   * requires separate x-y-z values. These methods are thread safe if
   * BuildLocator() is directly or indirectly called from a single thread
   * first.
   */
  vtkIdType FindClosestPoint(const double x[3]) override;

  ///@{
  /**
   * Given a position x and a radius r, return the id of the point closest to
   * the point in that radius, or (-1) if nothing found.  These methods are
   * thread safe if BuildLocator() is directly or indirectly called from a
   * single thread first. dist2 returns the squared distance to the
   * point. Note that if multiple points are located the same distance away,
   * the actual point returned is a function in which order the points are
   * processed (i.e., indeterminate).
   */
  vtkIdType FindClosestPointWithinRadius(double radius, const double x[3], double& dist2) override;
  virtual vtkIdType FindClosestPointWithinRadius(
    double radius, const double x[3], double inputDataLength, double& dist2);
  ///@}

  /**
   * Find the closest N points to a position. This returns the closest N
   * points to a position (unless N is greater than the number of points in
   * the locator). Note that if some points are precisely the same distance
   * from the query point, then some equidistance points may not be included
   * in the result. The returned points are sorted from closest to farthest.
   * This method is thread safe if BuildLocator() is directly or indirectly
   * called from a single thread first.
   */
  void FindClosestNPoints(int N, const double x[3], vtkIdList* result) override;

  /**
   * Find approximately N close points which are strictly greater than
   * >minDist2 away from the query point x (minDist2 is the square of the
   * distance). (Note: if minDist2==0.0, then no points coincident to x are
   * returned. To obtain coincident points, set minDist2<0.) The number of
   * points returned may != N either because there are fewer than N points in
   * the locator, the query region >minDist2 defines a subset of <N points,
   * or >N points may be returned because 1) its computationally simpler to
   * do so, and 2) *all* points of distance maxDist2 are returned. The method
   * returns the maximum distance squared (maxDist2) of the points, the point
   * ids in the vtkIdList result, and the point's r**2 from x in radii2.
   * Optionally, the points can be sorted by distance from the query point
   * (sorted from closest to farthest).  This method is thread safe if
   * BuildLocator() is directly or indirectly called from a single thread
   * first. Finally, a powerful feature of this method in that it's possible
   * to identify disjoint sets of points within nested spherical shells,
   * with optional spherical petals. (The petals are represented by a
   * four tuple of (x,y,z,r2).)
   */
  double FindNPointsInShell(int N, const double x[3], vtkDist2TupleArray& results,
    double minDist2 = (-0.1), bool sort = true, vtkDoubleArray* petals = nullptr);

  /**
   * Find all points within a specified radius R of position x.
   * The result is not sorted in any specific manner.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  void FindPointsWithinRadius(double R, const double x[3], vtkIdList* result) override;

  /**
   * Intersect the points contained in the locator with the line defined by
   * (a0,a1). Return the point within the tolerance tol that is closest to a0
   * (tol measured in the world coordinate system). If an intersection occurs
   * (i.e., the method returns nonzero), then the parametric location along
   * the line t, the closest position along the line lineX, and the coordinates
   * of the picked ptId is returned in ptX. (This method is thread safe after
   * the locator is built.)
   */
  int IntersectWithLine(double a0[3], double a1[3], double tol, double& t, double lineX[3],
    double ptX[3], vtkIdType& ptId);

  /**
   * Merge points in the locator given a tolerance. Return a merge map which
   * maps all points to the points to which they were merged. Note the number
   * of points in the merge map is the number of points the locator was built
   * with. The user is expected to pass in an allocated mergeMap. Note that
   * when tol!=0, the traversal order of threading can be specified using the
   * SetTraversalOrder() method. The traversal order is needed to generate
   * deterministic output (i.e., output independent of thread execution
   * order).
   */
  void MergePoints(double tol, vtkIdType* mergeMap);

  /**
   * Merge points and associated data in the locator. Return a merge map
   * which maps all points to the points to which they were merged. Note the
   * number of points in the merge map is the number of points the locator
   * was built with. Merging occurs when both the points and the associated
   * point data (represented by the data array) are equal. The user is
   * expected to pass in an allocated mergeMap, and the size of the data
   * array should be number of points in the locator. The data array may
   * have an arbitrary number of components.
   */
  void MergePointsWithData(vtkDataArray* data, vtkIdType* mergeMap);

  ///@{
  /**
   * See vtkLocator and vtkAbstractPointLocator interface documentation.
   * These methods are not thread safe.
   */
  void Initialize() override;
  void FreeSearchStructure() override;
  void BuildLocator() override;
  void ForceBuildLocator() override;
  void BuildLocator(const double* inBounds);
  ///@}

  /**
   * Populate a polydata with the faces of the bins that potentially contain cells.
   * Note that the level parameter has no effect on this method as there is no
   * hierarchy built (i.e., uniform binning). Typically this is used for debugging.
   */
  void GenerateRepresentation(int level, vtkPolyData* pd) override;

  /**
   * Given a bucket number bNum between 0 <= bNum < this->GetNumberOfBuckets(),
   * return the number of points found in the bucket.
   */
  vtkIdType GetNumberOfPointsInBucket(vtkIdType bNum);

  /**
   * Given a bucket number bNum between 0 <= bNum < this->GetNumberOfBuckets(),
   * return a list of point ids contained within the bucket. The user must
   * provide an instance of vtkIdList to contain the result.
   */
  void GetBucketIds(vtkIdType bNum, vtkIdList* bList);

  /**
   * Given a bucket/bin located at position (i,j,k), compute the center
   * of the bucket.
   */
  void GetBucketCenter(int i, int j, int k, double center[3]);

  ///@{
  /**
   * Set the maximum number of buckets in the locator. By default the value
   * is set to VTK_INT_MAX. Note that there are significant performance
   * implications at work here. If the number of buckets is set very large
   * (meaning > VTK_INT_MAX) then internal sorting may be performed using
   * 64-bit integers (which is much slower than using a 32-bit int). Of
   * course, memory requirements may dramatically increase as well.  It is
   * recommended that the default value be used; but for extremely large data
   * it may be desired to create a locator with an exceptionally large number
   * of buckets. Note also that during initialization of the locator if the
   * MaxNumberOfBuckets threshold is exceeded, the Divisions are scaled down
   * in such a way as not to exceed the MaxNumberOfBuckets proportionally to
   * the size of the bounding box in the x-y-z directions.
   */
  vtkSetClampMacro(MaxNumberOfBuckets, vtkIdType, 1000, VTK_ID_MAX);
  vtkGetMacro(MaxNumberOfBuckets, vtkIdType);
  ///@}

  /**
   * Inform the user as to whether large ids are being used. This flag only
   * has meaning after the locator has been built. Large ids are used when the
   * number of binned points, or the number of bins, is >= the maximum number
   * of buckets (specified by the user). Note that LargeIds are only available
   * on 64-bit architectures.
   */
  bool GetLargeIds() { return this->LargeIds; }

  ///@{
  /**
   * Provide an accessor to the bucket spacing. Valid after the locator is
   * built.
   */
  virtual double* GetSpacing() { return this->H; }
  virtual void GetSpacing(double spacing[3])
  {
    spacing[0] = this->H[0];
    spacing[1] = this->H[1];
    spacing[2] = this->H[2];
  }
  ///@}

  /**
   * Point merging is inherently an order-dependent process. Because naive
   * threaded execution can non-deterministically change the ordering of
   * merged points, threaded point merging for tolerances != 0.0 requires
   * some level of serialization of the merging process to produce
   * deterministic results. POINT_ORDER is completely serialized: two
   * points with ids p0,p1 are merged if they are within tolerance of one
   * another, with p1->p0 (p1 merged to p0) if p0<p1. IN BIN_ORDER, a
   * threaded checkerboarding approach is used so that p1->p0 when p0<p1 in
   * the local bin neighborhood and p1 not yet merged.
   */
  enum TraversalOrderType
  {
    POINT_ORDER = 0,
    BIN_ORDER = 1
  };

  ///@{
  /**
   * Specify the manner in which points are processed when a non-zero merge
   * tolerance is specified. By default, BIN_ORDER is used (i.e., threaded
   * using bin checkerboard traversal) versus POINT_ORDER, which is a serial traversal.
   */
  vtkSetClampMacro(
    TraversalOrder, int, vtkStaticPointLocator::POINT_ORDER, vtkStaticPointLocator::BIN_ORDER);
  vtkGetMacro(TraversalOrder, int);
  void SetTraversalOrderToPointOrder()
  {
    this->SetTraversalOrder(vtkStaticPointLocator::POINT_ORDER);
  }
  void SetTraversalOrderToBinOrder() { this->SetTraversalOrder(vtkStaticPointLocator::BIN_ORDER); }
  ///@}

  ///@{
  /**
   * The bounding box / size of the locator can be increased by specifying padding. If >0, then
   * the locator bounds extent will be increased by +/- padding in the z-y-z directions.
   */
  vtkSetMacro(Padding, double);
  vtkGetMacro(Padding, double);
  ///@}

  ///@{
  /**
   * Turn on/off flag to control whether the locator checks modified time after it
   * is built. These methods are generally used to accelerate the use of methods
   * in tight loops and avoid MTime checks. Typically, StaticOn() is invoked after
   * BuildLocator(), and then StaticOff() is invoked after the end of processing.
   */
  void StaticOn() { this->Static = true; }
  void StaticOff() { this->Static = false; }
  vtkGetMacro(Static, vtkTypeBool);
  ///@}

  /**
   * This method is useful for accessing the raw binned data. Call this after BuildLocator().
   */
  vtkBucketList* GetBuckets() { return this->Buckets; }

protected:
  vtkStaticPointLocator();
  ~vtkStaticPointLocator() override;

  void BuildLocatorInternal() override;

  int NumberOfPointsPerBucket;  // Used with AutomaticOn to control subdivide
  int Divisions[3];             // Number of sub-divisions in x-y-z directions
  double H[3];                  // Width of each bucket in x-y-z directions
  vtkBucketList* Buckets;       // Lists of point ids in each bucket
  vtkIdType MaxNumberOfBuckets; // Maximum number of buckets in locator
  bool LargeIds;                // Integer point ids are small (32-bit) or large (64-bit)
  int TraversalOrder;           // Control traversal order when threading
  double Padding;               // Pad out the bounding box of the locator
  vtkTypeBool Static;           // Control whether to repeatedly check modified time

private:
  vtkStaticPointLocator(const vtkStaticPointLocator&) = delete;
  void operator=(const vtkStaticPointLocator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
