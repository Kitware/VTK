/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKdTreePointLocator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkKdTreePointLocator
 * @brief   class to quickly locate points in 3-space
 *
 * vtkKdTreePointLocator is a wrapper class that derives from
 * vtkAbstractPointLocator and calls the search functions in vtkKdTree.
 *
 * @sa
 * vtkKdTree
*/

#ifndef vtkKdTreePointLocator_h
#define vtkKdTreePointLocator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkAbstractPointLocator.h"

class vtkIdList;
class vtkKdTree;

class VTKCOMMONDATAMODEL_EXPORT vtkKdTreePointLocator : public vtkAbstractPointLocator
{
public:
  vtkTypeMacro(vtkKdTreePointLocator,vtkAbstractPointLocator);
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
  vtkIdType FindClosestPointWithinRadius(
    double radius, const double x[3], double& dist2) override;

  /**
   * Find the closest N points to a position. This returns the closest
   * N points to a position. A faster method could be created that returned
   * N close points to a position, but necessarily the exact N closest.
   * The returned points are sorted from closest to farthest.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  void FindClosestNPoints(
    int N, const double x[3], vtkIdList *result) override;

  /**
   * Find all points within a specified radius R of position x.
   * The result is not sorted in any specific manner.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  void FindPointsWithinRadius(double R, const double x[3],
                              vtkIdList *result) override;

  //@{
  /**
   * See vtkLocator interface documentation.
   * These methods are not thread safe.
   */
  void FreeSearchStructure() override;
  void BuildLocator() override;
  void GenerateRepresentation(int level, vtkPolyData *pd) override;
  //@}

protected:
  vtkKdTreePointLocator();
  ~vtkKdTreePointLocator() override;

  vtkKdTree* KdTree;

private:
  vtkKdTreePointLocator(const vtkKdTreePointLocator&) = delete;
  void operator=(const vtkKdTreePointLocator&) = delete;
};

#endif


