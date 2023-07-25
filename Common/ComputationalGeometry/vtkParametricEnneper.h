// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkParametricEnneper
 * @brief   Generate Enneper's surface.
 *
 * vtkParametricEnneper generates Enneper's surface.
 * Enneper's surface is a self-intersecting minimal surface
 * possessing constant negative Gaussian curvature
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

#ifndef vtkParametricEnneper_h
#define vtkParametricEnneper_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkParametricFunction.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkParametricEnneper : public vtkParametricFunction
{
public:
  vtkTypeMacro(vtkParametricEnneper, vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct Enneper's surface with the following parameters:
   * MinimumU = -2, MaximumU = 2,
   * MinimumV = -2, MaximumV = 2,
   * JoinU = 0, JoinV = 0,
   * TwistU = 0, TwistV = 0,
   * ClockwiseOrdering = 0,
   * DerivativesAvailable = 1
   */
  static vtkParametricEnneper* New();

  /**
   * Return the parametric dimension of the class.
   */
  int GetDimension() override { return 2; }

  /**
   * Enneper's surface.

   * This function performs the mapping \f$f(u,v) \rightarrow (x,y,x)\f$, returning it
   * as Pt. It also returns the partial derivatives Du and Dv.
   * \f$Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)\f$ .
   * Then the normal is \f$N = Du X Dv\f$ .
   */
  void Evaluate(double uvw[3], double Pt[3], double Duvw[9]) override;

  /**
   * Calculate a user defined scalar using one or all of uvw, Pt, Duvw.

   * uv are the parameters with Pt being the cartesian point,
   * Duvw are the derivatives of this point with respect to u, v and w.
   * Pt, Duvw are obtained from Evaluate().

   * This function is only called if the ScalarMode has the value
   * vtkParametricFunctionSource::SCALAR_FUNCTION_DEFINED

   * If the user does not need to calculate a scalar, then the
   * instantiated function should return zero.
   */
  double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]) override;

protected:
  vtkParametricEnneper();
  ~vtkParametricEnneper() override;

private:
  vtkParametricEnneper(const vtkParametricEnneper&) = delete;
  void operator=(const vtkParametricEnneper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
