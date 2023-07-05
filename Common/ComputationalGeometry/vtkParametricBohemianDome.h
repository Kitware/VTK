// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkParametricBohemianDome
 * @brief   Generate a Bohemian dome.
 *
 * vtkParametricBohemianDome generates a parametric Bohemian dome. The Bohemian
 * dome is a quartic surface, and is described in much better detail at
 * <a href="https://www.math.hmc.edu/math142-01/mellon/curves_and_surfaces/surfaces/bohdom.html">HMC
 * page</a>.
 * @warning
 * I haven't set any restrictions on the A, B, or C values.
 * @par Thanks:
 * Tim Meehan
 */

#ifndef vtkParametricBohemianDome_h
#define vtkParametricBohemianDome_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkParametricFunction.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkParametricBohemianDome : public vtkParametricFunction
{
public:
  vtkTypeMacro(vtkParametricBohemianDome, vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Construct a Bohemian dome surface with the following parameters:
   */
  vtkGetMacro(A, double);
  vtkSetMacro(A, double);
  ///@}

  vtkGetMacro(B, double);
  vtkSetMacro(B, double);

  vtkGetMacro(C, double);
  vtkSetMacro(C, double);

  // (MinimumU, MaximumU) = (-pi, pi),
  // (MinimumV, MaximumV) = (-pi, pi),
  // JoinU = 1, JoinV = 1,
  // TwistU = 0, TwistV = 0;
  // ClockwiseOrdering = 0,
  // DerivativesAvailable = 1,
  static vtkParametricBohemianDome* New();

  /**
   * Return the parametric dimension of the class.
   */
  int GetDimension() override { return 2; }

  /**
   * BohemianDome surface.

   * This function performs the mapping \f$f(u,v) \rightarrow (x,y,x)\f$, returning it
   * as Pt. It also returns the partial derivatives Du and Dv.
   * \f$Pt = (x, y, z), D_u\vec{f} = (dx/du, dy/du, dz/du), D_v\vec{f} = (dx/dv, dy/dv, dz/dv)\f$ .
   * Then the normal is \f$N = D_u\vec{f} \times D_v\vec{f}\f$ .
   */
  void Evaluate(double uvw[3], double Pt[3], double Duvw[9]) override;

  /**
   * Calculate a user defined scalar using one or all of uvw, Pt, Duvw.
   * This method simply returns 0.
   */
  double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]) override;

protected:
  vtkParametricBohemianDome();
  ~vtkParametricBohemianDome() override;

  // Variables
  double A;
  double B;
  double C;

private:
  vtkParametricBohemianDome(const vtkParametricBohemianDome&) = delete;
  void operator=(const vtkParametricBohemianDome&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
