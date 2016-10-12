/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCACurvatureEstimation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPCACurvatureEstimation
 * @brief   generate curvature estimates using
 * principal component analysis
 *
 *
 * vtkPCACurvatureEstimation generates point normals using PCA (principal
 * component analysis).  Basically this estimates a local tangent plane
 * around sample point p by considering a small neighborhood of points
 * around p, and fitting a plane to the neighborhood (via PCA). A good
 * introductory reference is Hoppe's "Surface reconstruction from
 * unorganized points."
 *
 * To use this filter, sepcify a neighborhood size. This may have to be set
 * via experimentation. Optionally a point locator can be specified (instead
 * of the default locator), which is used to accelerate searches around a
 * sample point. Finally, the user should specify how to generate
 * consistently-oriented normals. As computed by PCA, normals may point in
 * +/- orientation, which may not be consistent with neighboring normals.
 *
 * The output of this filter is the same as the input except that a normal
 * per point is produced. (Note that these are unit normals.) While any
 * vtkPointSet type can be provided as input, the output is represented by an
 * explicit representation of points via a vtkPolyData. This output polydata
 * will populate its instance of vtkPoints, but no cells will be defined
 * (i.e., no vtkVertex or vtkPolyVertex are contained in the output).
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkPCANormalEstimation
*/

#ifndef vtkPCACurvatureEstimation_h
#define vtkPCACurvatureEstimation_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkAbstractPointLocator;


class VTKFILTERSPOINTS_EXPORT vtkPCACurvatureEstimation : public vtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static vtkPCACurvatureEstimation *New();
  vtkTypeMacro(vtkPCACurvatureEstimation,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

  //@{
  /**
   * For each sampled point, specify the number of the closest, surrounding
   * points used to estimate the normal (the so called k-neighborhood). By
   * default 25 points are used. Smaller numbers may speed performance at the
   * cost of accuracy.
   */
  vtkSetClampMacro(SampleSize,int,1,VTK_INT_MAX);
  vtkGetMacro(SampleSize,int);
  //@}

  //@{
  /**
   * Specify a point locator. By default a vtkStaticPointLocator is
   * used. The locator performs efficient searches to locate points
   * around a sample point.
   */
  void SetLocator(vtkAbstractPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkAbstractPointLocator);
  //@}

protected:
  vtkPCACurvatureEstimation();
  ~vtkPCACurvatureEstimation();

  // IVars
  int SampleSize;
  vtkAbstractPointLocator *Locator;

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkPCACurvatureEstimation(const vtkPCACurvatureEstimation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPCACurvatureEstimation&) VTK_DELETE_FUNCTION;

};

#endif
