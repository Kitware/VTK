/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricMobius.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParametricMobius - represent a Mobius strip
// .SECTION Description
// vtkParametricMobius represents a Mobius strip.

// .SECTION Thanks
// Andrew Maclean a.maclean@cas.edu.au for creating and contributing the
// class.
//
#ifndef __vtkParametricMobius_h
#define __vtkParametricMobius_h

#include "vtkParametricFunction.h"

class VTK_COMMON_EXPORT vtkParametricMobius : public vtkParametricFunction
{
public:
  vtkTypeRevisionMacro(vtkParametricMobius,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Construct a Mobius strip with the following parameters:
  // MinimumU = 0, MaximumU = 2*Pi+2*Pi/PointsU,
  // MinimumV = -1, MaximumV = 1, JoinU = 1, JoinV = 0,
  // TwistU = 0, TwistV = 0; Ordering = 1, DerivativesSupplied = 1,
  // Radius = 1.
  static vtkParametricMobius *New();

  // Description:
  // Set/Get the radius of the Mobius strip.
  vtkSetMacro(Radius,double);
  vtkGetMacro(Radius,double);

  // Description
  // Return the parametric dimension of the class.
  virtual int GetDimension() {return 2;}

  // Description:
  // The Mobius strip is defined as follows:
  // <pre>
  // 0 < u < 2 * pi, -1 < v < 1
  // 
  // a = 5
  // 
  // The parametric equations for a Mobius strip are:
  // - x = (a - v * sin(u/2))*sin(u)
  // - y = (a - v * sin(u/2))*cos(u)
  // - z = v * cos(u/2)
  // 
  // Derivatives:
  // - d(x(u,v)/du = -v*cos(u/2)*sin(u)/2 + y(u,v)
  // - d(x(u,v)/dv = -sin(u/2)*sin(u)
  // - d(y(u,v)/du = -v*cos(u/2)cos(u)/2 - x(u,v)
  // - d(y(u,v)/dv = -sin(u/2)*cos(u)
  // - d(z(u,v)/du = -v*sin(u/2)/2
  // - d(z(u,v)/dv = cos(u/2)
  // 
  // Let Du = (dx/du, dy/du, dz/du)
  // 
  // Let Dv = (dx/dv, dy/dv, dz/dv)
  // 
  // Then the normal n = Du X Dv.
  // </pre>
  // This function performs the mapping fn(u,v)->(x,y,x), returning it
  // as Pt. It also returns the partial derivatives Du and Dv.
  // Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)
  virtual void Evaluate(double u[3], double Pt[3], double Du[9]);

  // Description:
  // Calculate a user defined scalar using one or all of u,v,Pt,Du,Dv.
  //
  // u,v are the parameters with Pt being the the cartesian point, 
  // Du, Dv are the derivatives of this point with respect to u and v.
  // Pt, Du, Dv are obtained from fn().
  //
  // This function is only called if the ScalarMode has the value
  // vtkParametricFunction::userDefined
  //
  // If the user does not need to calculate a scalar, then the 
  // instantiated function should return zero. 
  //
  virtual double EvaluateScalar(double u[3], double Pt[3], double Du[9]);

protected:
  vtkParametricMobius();
  ~vtkParametricMobius();

  // Variables
  double Radius;

private:
  vtkParametricMobius(const vtkParametricMobius&);  // Not implemented.
  void operator=(const vtkParametricMobius&);  // Not implemented.
};

#endif
