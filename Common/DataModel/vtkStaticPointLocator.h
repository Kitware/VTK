/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStaticPointLocator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkStaticPointLocator
 * @brief   quickly locate points in 3-space
 *
 * vtkStaticPointLocator is a spatial search object to quickly locate points
 * in 3D.  vtkStaticPointLocator works by dividing a specified region of
 * space into a regular array of cuboid buckets, and then keeping a
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
 * This class is templated. It may run slower than serial execution if the code
 * is not optimized during compilation. Build in Release or ReleaseWithDebugInfo.
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
 * @sa
 * vtkPointLocator vtkCellLocator vtkLocator vtkAbstractPointLocator
*/

#ifndef vtkStaticPointLocator_h
#define vtkStaticPointLocator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkAbstractPointLocator.h"

class vtkIdList;
struct vtkBucketList;


class VTKCOMMONDATAMODEL_EXPORT vtkStaticPointLocator : public vtkAbstractPointLocator
{
public:
  /**
   * Construct with automatic computation of divisions, averaging
   * 5 points per bucket.
   */
  static vtkStaticPointLocator *New();

  //@{
  /**
   * Standard type and print methods.
   */
  vtkTypeMacro(vtkStaticPointLocator,vtkAbstractPointLocator);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the average number of points in each bucket. This data member is
   * used in conjunction with the Automatic data member (if enabled) to
   * determine the number of locator x-y-z divisions.
   */
  vtkSetClampMacro(NumberOfPointsPerBucket,int,1,VTK_INT_MAX);
  vtkGetMacro(NumberOfPointsPerBucket,int);
  //@}

  //@{
  /**
   * Set the number of divisions in x-y-z directions. If the Automatic data
   * member is enabled, the Divisions are set according to the
   * NumberOfPointsPerBucket and MaxNumberOfBuckets data members. The number
   * of divisions must be >= 1 in each direction.
   */
  vtkSetVector3Macro(Divisions,int);
  vtkGetVectorMacro(Divisions,int,3);
  //@}

  // Re-use any superclass signatures that we don't override.
  using vtkAbstractPointLocator::FindClosestPoint;
  using vtkAbstractPointLocator::FindClosestNPoints;
  using vtkAbstractPointLocator::FindPointsWithinRadius;
  using vtkAbstractPointLocator::GetBounds;

  /**
   * Given a position x, return the id of the point closest to it. An
   * alternative method (defined in the superclass) requires separate x-y-z
   * values. These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  vtkIdType FindClosestPoint(const double x[3]) override;

  //@{
  /**
   * Given a position x and a radius r, return the id of the point closest to
   * the point in that radius.  These methods are thread safe if
   * BuildLocator() is directly or indirectly called from a single thread
   * first. dist2 returns the squared distance to the point. Note that if multiple
   * points are located the same distance away, the actual point returned is a
   * function in which order the points are processed (i.e., indeterminate).
   */
  vtkIdType FindClosestPointWithinRadius(
    double radius, const double x[3], double& dist2) override;
  virtual vtkIdType FindClosestPointWithinRadius(double radius, const double x[3],
                                                 double inputDataLength,
                                                 double& dist2);
  //@}

  /**
   * Find the closest N points to a position. This returns the closest N
   * points to a position. A faster method could be created that returned N
   * close points to a position, but necessarily the exact N closest.  The
   * returned points are sorted from closest to farthest.  These methods are
   * thread safe if BuildLocator() is directly or indirectly called from a
   * single thread first.
   */
  void FindClosestNPoints(int N, const double x[3], vtkIdList *result) override;

  /**
   * Find all points within a specified radius R of position x.
   * The result is not sorted in any specific manner.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  void FindPointsWithinRadius(double R, const double x[3],
                              vtkIdList *result) override;

  /**
   * Intersect the points contained in the locator with the line defined by
   * (a0,a1). Return the point within the tolerance tol that is closest to a0
   * (tol measured in the world coordinate system). If an intersection occurs
   * (i.e., the method returns nonzero), then the parametric location along
   * the line t, the closest position along the line lineX, and the coordinates
   * of the picked ptId is returned in ptX. (This method is thread safe after
   * the locator is built.)
   */
  int IntersectWithLine(double a0[3], double a1[3], double tol, double& t,
                        double lineX[3], double ptX[3], vtkIdType &ptId);

  /**
   * Merge points in the locator given a tolerance. Return a merge map which
   * represents the mapping of "concident" point ids to a single point. Note
   * the number of points in the merge map is the number of points the
   * locator was built with. The user is expected to pass in an allocated
   * mergeMap.
   */
  void MergePoints(double tol, vtkIdType *mergeMap);

  //@{
  /**
   * See vtkLocator and vtkAbstractPointLocator interface documentation.
   * These methods are not thread safe.
   */
  void Initialize() override;
  void FreeSearchStructure() override;
  void BuildLocator() override;
  void BuildLocator(const double *bounds);
  //@}

  /**
   * Populate a polydata with the faces of the bins that potentially contain cells.
   * Note that the level parameter has no effect on this method as there is no
   * hierarchy built (i.e., uniform binning). Typically this is used for debugging.
   */
  void GenerateRepresentation(int level, vtkPolyData *pd) override;

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
  void GetBucketIds(vtkIdType bNum, vtkIdList *bList);

  //@{
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
  vtkSetClampMacro(MaxNumberOfBuckets,vtkIdType,1000,VTK_ID_MAX);
  vtkGetMacro(MaxNumberOfBuckets,vtkIdType);
  //@}

  /**
   * Inform the user as to whether large ids are being used. This flag only
   * has meaning after the locator has been built. Large ids are used when the
   * number of binned points, or the number of bins, is >= the maximum number
   * of buckets (specified by the user). Note that LargeIds are only available
   * on 64-bit architectures.
   */
  bool GetLargeIds() {return this->LargeIds;}

  //@{
  /**
   * Provide an accessor to the bucket spacing. Valid after the locator is
   * built.
   */
  virtual double *GetSpacing() { return this->H; }
  virtual void GetSpacing(double spacing[3])
  { spacing[0] = this->H[0]; spacing[1] = this->H[1]; spacing[2] = this->H[2]; }
  //@}

protected:
  vtkStaticPointLocator();
  ~vtkStaticPointLocator() override;

  int NumberOfPointsPerBucket; // Used with AutomaticOn to control subdivide
  int Divisions[3]; // Number of sub-divisions in x-y-z directions
  double H[3]; // Width of each bucket in x-y-z directions
  vtkBucketList *Buckets; // Lists of point ids in each bucket
  vtkIdType MaxNumberOfBuckets; // Maximum number of buckets in locator
  bool LargeIds; //indicate whether integer ids are small or large

private:
  vtkStaticPointLocator(const vtkStaticPointLocator&) = delete;
  void operator=(const vtkStaticPointLocator&) = delete;

};

#endif
