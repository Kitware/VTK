// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPointLocator
 * @brief   quickly locate points in 3-space
 *
 * vtkPointLocator is a spatial search object to quickly locate points in 3D.
 * vtkPointLocator works by dividing a specified region of space into a regular
 * array of "rectangular" buckets, and then keeping a list of points that
 * lie in each bucket. Typical operation involves giving a position in 3D
 * and finding the closest point.
 *
 * vtkPointLocator has two distinct methods of interaction. In the first
 * method, you supply it with a dataset, and it operates on the points in
 * the dataset. In the second method, you supply it with an array of points,
 * and the object operates on the array.
 *
 * @warning
 * Many other types of spatial locators have been developed such as
 * octrees and kd-trees. These are often more efficient for the
 * operations described here.
 *
 * @warning
 * Frequently vtkStaticPointLocator is used in lieu of vtkPointLocator.
 * They are very similar in terms of algorithmic approach, however
 * vtkStaticCellLocator is threaded and is typically much faster for
 * a large number of points (on the order of 3-5x faster). For small numbers
 * of points, vtkPointLocator is just as fast as vtkStaticPointLocator.
 *
 * @sa
 * vtkCellPicker vtkPointPicker vtkStaticPointLocator
 */

#ifndef vtkPointLocator_h
#define vtkPointLocator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkIncrementalPointLocator.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkIdList;
class vtkNeighborPoints;
class vtkPoints;

class VTKCOMMONDATAMODEL_EXPORT vtkPointLocator : public vtkIncrementalPointLocator
{
public:
  /**
   * Construct with automatic computation of divisions, averaging
   * 25 points per bucket.
   */
  static vtkPointLocator* New();

  ///@{
  /**
   * Standard methods for type management and printing.
   */
  vtkTypeMacro(vtkPointLocator, vtkIncrementalPointLocator);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Set the number of divisions in x-y-z directions.
   */
  vtkSetVector3Macro(Divisions, int);
  vtkGetVectorMacro(Divisions, int, 3);
  ///@}

  ///@{
  /**
   * Specify the average number of points in each bucket.
   */
  vtkSetClampMacro(NumberOfPointsPerBucket, int, 1, VTK_INT_MAX);
  vtkGetMacro(NumberOfPointsPerBucket, int);
  ///@}

  // Reuse any superclass signatures that we don't override.
  using vtkAbstractPointLocator::FindClosestPoint;

  /**
   * Given a position x, return the id of the point closest to it. Alternative
   * method requires separate x-y-z values.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  vtkIdType FindClosestPoint(const double x[3]) override;

  ///@{
  /**
   * Given a position x and a radius r, return the id of the point
   * closest to the point in that radius.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first. dist2 returns the squared
   * distance to the point.
   */
  vtkIdType FindClosestPointWithinRadius(double radius, const double x[3], double& dist2) override;
  virtual vtkIdType FindClosestPointWithinRadius(
    double radius, const double x[3], double inputDataLength, double& dist2);
  ///@}

  /**
   * Initialize the point insertion process. The newPts is an object
   * representing point coordinates into which incremental insertion methods
   * place their data. Bounds are the box that the points lie in.
   * Not thread safe.
   */
  int InitPointInsertion(vtkPoints* newPts, const double bounds[6]) override;

  /**
   * Initialize the point insertion process. The newPts is an object
   * representing point coordinates into which incremental insertion methods
   * place their data. Bounds are the box that the points lie in.
   * Not thread safe.
   */
  int InitPointInsertion(vtkPoints* newPts, const double bounds[6], vtkIdType estNumPts) override;

  /**
   * Incrementally insert a point into search structure with a particular
   * index value. You should use the method IsInsertedPoint() to see whether
   * this point has already been inserted (that is, if you desire to prevent
   * duplicate points). Before using this method you must make sure that
   * newPts have been supplied, the bounds has been set properly, and that
   * divs are properly set. (See InitPointInsertion().)
   * Not thread safe.
   */
  void InsertPoint(vtkIdType ptId, const double x[3]) override;

  /**
   * Incrementally insert a point into search structure. The method returns
   * the insertion location (i.e., point id). You should use the method
   * IsInsertedPoint() to see whether this point has already been
   * inserted (that is, if you desire to prevent duplicate points).
   * Before using this method you must make sure that newPts have been
   * supplied, the bounds has been set properly, and that divs are
   * properly set. (See InitPointInsertion().)
   * Not thread safe.
   */
  vtkIdType InsertNextPoint(const double x[3]) override;

  ///@{
  /**
   * Determine whether point given by x[3] has been inserted into points list.
   * Return id of previously inserted point if this is true, otherwise return
   * -1. This method is thread safe.
   */
  vtkIdType IsInsertedPoint(double x, double y, double z) override
  {
    double xyz[3];
    xyz[0] = x;
    xyz[1] = y;
    xyz[2] = z;
    return this->IsInsertedPoint(xyz);
  }
  vtkIdType IsInsertedPoint(const double x[3]) override;
  ///@}

