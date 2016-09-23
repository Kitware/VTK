/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricBour.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkParametricBour
 * @brief   Generate Bour's minimal surface.
 *
 * vtkParametricBour generates Bour's minimal surface parametrically. More
 * information can be found at
 * <a href="http://en.wikipedia.org/wiki/Bour%27s_minimal_surface">Wikipedia</a>.
 * @par Thanks:
 * Tim Meehan
*/

#ifndef vtkParametricBour_h
#define vtkParametricBour_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkParametricFunction.h"

class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkParametricBour : public vtkParametricFunction
{
public:

  vtkTypeMacro(vtkParametricBour,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct Bour's minimal surface with the following parameters:
   * (MinimumU, MaximumU) = (0., 1.),
   * (MinimumV, MaximumV) = (0., 4.*pi),
   * JoinU = 0, JoinV = 0,
   * TwistU = 0, TwistV = 0;
   * ClockwiseOrdering = 1,
   * DerivativesAvailable = 1,
   */
  static vtkParametricBour *New();

  /**
   * Return the parametric dimension of the class.
   */
  int GetDimension() VTK_OVERRIDE {return 2;}

  /**
   * Bour's minimal surface.

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
  vtkParametricBour();
  ~vtkParametricBour() VTK_OVERRIDE;

private:
  vtkParametricBour(const vtkParametricBour&) VTK_DELETE_FUNCTION;
  void operator=(const vtkParametricBour&) VTK_DELETE_FUNCTION;
};

#endif
