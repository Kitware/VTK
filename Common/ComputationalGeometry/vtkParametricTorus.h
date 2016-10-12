/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricTorus.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkParametricTorus
 * @brief   Generate a torus.
 *
 * vtkParametricTorus generates a torus.
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

#ifndef vtkParametricTorus_h
#define vtkParametricTorus_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkParametricFunction.h"

class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkParametricTorus : public vtkParametricFunction
{

public:
  vtkTypeMacro(vtkParametricTorus,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct a torus with the following parameters:
   * MinimumU = 0, MaximumU = 2*Pi,
   * MinimumV = 0, MaximumV = 2*Pi,
   * JoinU = 1, JoinV = 1,
   * TwistU = 0, TwistV = 0,
   * ClockwiseOrdering = 1,
   * DerivativesAvailable = 1,
   * RingRadius = 1, CrossSectionRadius = 0.5.
   */
  static vtkParametricTorus *New();

  //@{
  /**
   * Set/Get the radius from the center to the middle of the ring of the
   * torus. Default is 1.0.
   */
  vtkSetMacro(RingRadius,double);
  vtkGetMacro(RingRadius,double);
  //@}

  //@{
  /**
   * Set/Get the radius of the cross section of ring of the torus. Default is 0.5.
   */
  vtkSetMacro(CrossSectionRadius,double);
  vtkGetMacro(CrossSectionRadius,double);
  //@}

  /**
   * Return the parametric dimension of the class.
   */
  int GetDimension() VTK_OVERRIDE {return 2;}

  /**
   * A torus.

   * This function performs the mapping \f$f(u,v) \rightarrow (x,y,x)\f$, returning it
   * as Pt. It also returns the partial derivatives Du and Dv.
   * \f$Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)\f$.
   * Then the normal is \f$N = Du X Dv\f$.
   */
  void Evaluate(double uvw[3], double Pt[3], double Duvw[9]) VTK_OVERRIDE;

  /**
   * Calculate a user defined scalar using one or all of uvw, Pt, Duvw.

   * uvw are the parameters with Pt being the the Cartesian point,
   * Duvw are the derivatives of this point with respect to u, v and w.
   * Pt, Duvw are obtained from Evaluate().

   * This function is only called if the ScalarMode has the value
   * vtkParametricFunctionSource::SCALAR_FUNCTION_DEFINED

   * If the user does not need to calculate a scalar, then the
   * instantiated function should return zero.
   */
  double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]) VTK_OVERRIDE;

protected:
  vtkParametricTorus();
  ~vtkParametricTorus() VTK_OVERRIDE;

  // Variables
  double RingRadius;
  double CrossSectionRadius;

private:
  vtkParametricTorus(const vtkParametricTorus&) VTK_DELETE_FUNCTION;
  void operator=(const vtkParametricTorus&) VTK_DELETE_FUNCTION;
};

#endif
