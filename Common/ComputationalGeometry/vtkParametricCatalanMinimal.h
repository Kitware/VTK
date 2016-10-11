/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricCatalanMinimal.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkParametricCatalanMinimal
 * @brief   Generate Catalan's minimal surface.
 *
 * vtkParametricCatalanMinimal generates Catalan's minimal surface
 * parametrically. This minimal surface contains the cycloid as a geodesic.
 * More information about it can be found at
 * <a href="https://en.wikipedia.org/wiki/Catalan%27s_minimal_surface">Wikipedia</a>.
 * @par Thanks:
 * Tim Meehan
*/

#ifndef vtkParametricCatalanMinimal_h
#define vtkParametricCatalanMinimal_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkParametricFunction.h"

class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT
  vtkParametricCatalanMinimal : public vtkParametricFunction
{
  public:

    vtkTypeMacro(vtkParametricCatalanMinimal, vtkParametricFunction);
    void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

    /**
     * Construct Catalan's minimal surface with the following parameters:
     * (MinimumU, MaximumU) = (-4.*pi, 4.*pi),
     * (MinimumV, MaximumV) = (-1.5, 1.5),
     * JoinU = 0, JoinV = 0,
     * TwistU = 0, TwistV = 0;
     * ClockwiseOrdering = 0,
     * DerivativesAvailable = 1,
     */
    static vtkParametricCatalanMinimal *New();

    /**
     * Return the parametric dimension of the class.
     */
    int GetDimension() VTK_OVERRIDE {return 2;}

    /**
     * Catalan's minimal surface.

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
    double EvaluateScalar(double uvw[3], double Pt[3],
                          double Duvw[9]) VTK_OVERRIDE;

  protected:
    vtkParametricCatalanMinimal();
    ~vtkParametricCatalanMinimal() VTK_OVERRIDE;

  private:
    vtkParametricCatalanMinimal(const vtkParametricCatalanMinimal&) VTK_DELETE_FUNCTION;
    void operator=(const vtkParametricCatalanMinimal&) VTK_DELETE_FUNCTION;
};

#endif
