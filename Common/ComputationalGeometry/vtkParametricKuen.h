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
#include "vtkMath.h" // for vtkMath::Pi()

class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkParametricKuen : public
  vtkParametricFunction
{
  public:

    vtkTypeMacro(vtkParametricKuen, vtkParametricFunction);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    /**
     * Construct Kuen's surface with the following parameters:
     * (MinimumU, MaximumU) = (-4.5, 4.5),
     * (MinimumV, MaximumV) = (DeltaV0, pi),
     * JoinU = 0, JoinV = 0,
     * TwistU = 0, TwistV = 0;
     * ClockwiseOrdering = 0,
     * DerivativesAvailable = 1,
     */
    static vtkParametricKuen *New();

    /**
     * Return the parametric dimension of the class.
     */
    int GetDimension() override {return 2;}

    //@{
    /**
    * Set/Get the value to use when V == 0.
    * Default is 0.05, giving the best appearance with the default settings.
    * Setting it to a value less than 0.05 extrapolates the surface
    * towards a pole in the -z direction.
    * Setting it to 0 retains the pole whose z-value is -inf.
    */
    vtkSetMacro(DeltaV0, double);
    vtkGetMacro(DeltaV0, double);
    //@}

    /**
     * Kuen's surface.

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
    vtkParametricKuen();
    ~vtkParametricKuen() override;

  private:
    vtkParametricKuen(const vtkParametricKuen&) = delete;
    void operator=(const vtkParametricKuen&) = delete;

    double DeltaV0;
};

#endif
