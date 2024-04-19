// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkParametricSuperToroid
 * @brief   Generate a supertoroid.
 *
 * vtkParametricSuperToroid generates a supertoroid.  Essentially a
 * supertoroid is a torus with the sine and cosine terms raised to a power.
 * A supertoroid is a versatile primitive that is controlled by four
 * parameters r0, r1, n1 and n2. r0, r1 determine the type of torus whilst
 * the value of n1 determines the shape of the torus ring and n2 determines
 * the shape of the cross section of the ring. It is the different values of
 * these powers which give rise to a family of 3D shapes that are all
 * basically toroidal in shape.
 *
 * For further information about this surface, please consult the
 * technical description "Parametric surfaces" in http://www.vtk.org/publications
 * in the "VTK Technical Documents" section in the VTk.org web pages.
 *
 * Also see: http://paulbourke.net/geometry/torus/#super.
 *
 * @warning
 * Care needs to be taken specifying the bounds correctly. You may need to
 * carefully adjust MinimumU, MinimumV, MaximumU, MaximumV.
 *
 * @par Thanks:
 * Andrew Maclean andrew.amaclean@gmail.com for creating and contributing the
 * class.
 *
 */

#ifndef vtkParametricSuperToroid_h
#define vtkParametricSuperToroid_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkParametricFunction.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkParametricSuperToroid : public vtkParametricFunction
{
public:
  vtkTypeMacro(vtkParametricSuperToroid, vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct a supertoroid with the following parameters:
   * MinimumU = 0, MaximumU = 2*Pi,
   * MinimumV = 0, MaximumV = 2*Pi,
   * JoinU = 0, JoinV = 0,
   * TwistU = 0, TwistV = 0,
   * ClockwiseOrdering = 1,
   * DerivativesAvailable = 0,
   * RingRadius = 1, CrossSectionRadius = 0.5,
   * N1 = 1, N2 = 1, XRadius = 1,
   * YRadius = 1, ZRadius = 1, a torus in this case.
   */
  static vtkParametricSuperToroid* New();

  /**
   * Return the parametric dimension of the class.
   */
  int GetDimension() override { return 2; }

  ///@{
  /**
   * Set/Get the radius from the center to the middle of the ring of the
   * supertoroid. Default is 1.
   */
  vtkSetMacro(RingRadius, double);
  vtkGetMacro(RingRadius, double);
  ///@}

  ///@{
  /**
   * Set/Get the radius of the cross section of ring of the supertoroid.
   * Default = 0.5.
   */
  vtkSetMacro(CrossSectionRadius, double);
  vtkGetMacro(CrossSectionRadius, double);
  ///@}

  ///@{
  /**
   * Set/Get the scaling factor for the x-axis. Default is 1.
   */
  vtkSetMacro(XRadius, double);
  vtkGetMacro(XRadius, double);
  ///@}

  ///@{
  /**
   * Set/Get the scaling factor for the y-axis. Default is 1.
   */
  vtkSetMacro(YRadius, double);
  vtkGetMacro(YRadius, double);
  ///@}

  ///@{
  /**
   * Set/Get the scaling factor for the z-axis. Default is 1.
   */
  vtkSetMacro(ZRadius, double);
  vtkGetMacro(ZRadius, double);
  ///@}

  ///@{
  /**
   * Set/Get the shape of the torus ring.  Default is 1.
   */
  vtkSetMacro(N1, double);
  vtkGetMacro(N1, double);
  ///@}

  ///@{
  /**
   * Set/Get the shape of the cross section of the ring. Default is 1.
   */
  vtkSetMacro(N2, double);
  vtkGetMacro(N2, double);
  ///@}

  /**
   * A supertoroid.

   * This function performs the mapping \f$f(u,v) \rightarrow (x,y,x)\f$, returning it
   * as Pt. It also returns the partial derivatives Du and Dv.
   * \f$Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)\f$ .
   * Then the normal is \f$N = Du X Dv\f$ .
   */
  void Evaluate(double uvw[3], double Pt[3], double Duvw[9]) override;

  /**
   * Calculate a user defined scalar using one or all of uvw, Pt, Duvw.

   * uvw are the parameters with Pt being the cartesian point,
   * Duvw are the derivatives of this point with respect to u, v and w.
   * Pt, Duvw are obtained from Evaluate().

   * This function is only called if the ScalarMode has the value
   * vtkParametricFunctionSource::SCALAR_FUNCTION_DEFINED

   * If the user does not need to calculate a scalar, then the
   * instantiated function should return zero.
   */
  double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]) override;

protected:
  vtkParametricSuperToroid();
  ~vtkParametricSuperToroid() override;

  // Variables
  double RingRadius;
  double CrossSectionRadius;
  double XRadius;
  double YRadius;
  double ZRadius;
  double N1;
  double N2;

private:
  vtkParametricSuperToroid(const vtkParametricSuperToroid&) = delete;
  void operator=(const vtkParametricSuperToroid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