  /**
   * Determine whether point given by x[3] has been inserted into points list.
   * Return 0 if point was already in the list, otherwise return 1. If the
   * point was not in the list, it will be ADDED.  In either case, the id of
   * the point (newly inserted or not) is returned in the ptId argument.
   * Note this combines the functionality of IsInsertedPoint() followed
   * by a call to InsertNextPoint().
   * This method is not thread safe.
   */
  int InsertUniquePoint(const double x[3], vtkIdType& ptId) override;

  /**
   * Given a position x, return the id of the point closest to it. This method
   * is used when performing incremental point insertion. Note that -1
   * indicates that no point was found.
   * This method is thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  vtkIdType FindClosestInsertedPoint(const double x[3]) override;

  /**
   * Find the closest N points to a position. This returns the closest
   * N points to a position. A faster method could be created that returned
   * N close points to a position, but necessarily the exact N closest.
   * The returned points are sorted from closest to farthest.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  void FindClosestNPoints(int N, const double x[3], vtkIdList* result) override;

  ///@{
  /**
   * Find the closest points to a position such that each octant of
   * space around the position contains at least N points. Loosely
   * limit the search to a maximum number of points evaluated, M.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  virtual void FindDistributedPoints(int N, const double x[3], vtkIdList* result, int M);
  virtual void FindDistributedPoints(int N, double x, double y, double z, vtkIdList* result, int M);
  ///@}

  /**
   * Find all points within a specified radius R of position x.
   * The result is not sorted in any specific manner.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  void FindPointsWithinRadius(double R, const double x[3], vtkIdList* result) override;

  /**
   * Given a position x, return the list of points in the bucket that
   * contains the point. It is possible that nullptr is returned. The user
   * provides an ijk array that is the indices into the locator.
   * This method is thread safe.
   */
  virtual vtkIdList* GetPointsInBucket(const double x[3], int ijk[3]);

  ///@{
  /**
   * Provide an accessor to the points.
   */
  vtkGetObjectMacro(Points, vtkPoints);
  ///@}

  ///@{
  /**
   * See vtkLocator interface documentation.
   * These methods are not thread safe.
   */
  void Initialize() override;
  void FreeSearchStructure() override;
  void BuildLocator() override;
  void ForceBuildLocator() override;
  void GenerateRepresentation(int level, vtkPolyData* pd) override;
  ///@}

protected:
  vtkPointLocator();
  ~vtkPointLocator() override;

  void BuildLocatorInternal() override;

  // place points in appropriate buckets
  void GetBucketNeighbors(
    vtkNeighborPoints* buckets, const int ijk[3], const int ndivs[3], int level);
  void GetOverlappingBuckets(
    vtkNeighborPoints* buckets, const double x[3], const int ijk[3], double dist, int level);
  void GetOverlappingBuckets(vtkNeighborPoints* buckets, const double x[3], double dist,
    int prevMinLevel[3], int prevMaxLevel[3]);
  void GenerateFace(int face, int i, int j, int k, vtkPoints* pts, vtkCellArray* polys);
  double Distance2ToBucket(const double x[3], const int nei[3]);
  double Distance2ToBounds(const double x[3], const double bounds[6]);

  vtkPoints* Points;           // Used for merging points
  int Divisions[3];            // Number of sub-divisions in x-y-z directions
  int NumberOfPointsPerBucket; // Used with previous boolean to control subdivide
  vtkIdList** HashTable;       // lists of point ids in buckets
  double H[3];                 // width of each bucket in x-y-z directions

  double InsertionTol2;
  vtkIdType InsertionPointId;
  double InsertionLevel;

  // These are inlined methods and data members for performance reasons
  double HX, HY, HZ;
  double FX, FY, FZ, BX, BY, BZ;
  vtkIdType XD, YD, ZD, SliceSize;

  void GetBucketIndices(const double* x, int ijk[3]) const
  {
    // Compute point index. Make sure it lies within range of locator.
    vtkIdType tmp0 = static_cast<vtkIdType>(((x[0] - this->BX) * this->FX));
    vtkIdType tmp1 = static_cast<vtkIdType>(((x[1] - this->BY) * this->FY));
    vtkIdType tmp2 = static_cast<vtkIdType>(((x[2] - this->BZ) * this->FZ));

    ijk[0] = tmp0 < 0 ? 0 : (tmp0 >= this->XD ? this->XD - 1 : tmp0);
    ijk[1] = tmp1 < 0 ? 0 : (tmp1 >= this->YD ? this->YD - 1 : tmp1);
    ijk[2] = tmp2 < 0 ? 0 : (tmp2 >= this->ZD ? this->ZD - 1 : tmp2);
  }

  vtkIdType GetBucketIndex(const double* x) const
  {
    int ijk[3];
    this->GetBucketIndices(x, ijk);
    return ijk[0] + ijk[1] * this->XD + ijk[2] * this->SliceSize;
  }

  void ComputePerformanceFactors();

private:
  vtkPointLocator(const vtkPointLocator&) = delete;
  void operator=(const vtkPointLocator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
