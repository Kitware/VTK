/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergePoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMergePoints
 * @brief   merge exactly coincident points
 *
 * vtkMergePoints is a locator object to quickly locate points in 3D.
 * The primary difference between vtkMergePoints and its superclass
 * vtkPointLocator is that vtkMergePoints merges precisely coincident points
 * and is therefore much faster.
 * @sa
 * vtkCleanPolyData
*/

#ifndef vtkMergePoints_h
#define vtkMergePoints_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkPointLocator.h"

class VTKCOMMONDATAMODEL_EXPORT vtkMergePoints : public vtkPointLocator
{
public:
  static vtkMergePoints *New();
  vtkTypeMacro(vtkMergePoints,vtkPointLocator);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Determine whether point given by x[3] has been inserted into points list.
   * Return id of previously inserted point if this is true, otherwise return
   * -1.
   */
  vtkIdType IsInsertedPoint(const double x[3]) VTK_OVERRIDE;
  vtkIdType IsInsertedPoint(double x, double  y, double z) VTK_OVERRIDE
    {return this->vtkPointLocator::IsInsertedPoint(x, y, z); };
  //@}

  /**
   * Determine whether point given by x[3] has been inserted into points list.
   * Return 0 if point was already in the list, otherwise return 1. If the
   * point was not in the list, it will be ADDED.  In either case, the id of
   * the point (newly inserted or not) is returned in the ptId argument.
   * Note this combines the functionality of IsInsertedPoint() followed
   * by a call to InsertNextPoint().
   */
  int InsertUniquePoint(const double x[3], vtkIdType &ptId) VTK_OVERRIDE;

protected:
  vtkMergePoints() {}
  ~vtkMergePoints() VTK_OVERRIDE {}

private:
  vtkMergePoints(const vtkMergePoints&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMergePoints&) VTK_DELETE_FUNCTION;
};

#endif


