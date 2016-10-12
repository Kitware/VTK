/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricKuen.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkParametricKuen
 * @brief   Generate Kuens' surface.
 *
 * vtkParametricKuen generates Kuens' surface. This surface has a constant
 * negative gaussian curvature. For more information about this surface, see
 * Dr. O'Niell's page at the
 * <a href="http://www.math.ucla.edu/~bon/kuen.html">UCLA Mathematics Department</a>.
 * @par Thanks:
 * Tim Meehan
*/

#ifndef vtkParametricKuen_h
#define vtkParametricKuen_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkParametricFunction.h"

class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkParametricKuen : public vtkParametricFunction
{
public:

  vtkTypeMacro(vtkParametricKuen,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct Kuen's surface with the following parameters:
   * (MinimumU, MaximumU) = (-3*pi, 3*pi),
   * (MinimumV, MaximumV) = (0., pi),
   * JoinU = 0, JoinV = 0,
   * TwistU = 0, TwistV = 0;
   * ClockwiseOrdering = 1,
   * DerivativesAvailable = 1,
   */
  static vtkParametricKuen *New();

  /**
   * Return the parametric dimension of the class.
   */
  int GetDimension() VTK_OVERRIDE {return 2;}

  /**
   * Kuen's surface.

   * This function performs the mapping \f$f(u,v) \rightarrow (x,y,x)\f$, returning it
   * as Pt. It also returns the partial derivatives Du and Dv.
   * \f$Pt = (x, y, z), D_u\vec{f} = (dx/du, dy/du, dz/du), D_v\vec{f} = (dx/dv, dy/dv, dz/dv)\f$ .
   * Then the normal is \f$N = D_u\vec{f} \times D_v\vec{f}\f$ .
   */
  void Evaluate(double uvw[3], double Pt[3], double Duvw[9]) VTK_OVERRIDE;

  /**
   * Calculate a user defined scalar using one or all of uvw, Pt, Duvw.
   * This method simply returns 0.
   */
  double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]) VTK_OVERRIDE;

protected:
  vtkParametricKuen();
  ~vtkParametricKuen() VTK_OVERRIDE;

private:
  vtkParametricKuen(const vtkParametricKuen&) VTK_DELETE_FUNCTION;
  void operator=(const vtkParametricKuen&) VTK_DELETE_FUNCTION;
};

#endif
