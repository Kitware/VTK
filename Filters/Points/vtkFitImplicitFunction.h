/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFitImplicitFunction.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFitImplicitFunction
 * @brief   extract points on the surface of an implicit function
 *
 *
 * vtkFitImplicitFunction extract points that are on the surface of an
 * implicit function (within some threshold). Implicit functions in VTK are
 * any function of the form f(x,y,z)=c, where values c==0 are considered the
 * surface of the implicit function. Typical examples of implicit functions
 * include planes, spheres, cylinders, cones, etc. plus boolean combinations
 * of these functions. In this implementation, a threshold is used to create
 * a fuzzy region considered "on" the surface. In essence, this is a very
 * poor man's RANSAC algorithm, where the user picks a function on which to
 * fit some points. Thus it is possible to use this filter to define a
 * proposed model and place it into an optimization loop to best fit it to a
 * set of points.
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
 * vtkPointCloudFilter vtkExtractPoints vtkImplicitFunction
*/

#ifndef vtkFitImplicitFunction_h
#define vtkFitImplicitFunction_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPointCloudFilter.h"

class vtkImplicitFunction;
class vtkPointSet;


class VTKFILTERSPOINTS_EXPORT vtkFitImplicitFunction : public vtkPointCloudFilter
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static vtkFitImplicitFunction *New();
  vtkTypeMacro(vtkFitImplicitFunction,vtkPointCloudFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the implicit function defining a surface on which points
   * are to be extracted.
   */
  virtual void SetImplicitFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ImplicitFunction,vtkImplicitFunction);
  //@}

  //@{
  /**
   * Specify a threshold value which defines a fuzzy extraction surface.
   * Since in this filter the implicit surface is defined as f(x,y,z)=0;
   * the extracted points are (-Threshold <= f(x,y,z) < Threshold).
   */
  vtkSetClampMacro(Threshold,double,0.0,VTK_FLOAT_MAX);
  vtkGetMacro(Threshold,double);
  //@}

  /**
   * Return the MTime taking into account changes to the implicit function.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkFitImplicitFunction();
  ~vtkFitImplicitFunction() override;

  vtkImplicitFunction *ImplicitFunction;
  double Threshold;

  // All derived classes must implement this method. Note that a side effect of
  // the class is to populate the PointMap. Zero is returned if there is a failure.
  int FilterPoints(vtkPointSet *input) override;

private:
  vtkFitImplicitFunction(const vtkFitImplicitFunction&) = delete;
  void operator=(const vtkFitImplicitFunction&) = delete;

};

#endif
