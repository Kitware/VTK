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
class vtkBucketList;


class VTKCOMMONDATAMODEL_EXPORT vtkStaticPointLocator : public vtkAbstractPointLocator
{
friend class vtkBucketList;
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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
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
   * NumberOfPointsPerBucket data member.
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
  vtkIdType FindClosestPoint(const double x[3]) VTK_OVERRIDE;

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
    double radius, const double x[3], double& dist2) VTK_OVERRIDE;
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
  void FindClosestNPoints(int N, const double x[3], vtkIdList *result) VTK_OVERRIDE;

  /**
   * Find all points within a specified radius R of position x.
   * The result is not sorted in any specific manner.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  void FindPointsWithinRadius(double R, const double x[3],
                              vtkIdList *result) VTK_OVERRIDE;

  //@{
  /**
   * See vtkLocator and vtkAbstractPointLocator interface documentation.
   * These methods are not thread safe.
   */
  void Initialize() VTK_OVERRIDE;
  void FreeSearchStructure() VTK_OVERRIDE;
  void BuildLocator() VTK_OVERRIDE;
  void GenerateRepresentation(int level, vtkPolyData *pd) VTK_OVERRIDE;
  //@}

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

  /**
   * Inform the user as to whether large ids are being used. This flag only
   * has meaning after the locator has been built. Large ids are used when the
   * number of binned points, or the number of bins, is >= the signed integer
   * max value.
   */
  bool GetLargeIds() {return this->LargeIds;}

protected:
  vtkStaticPointLocator();
  ~vtkStaticPointLocator() VTK_OVERRIDE;

  int NumberOfPointsPerBucket; // Used with AutomaticOn to control subdivide
  int Divisions[3]; // Number of sub-divisions in x-y-z directions
  double H[3]; // Width of each bucket in x-y-z directions
  vtkBucketList *Buckets; // Lists of point ids in each bucket
  bool LargeIds; //indicate whether integer ids are small or large

private:
  vtkStaticPointLocator(const vtkStaticPointLocator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkStaticPointLocator&) VTK_DELETE_FUNCTION;

};

#endif
