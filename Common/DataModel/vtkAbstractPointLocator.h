/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractPointLocator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
 * @sa
 * vtkPointLocator vtkStaticPointLocator vtkMergePoints
*/

#ifndef vtkAbstractPointLocator_h
#define vtkAbstractPointLocator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkLocator.h"

class vtkIdList;

class VTKCOMMONDATAMODEL_EXPORT vtkAbstractPointLocator : public vtkLocator
{
public:
  //@{
  /**
   * Standard type and print methods.
   */
  vtkTypeMacro(vtkAbstractPointLocator,vtkLocator);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Given a position x, return the id of the point closest to it. Alternative
   * method requires separate x-y-z values.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  virtual vtkIdType FindClosestPoint(const double x[3]) = 0;
  vtkIdType FindClosestPoint(double x, double y, double z);
  //@}

  /**
   * Given a position x and a radius r, return the id of the point
   * closest to the point in that radius.
   * dist2 returns the squared distance to the point.
   */
  virtual vtkIdType FindClosestPointWithinRadius(
    double radius, const double x[3], double& dist2) = 0;

  //@{
  /**
   * Find the closest N points to a position. This returns the closest
   * N points to a position. A faster method could be created that returned
   * N close points to a position, but necessarily the exact N closest.
   * The returned points are sorted from closest to farthest.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  virtual void FindClosestNPoints(
    int N, const double x[3], vtkIdList *result) = 0;
  void FindClosestNPoints(int N, double x, double y, double z,
                          vtkIdList *result);
  //@}

  //@{
  /**
   * Find all points within a specified radius R of position x.
   * The result is not sorted in any specific manner.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  virtual void FindPointsWithinRadius(double R, const double x[3],
                                      vtkIdList *result) = 0;
  void FindPointsWithinRadius(double R, double x, double y, double z,
                                      vtkIdList *result);
  //@}

  //@{
  /**
   * Provide an accessor to the bounds. Valid after the locator is built.
   */
  virtual double *GetBounds() { return this->Bounds; }
  virtual void GetBounds(double*);
  //@}

  //@{
  /**
   * Return the total number of buckets in the locator. This has meaning only
   * after the locator is constructed.
   */
  vtkGetMacro(NumberOfBuckets,vtkIdType);
  //@}

protected:
  vtkAbstractPointLocator();
  ~vtkAbstractPointLocator() override;

  double Bounds[6]; // bounds of points
  vtkIdType NumberOfBuckets; // total size of locator

private:
  vtkAbstractPointLocator(const vtkAbstractPointLocator&) = delete;
  void operator=(const vtkAbstractPointLocator&) = delete;
};

#endif
