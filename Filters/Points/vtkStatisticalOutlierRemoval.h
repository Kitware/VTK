/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStatisticalOutlierRemoval.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkStatisticalOutlierRemoval
 * @brief   remove sparse outlier points
 *
 *
 * The vtkStatisticalOutlierRemoval filter removes sparse outlier points
 * through statistical analysis. The average (mean) distance between points
 * in the point cloud is computed (taking a local sample size around each
 * point); followed by computation of the global standard deviation of
 * distances between points. This global, statistical information is compared
 * against the mean separation distance for each point; those points whose
 * average separation is greater than the user-specified variation in
 * a multiple of standard deviation are removed.
 *
 * Note that while any vtkPointSet type can be provided as input, the output is
 * represented by an explicit representation of points via a
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
 * vtkPointCloudFilter vtkRadiusOutlierRemoval vtkExtractPoints
 * vtkThresholdPoints
*/

#ifndef vtkStatisticalOutlierRemoval_h
#define vtkStatisticalOutlierRemoval_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPointCloudFilter.h"

class vtkAbstractPointLocator;
class vtkPointSet;


class VTKFILTERSPOINTS_EXPORT vtkStatisticalOutlierRemoval : public vtkPointCloudFilter
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static vtkStatisticalOutlierRemoval *New();
  vtkTypeMacro(vtkStatisticalOutlierRemoval,vtkPointCloudFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

  //@{
  /**
   * For each point sampled, specify the number of the closest, surrounding
   * points used to compute statistics. By default 25 points are used. Smaller
   * numbers may speed performance.
   */
  vtkSetClampMacro(SampleSize,int,1,VTK_INT_MAX);
  vtkGetMacro(SampleSize,int);
  //@}

  //@{
  /**
   * The filter uses this specified standard deviation factor to extract
   * points. By default, points within 1.0 standard deviations (i.e., a
   * StandardDeviationFactor=1.0) of the mean distance to neighboring
   * points are retained.
   */
  vtkSetClampMacro(StandardDeviationFactor,double,0.0,VTK_FLOAT_MAX);
  vtkGetMacro(StandardDeviationFactor,double);
  //@}

  //@{
  /**
   * Specify a point locator. By default a vtkStaticPointLocator is
   * used. The locator performs efficient searches to locate points
   * surroinding a sample point.
   */
  void SetLocator(vtkAbstractPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkAbstractPointLocator);
  //@}

  //@{
  /**
   * After execution, return the value of the computed mean. Before execution
   * the value returned is invalid.
   */
  vtkSetClampMacro(ComputedMean,double,0.0,VTK_FLOAT_MAX);
  vtkGetMacro(ComputedMean,double);
  //@}

  //@{
  /**
   * After execution, return the value of the computed sigma (standard
   * deviation). Before execution the value returned is invalid.
   */
  vtkSetClampMacro(ComputedStandardDeviation,double,0.0,VTK_FLOAT_MAX);
  vtkGetMacro(ComputedStandardDeviation,double);
  //@}

protected:
  vtkStatisticalOutlierRemoval();
  ~vtkStatisticalOutlierRemoval();

  int SampleSize;
  double StandardDeviationFactor;
  vtkAbstractPointLocator *Locator;

  // Derived quantities
  double ComputedMean;
  double ComputedStandardDeviation;

  // All derived classes must implement this method. Note that a side effect of
  // the class is to populate the PointMap. Zero is returned if there is a failure.
  virtual int FilterPoints(vtkPointSet *input);

private:
  vtkStatisticalOutlierRemoval(const vtkStatisticalOutlierRemoval&) VTK_DELETE_FUNCTION;
  void operator=(const vtkStatisticalOutlierRemoval&) VTK_DELETE_FUNCTION;

};

#endif
