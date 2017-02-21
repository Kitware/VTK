/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitPolyDataDistance.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImplicitPolyDataDistance
 *
 *
 * Implicit function that computes the distance from a point x to the
 * nearest point p on an input vtkPolyData. The sign of the function
 * is set to the sign of the dot product between the angle-weighted
 * pseudonormal at the nearest surface point and the vector x - p.
 * Points interior to the geometry have a negative distance, points on
 * the exterior have a positive distance, and points on the input
 * vtkPolyData have a distance of zero. The gradient of the function
 * is the angle-weighted pseudonormal at the nearest point.
 *
 * Baerentzen, J. A. and Aanaes, H. (2005). Signed distance
 * computation using the angle weighted pseudonormal. IEEE
 * Transactions on Visualization and Computer Graphics, 11:243-253.
 *
 * This code was contributed in the VTK Journal paper:
 * "Boolean Operations on Surfaces in VTK Without External Libraries"
 * by Cory Quammen, Chris Weigle C., Russ Taylor
 * http://hdl.handle.net/10380/3262
 * http://www.midasjournal.org/browse/publication/797
*/

#ifndef vtkImplicitPolyDataDistance_h
#define vtkImplicitPolyDataDistance_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkImplicitFunction.h"

class vtkCellLocator;
class vtkPolyData;

class VTKFILTERSCORE_EXPORT vtkImplicitPolyDataDistance : public vtkImplicitFunction
{
public:
  static vtkImplicitPolyDataDistance *New();
  vtkTypeMacro(vtkImplicitPolyDataDistance,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Return the MTime also considering the Input dependency.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  /**
   * Evaluate plane equation of nearest triangle to point x[3].
   */
  using vtkImplicitFunction::EvaluateFunction;
  double EvaluateFunction(double x[3]) VTK_OVERRIDE;

  /**
   * Evaluate function gradient of nearest triangle to point x[3].
   */
  void EvaluateGradient(double x[3], double g[3]) VTK_OVERRIDE;

  /**
   * Evaluate plane equation of nearest triangle to point x[3] and provides closest point on an input vtkPolyData.
   */
  double EvaluateFunctionAndGetClosestPoint (double x[3], double closestPoint[3]);

  /**
   * Set the input vtkPolyData used for the implicit function
   * evaluation.  Passes input through an internal instance of
   * vtkTriangleFilter to remove vertices and lines, leaving only
   * triangular polygons for evaluation as implicit planes.
   */
  void SetInput(vtkPolyData *input);

  //@{
  /**
   * Set/get the function value to use if no input vtkPolyData
   * specified.
   */
  vtkSetMacro(NoValue, double);
  vtkGetMacro(NoValue, double);
  //@}

  //@{
  /**
   * Set/get the function gradient to use if no input vtkPolyData
   * specified.
   */
  vtkSetVector3Macro(NoGradient, double);
  vtkGetVector3Macro(NoGradient, double);
  //@}

  //@{
  /**
   * Set/get the closest point to use if no input vtkPolyData
   * specified.
   */
  vtkSetVector3Macro(NoClosestPoint, double);
  vtkGetVector3Macro(NoClosestPoint, double);
  //@}

  //@{
  /**
   * Set/get the tolerance usued for the locator.
   */
  vtkGetMacro(Tolerance, double);
  vtkSetMacro(Tolerance, double);
  //@}

protected:
  vtkImplicitPolyDataDistance();
  ~vtkImplicitPolyDataDistance() VTK_OVERRIDE;

  /**
   * Create default locator. Used to create one when none is specified.
   */
  void CreateDefaultLocator(void);

  double SharedEvaluate(double x[3], double g[3], double p[3]);

  double NoGradient[3];
  double NoClosestPoint[3];
  double NoValue;
  double Tolerance;

  vtkPolyData *Input;
  vtkCellLocator *Locator;

private:
  vtkImplicitPolyDataDistance(const vtkImplicitPolyDataDistance&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImplicitPolyDataDistance&) VTK_DELETE_FUNCTION;
};

#endif
