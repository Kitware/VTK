/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricCrossCap.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkParametricCrossCap
 * @brief   Generate a cross-cap.
 *
 * vtkParametricCrossCap generates a cross-cap which is a
 * non-orientable self-intersecting single-sided surface.
 * This is one possible image of a projective plane in three-space.
 *
 * For further information about this surface, please consult the
 * technical description "Parametric surfaces" in http://www.vtk.org/publications
 * in the "VTK Technical Documents" section in the VTk.org web pages.
 *
 * @par Thanks:
 * Andrew Maclean andrew.amaclean@gmail.com for creating and contributing the
 * class.
 *
*/

#ifndef vtkParametricCrossCap_h
#define vtkParametricCrossCap_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkParametricFunction.h"

class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkParametricCrossCap : public vtkParametricFunction
{
public:

  vtkTypeMacro(vtkParametricCrossCap,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct a cross-cap with the following parameters:
   * MinimumU = 0, MaximumU = Pi,
   * MinimumV = 0, MaximumV = Pi,
   * JoinU = 1, JoinV = 1,
   * TwistU = 1, TwistV = 1;
   * ClockwiseOrdering = 1,
   * DerivativesAvailable = 1
   */
  static vtkParametricCrossCap *New();

  /**
   * Return the parametric dimension of the class.
   */
  int GetDimension() VTK_OVERRIDE {return 2;}

  /**
   * A cross-cap.

   * This function performs the mapping \f$f(u,v) \rightarrow (x,y,x)\f$, returning it
   * as Pt. It also returns the partial derivatives Du and Dv.
   * \f$Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)\f$ .
   * Then the normal is \f$N = Du X Dv\f$ .
   */
  void Evaluate(double uvw[3], double Pt[3], double Duvw[9]) VTK_OVERRIDE;

  /**
   * Calculate a user defined scalar using one or all of uvw, Pt, Duvw.

   * uvw are the parameters with Pt being the the cartesian point,
   * Duvw are the derivatives of this point with respect to u, v and w.
   * Pt, Duvw are obtained from Evaluate().

   * This function is only called if the ScalarMode has the value
   * vtkParametricFunctionSource::SCALAR_FUNCTION_DEFINED

   * If the user does not need to calculate a scalar, then the
   * instantiated function should return zero.
   */
  double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]) VTK_OVERRIDE;

protected:
  vtkParametricCrossCap();
  ~vtkParametricCrossCap() VTK_OVERRIDE;

private:
  vtkParametricCrossCap(const vtkParametricCrossCap&) VTK_DELETE_FUNCTION;
  void operator=(const vtkParametricCrossCap&) VTK_DELETE_FUNCTION;
};

#endif
