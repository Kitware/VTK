/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricPseudosphere.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParametricPseudosphere - Generate a pseudosphere.
// .SECTION Description
// vtkParametricPseudosphere generates a parametric pseudosphere. The
// pseudosphere is generated as a surface of revolution of the tractrix about
// it's asymptote, and is a surface of constant negative Gaussian curvature.
// You can find out more about this interesting surface at
// <a href="http://mathworld.wolfram.com/Pseudosphere.html">Math World</a>.
// .SECTION Thanks
// Tim Meehan

#ifndef __vtkParametricPseudosphere_h
#define __vtkParametricPseudosphere_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkParametricFunction.h"

class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkParametricPseudosphere : public vtkParametricFunction
{
public:

  vtkTypeMacro(vtkParametricPseudosphere,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct a pseudosphere surface with the following parameters:
  // (MinimumU, MaximumU) = (-5., 5.),
  // (MinimumV, MaximumV) = (-pi, pi),
  // JoinU = 0, JoinV = 1,
  // TwistU = 0, TwistV = 0;
  // ClockwiseOrdering = 1,
  // DerivativesAvailable = 1,
  static vtkParametricPseudosphere *New();

  // Description
  // Return the parametric dimension of the class.
  virtual int GetDimension() {return 2;}

  // Description:
  // Pseudosphere surface.
  //
  // This function performs the mapping \f$f(u,v) \rightarrow (x,y,x)\f$, returning it
  // as Pt. It also returns the partial derivatives Du and Dv.
  // \f$Pt = (x, y, z), D_u\vec{f} = (dx/du, dy/du, dz/du), D_v\vec{f} = (dx/dv, dy/dv, dz/dv)\f$ .
  // Then the normal is \f$N = D_u\vec{f} \times D_v\vec{f}\f$ .
  virtual void Evaluate(double uvw[3], double Pt[3], double Duvw[9]);

  // Description:
  // Calculate a user defined scalar using one or all of uvw, Pt, Duvw.
  // This method simply returns 0.
  virtual double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]);

protected:
  vtkParametricPseudosphere();
  ~vtkParametricPseudosphere();

private:
  vtkParametricPseudosphere(const vtkParametricPseudosphere&);  // Not implemented.
  void operator=(const vtkParametricPseudosphere&);  // Not implemented.
};

#endif
