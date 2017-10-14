/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRadiusOutlierRemoval.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRadiusOutlierRemoval
 * @brief   remove isolated points
 *
 *
 * vtkRadiusOutlierRemoval removes isolated points; i.e., those points that
 * have few neighbors within a specified radius. The user must specify the
 * radius defining the local region, as well as the isolation threshold
 * (i.e., number of neighboring points required for the point to be
 * considered isolated). Optionally, users can specify a point locator to
 * accelerate local neighborhood search operations. (By default a
 * vtkStaticPointLocator will be created.)
 *
 * Note that while any vtkPointSet type can be provided as input, the output
 * is represented by an explicit representation of points via a
 * vtkPolyData. This output polydata will populate its instance of vtkPoints,
 * but no cells will be defined (i.e., no vtkVertex or vtkPolyVertex are
 * contained in the output). Also, after filter execution, the user can
 * request a vtkIdType* map which indicates how the input points were mapped
 * to the output. A value of map[i] (where i is the ith input point) less
 * than 0 means that the ith input point was removed. (See also the
 * superclass documentation for accessing the removed points through the
 * filter's second output.)
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkPointCloudFilter vtkStatisticalOutlierRemoval vtkExtractPoints
 * vtkThresholdPoints vtkImplicitFunction
*/

#ifndef vtkRadiusOutlierRemoval_h
#define vtkRadiusOutlierRemoval_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPointCloudFilter.h"

class vtkAbstractPointLocator;
class vtkPointSet;


class VTKFILTERSPOINTS_EXPORT vtkRadiusOutlierRemoval : public vtkPointCloudFilter
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static vtkRadiusOutlierRemoval *New();
  vtkTypeMacro(vtkRadiusOutlierRemoval,vtkPointCloudFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the local search radius.
   */
  vtkSetClampMacro(Radius,double,0.0,VTK_FLOAT_MAX);
  vtkGetMacro(Radius,double);
  //@}

  //@{
  /**
   * Specify the number of neighbors that a point must have, within
   * the specified radius, for the point to not be considered isolated.
   */
  vtkSetClampMacro(NumberOfNeighbors,int,1,VTK_INT_MAX);
  vtkGetMacro(NumberOfNeighbors,int);
  //@}

  //@{
  /**
   * Specify a point locator. By default a vtkStaticPointLocator is
   * used. The locator performs efficient searches to locate near a
   * specified interpolation position.
   */
  void SetLocator(vtkAbstractPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkAbstractPointLocator);
  //@}

protected:
  vtkRadiusOutlierRemoval();
  ~vtkRadiusOutlierRemoval() override;

  double Radius;
  int NumberOfNeighbors;
  vtkAbstractPointLocator *Locator;

  // All derived classes must implement this method. Note that a side effect of
  // the class is to populate the PointMap. Zero is returned if there is a failure.
  int FilterPoints(vtkPointSet *input) override;

private:
  vtkRadiusOutlierRemoval(const vtkRadiusOutlierRemoval&) = delete;
  void operator=(const vtkRadiusOutlierRemoval&) = delete;

};

#endif
