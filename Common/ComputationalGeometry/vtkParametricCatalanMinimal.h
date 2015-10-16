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
// .NAME vtkParametricCatalanMinimal - Generate Catalan's minimal surface.
// .SECTION Description
// vtkParametricCatalanMinimal generates Catalan's minimal surface
// parametrically. This minimal surface contains the cycloid as a geodesic.
// More information about it can be found at
// <a href="https://en.wikipedia.org/wiki/Catalan%27s_minimal_surface">Wikipedia</a>.
// .SECTION Thanks
// Tim Meehan

#ifndef __vtkParametricCatalanMinimal_h
#define __vtkParametricCatalanMinimal_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkParametricFunction.h"

class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkParametricCatalanMinimal : public vtkParametricFunction
{
public:

  vtkTypeMacro(vtkParametricCatalanMinimal,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct Catalan's minimal surface with the following parameters:
  // (MinimumU, MaximumU) = (-4.*pi, 4.*pi),
  // (MinimumV, MaximumV) = (-1.5, 1.5),
  // JoinU = 0, JoinV = 0,
  // TwistU = 0, TwistV = 0;
  // ClockwiseOrdering = 1,
  // DerivativesAvailable = 1,
  static vtkParametricCatalanMinimal *New();

  // Description
  // Return the parametric dimension of the class.
  virtual int GetDimension() {return 2;}

  // Description:
  // Catalan's minimal surface.
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
  vtkParametricCatalanMinimal();
  ~vtkParametricCatalanMinimal();

private:
  vtkParametricCatalanMinimal(const vtkParametricCatalanMinimal&);  // Not implemented.
  void operator=(const vtkParametricCatalanMinimal&);  // Not implemented.
};

#endif
