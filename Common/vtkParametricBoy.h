/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricBoy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParametricBoy - Generate Boy's surface.
// .SECTION Description
// vtkParametricBoy generates Boy's surface.
// This is a Model of the projective plane without singularities.
// It was found by Werner Boy on assignment from David Hilbert.
// .SECTION Thanks
// Andrew Maclean a.maclean@cas.edu.au for 
// creating and contributing the class.
//
#ifndef __vtkParametricBoy_h
#define __vtkParametricBoy_h

#include "vtkParametricFunction.h"

class VTK_COMMON_EXPORT vtkParametricBoy : public vtkParametricFunction
{
public:

  vtkTypeRevisionMacro(vtkParametricBoy,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Construct Boy's surface with the following parameters:
  // MinimumU = 0, MaximumU = Pi, 
  // MinimumV = 0, MaximumV = Pi, 
  // JoinU = 1, JoinV = 1, 
  // TwistU = 1, TwistV = 1; 
  // ClockwiseOrdering = 1, 
  // DerivativesAvailable = 1, 
  // ZScale = 0.125.
  static vtkParametricBoy *New();

  // Description
  // Return the parametric dimension of the class.
  virtual int GetDimension() {return 2;}

  // Description:
  // Set/Get the scale factor for the z-coordinate. 
  // Default = 1/8, giving a nice shape.
  vtkSetMacro(ZScale,double);
  vtkGetMacro(ZScale,double);

  // Description:
  // Boy's surface is a Model of the projective plane without singularities. Found
  // by Werner Boy on assignment from David Hilbert.
  //
  //
  // A parametric representation of Boy's surface
  // Define:
  // -  X(u,v) = cos(u)*sin(v)
  // -  Y(u,v) = sin(u)*sin(v)
  // -  Z(u,v) = cos(v)
  //
  // Let:
  // -  F(u,v) = 1.0/2.0*(2*X^2-Y^2-Z^2+2.0*Y*Z*(Y^2-Z^2)+Z*X*(X^2-Z^2)+X*Y*(Y^2-X^2))
  // -  G(u,v) = sqrt(3.0)/2.0*(Y^2-Z^2+(Z*X*(Z^2-X^2)+X*Y*(Y^2-X^2)))
  // -  H(u,v) = (X+Y+Z)*((X+Y+Z)^3+4.0*(Y-X)*(Z-Y)*(X-Z))
  //
  // Then
  // - S(u,v) = (F(u,v),G(u,v),H(u,v)) defines the surface. 
  //  
  // where 0 <= u <= pi, 0 <= v <= pi
  //
  // The derivatives are given by:
  // - d(F(u,v)/du = -1/2*X^4-Z^3*X+3*Y^2*X^2-3/2*Z*X^2*Y+3*Z*X*Y^2-
  //         3*Y*X-1/2*Y^4+1/2*Z^3*Y
  // - d(F(u,v)/dv = (3/2*Z^2*X^2+2*Z*X-1/2*Z^4)*cos(u)+
  //        (-2*Z*X^3+2*Z*X*Y^2+3*Z^2*Y^2-Z*Y-Z^4)*sin(u)+
  //        (-1/2*X^3+3/2*Z^2*X-Y^3+3*Z^2*Y+Z)*sin(v)
  // - d(G(u,v)/du = -1/2*3^(1/2)*X^4+3*3^(1/2)*Y^2*X^2+3/2*3^(1/2)*Z*X^2*Y+
  //        3^(1/2)*Y*X-1/2*3^(1/2)*Y^4-1/2*3^(1/2)*Z^3*Y
  // - d(G(u,v)/dv = (-3/2*3^(1/2)*Z^2*X^2+1/2*3^(1/2)*Z^4)*cos(u)+
  //        (-2*3^(1/2)*Z*X^3+2*3^(1/2)*Z*Y^2*X+3^(1/2)*Z*Y)*sin(u)+
  //        (1/2*3^(1/2)*X^3-3/2*3^(1/2)*Z^2*X+3^(1/2)*Z)*sin(v)
  // - d(H(u,v)/du = X^4+3/2*Z*X^3+3/2*Z^2*X^2+X^3*Y-3*X^2*Y^2+
  //        3*Z*X^2*Y-Y^3*X-3/2*Z*Y^3-3/2*Z^2*Y^2-Z^3*Y
  // - d(H(u,v)/dv = (1/2*Z*X^3+3/2*Z^3*X+Z^4)*cos(u)+(4*Z*X^3+3*Z*X^2*Y+
  //        9/2*Z^2*X^2+9/2*Z^2*X*Y+3*Z^3*X+1/2*Z*Y^3+3*Z^2*Y^2+
  //        3/2*Z^3*Y)*sin(u)+(-3/2*X^2*Y-3/2*Z*X^2-3/2*X*Y^2-
  //        3*Z*X*Y-3*Z^2*X-Y^3-3/2*Z*Y^2-1/2*Z^3)*sin(v)
  //
  // Let Du = (dx/du, dy/du, dz/du)
  //
  // Let Dv = (dx/dv, dy/dv, dz/dv)
  //
  // Then the normal n = Du X Dv
  //
  // For the parametric representation, a good representation is found by
  // scaling the x,y,z directions by (1,1,1/8).
  //
  // This function performs the mapping fn(u,v)->(x,y,x), returning it
  // as Pt. It also returns the partial derivatives Du and Dv.
  // \f$Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)\f$
  void Evaluate(double uvw[3], double Pt[3], double Duvw[9]);

  // Description:
  // Calculate a user defined scalar using one or all of uvw,Pt,Duvw.
  //
  // uvw are the parameters with Pt being the the cartesian point, 
  // Duvw are the derivatives of this point with respect to u, v and w.
  // Pt, Duvw are obtained from Evaluate().
  //
  // This function is only called if the ScalarMode has the value
  // vtkParametricTriangulator::FunctionDefined
  //
  // If the user does not need to calculate a scalar, then the 
  // instantiated function should return zero. 
  //
  double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]);

protected:
  vtkParametricBoy();
  ~vtkParametricBoy();

  // Variables
  double ZScale;

private:
  vtkParametricBoy(const vtkParametricBoy&);  // Not implemented.
  void operator=(const vtkParametricBoy&);  // Not implemented.
};

#endif
