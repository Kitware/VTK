/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricConicSpiral.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParametricConicSpiral - generate conic spiral surfaces that resemble sea-shells.
// .SECTION Description
// vtkParametricConicSpiral generates conic spiral surfaces. These can resemble sea shells, or
// may look like a torus "eating" its own tail.
// .SECTION Thanks
// Andrew Maclean a.maclean@cas.edu.au for creating and contributing the
// class.
//
#ifndef __vtkParametricConicSpiral_h
#define __vtkParametricConicSpiral_h

#include "vtkParametricFunction.h"

class VTK_COMMON_EXPORT vtkParametricConicSpiral : public vtkParametricFunction
{
public:
  vtkTypeRevisionMacro(vtkParametricConicSpiral,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Construct a conic spiral surface with the following parameters: 
  // MinimumU = 0, MaximumU = 2Pi, 
  // MinimumV = 0, MaximumV = 2Pi, 
  // JoinU = 0, JoinV = 0, 
  // TwistU = 0, TwistV = 0, 
  // ClockwiseOrdering = 1, 
  // DerivativesAvailable = 1, 
  // A = 0.2, B = 1.0, C = 0.1, N = 2.
  static vtkParametricConicSpiral *New();

  // Description
  // Return the parametric dimension of the class.
  virtual int GetDimension() {return 2;}

   // Description:
  // Set/Get the scale factor. 
  // Default = 0.2
  vtkSetMacro(A,double);
  vtkGetMacro(A,double);

  // Description:
  // Set/Get the A function coefficient (see equation below). 
  // Default = 1.
  vtkSetMacro(B,double);
  vtkGetMacro(B,double);

  // Description:
  // Set/Get the B function coefficient (see equation below). 
  // Default = 0.1.
  vtkSetMacro(C,double);
  vtkGetMacro(C,double);

  // Description:
  // Set/Get the C function coefficient (see equation below). 
  // Default = 2.
  vtkSetMacro(N,double);
  vtkGetMacro(N,double);

  // Description:
  // A parametric representation of a conic spiral surface (i.e., a toroid
  // "eating" its own tail) is given by:
  // <pre>
  // Define:
  // -  X(u,v) = a*(1-v/(2*pi))*cos(n*v)*(1+cos(u))+c*cos(n*v)
  // -  Y(u,v) = a*(1-v/(2*pi))*sin(n*v)*(1+cos(u))+c*sin(n*v)
  // -  Z(u,v) = b*v/(2*pi)+a*(1-v/(2*pi))*sin(u)
  //
  // Where:  a=0.2,b=1,c=0.1,n=2,u=0..2*pi},v=0..2*pi
  //
  // Then
  // - S(u,v) = (X(u,v),Y(u,v),Z(u,v)) defines the surface. 
  //
  // The derivatives are given by:
  // - d(X(u,v)/du = -a*(1-1/2*v/pi)*cos(n*v)*sin(u)
  // - d(X(u,v)/dv = -1/2*a/pi*cos(n*v)*(1+cos(u))-a*(1-1/2*v/pi)*sin(n*v)*n*(1+cos(u))-c*sin(n*v)*n
  // - d(Y(u,v)/du = -a*(1-1/2*v/pi)*sin(n*v)*sin(u)
  // - d(Y(u,v)/dv = -1/2*a/Pi*sin(n*v)*(1+cos(u))+a*(1-1/2*v/pi)*cos(n*v)*n*(1+cos(u))+c*cos(n*v)*n
  // - d(Z(u,v)/du = a*(1-1/2*v/pi)*cos(u)
  // - d(Z(u,v)/dv = 1/2*b/pi-1/2*a/pi*sin(u)
  //
  // Let Du = (dy/du, dy/du, dy/du). Let Dv = (dx/dv, dy/dv, dz/dv). Then the normal n = Du X Dv.
  // </pre>
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
  // vtkParametricFunctionSource::FunctionDefined.
  //
  // If the user does not need to calculate a scalar, then the 
  // instantiated function should return zero. 
  double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]);

protected:
  vtkParametricConicSpiral();
  ~vtkParametricConicSpiral();

  // Variables
  double A;
  double B;
  double C;
  double N;

private:
  vtkParametricConicSpiral(const vtkParametricConicSpiral&);  // Not implemented.
  void operator=(const vtkParametricConicSpiral&);  // Not implemented.
};

#endif
