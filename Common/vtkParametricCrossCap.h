/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricCrossCap.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParametricCrossCap - Generate a cross-cap.
// .SECTION Description
// vtkParametricCrossCap generates a cross-cap which is a 
// non-orientable self-intersecting single-sided surface.
// This is one possible image of a projective plane in three-space.
// .SECTION Thanks
// Andrew Maclean a.maclean@cas.edu.au for 
// creating and contributing the class.
//
#ifndef __vtkParametricCrossCap_h
#define __vtkParametricCrossCap_h

#include "vtkParametricFunction.h"

class VTK_COMMON_EXPORT vtkParametricCrossCap : public vtkParametricFunction
{
public:

  vtkTypeRevisionMacro(vtkParametricCrossCap,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Construct a cross-cap with the following parameters:
  // MinimumU = 0, MaximumU = Pi,
  // MinimumV = 0, MaximumV = Pi, 
  // JoinU = 1, JoinV = 1,
  // TwistU = 1, TwistV = 1; 
  // ClockwiseOrdering = 1, 
  // DerivativesAvailable = 1
  static vtkParametricCrossCap *New();

  // Description
  // Return the parametric dimension of the class.
  virtual int GetDimension() {return 2;}

  // Description:
  // A cross-cap is a self intersecting single-sided surface.
  //
  // The parametric form of the equations for a cross cap are:
  // - x = cos(u) * sin(2*v) 
  // - y = sin(u) * sin(2*v) 
  // - z = cos(v) * cos(v) - cos(u) * cos(u) * sin(v) * sin(v) 
  //
  // for: 0 <= u <= PI  and 0 <= v <=  PI 
  //
  // Derivatives are:
  // - d(x(u,v))/du = -sin(u) * sin(2*v) = -y
  // - d(x(u,v))/dv = 2 * cos(u) * cos(2*v)
  // - d(y(u,v))/du = cos(u) * sin(2*v) = x
  // - d(y(u,v))/dv = 2 * sin(u) * cos(2*v)
  // - d(z(u,v))/du = 2 * cos(u) * sin(u) * sin(v) * sin(v)
  // - d(z(u,v))/dv = -2 * cos(v) * sin(v) * (1 + cos(u) * cos(u))
  //
  // Let Du = (dx/du, dy/du, dz/du)
  //
  // Let Dv = (dx/dv, dy/dv, dz/dv)
  //
  // Then the normal n = Du X Dv
  //
  //
  // This function performs the mapping fn(u,v)->(x,y,x), returning it
  // as Pt. It also returns the partial derivatives Du and Dv.
  // Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)
  void Evaluate(double uvw[3], double Pt[3], double Duvw[9]);

  // Description:
  // Calculate a user defined scalar using one or all of uvw,Pt,Duvw.
  //
  // uvw are the parameters with Pt being the the cartesian point, 
  // Duvw are the derivatives of this point with respect to u, v and w.
  // Pt, Duvw are obtained from Evaluate().
  //
  // This function is only called if the ScalarMode has the value
  // vtkParametricTriangulator::userDefined
  //
  // If the user does not need to calculate a scalar, then the 
  // instantiated function should return zero. 
  //
  double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]);

protected:
  vtkParametricCrossCap();
  ~vtkParametricCrossCap();

private:
  vtkParametricCrossCap(const vtkParametricCrossCap&);  // Not implemented.
  void operator=(const vtkParametricCrossCap&);  // Not implemented.
};

#endif
