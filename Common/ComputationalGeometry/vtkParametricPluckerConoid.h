/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricPluckerConoid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkParametricPluckerConoid
 * @brief   Generate Plucker's conoid surface.
 *
 * vtkParametricPluckerConoid generates Plucker's conoid surface parametrically.
 * Plucker's conoid is a ruled surface, named after Julius Plucker. It is
 * possible to set the number of folds in this class via the parameter 'N'.
 *
 * For more information, see the Wikipedia page on
 * <a href="https://en.wikipedia.org/wiki/Pl%c3%bccker%27s_conoid">Plucker's Conoid</a>.
 * @warning
 * I haven't done any special checking on the number of folds parameter, N.
 * @par Thanks:
 * Tim Meehan
*/

#ifndef vtkParametricPluckerConoid_h
#define vtkParametricPluckerConoid_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkParametricFunction.h"

class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkParametricPluckerConoid :
  public vtkParametricFunction
{
  public:


    vtkTypeMacro(vtkParametricPluckerConoid, vtkParametricFunction);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    //@{
    /**
     * This is the number of folds in the conoid.
     */
    vtkGetMacro(N, int);
    vtkSetMacro(N, int);
    //@}

    /**
     * Construct Plucker's conoid surface with the following parameters:
     * (MinimumU, MaximumU) = (0., 3.),
     * (MinimumV, MaximumV) = (0., 2*pi),
     * JoinU = 0, JoinV = 0,
     * TwistU = 0, TwistV = 0;
     * ClockwiseOrdering = 0,
     * DerivativesAvailable = 1,
     */
    static vtkParametricPluckerConoid *New();

    /**
     * Return the parametric dimension of the class.
     */
    int GetDimension() override {return 2;}

    /**
     * Plucker's conoid surface.

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
    double EvaluateScalar(double uvw[3], double Pt[3],
                          double Duvw[9]) override;

  protected:
    vtkParametricPluckerConoid();
    ~vtkParametricPluckerConoid() override;

    // Variables
    int N;

  private:
    vtkParametricPluckerConoid(const vtkParametricPluckerConoid&) = delete;
    void operator=(const vtkParametricPluckerConoid&) = delete;
};

#endif
