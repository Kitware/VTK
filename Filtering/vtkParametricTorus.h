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
// .NAME vtkParametricTorus - a torus parametric function.
// .SECTION Description
// vtkParametricTorus generates a torus parametric function. The torus is
// parameterized between [0,2*Pi] in each of the (u,v) directions.
//
// .SECTION Thanks
// Andrew Maclean a.maclean@cas.edu.au for 
// creating and contributing the class.
//
#ifndef __vtkParametricTorus_h
#define __vtkParametricTorus_h

#include "vtkParametricFunction.h"

class VTK_FILTERING_EXPORT vtkParametricTorus : public vtkParametricFunction
{

public:
  vtkTypeRevisionMacro(vtkParametricTorus,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Construct a torus with the following parameters:
  // PointsU = 50, PointsV = 50, MinimumU = 0, MaximumU = 2*Pi,
  // MinimumV = 0, MaximumV = 2*Pi, JoinU = 1, JoinV = 1,
  // TwistU = 0, TwistV = 0; Ordering = 1, DerivativesSupplied = 1,
  // RingRadius = 1, CrossSectionRadius = 0.5.
  static vtkParametricTorus *New();

  // Description:
  // Set/Get the radius from the center to the middle of the ring of the
  // torus.  The default value is 1.0.
  vtkSetMacro(RingRadius,double);
  vtkGetMacro(RingRadius,double);

  // Description:
  // Set/Get the radius of the cross section of ring of the torus.  The default value
  // is 0.5.
  vtkSetMacro(CrossSectionRadius,double);
  vtkGetMacro(CrossSectionRadius,double);

  // Description
  // Return the parametric dimension of the class.
  virtual int GetDimension() {return 2;}

  // Description:
  // Make a torus.
  // <pre>
  // The parametric equations for a torus are:
  //   - x = (c+a*cos(v))*cos(u)
  //   - y = (c+a*cos(v))*sin(u)
  //   - z = a*sin(v)
  // 
  //   where:
  //   - 0 <= u < 2 * pi, 0 <= v < 2*pi
  //   - c = the radius from the center to the middle of the ring of the torus.
  //   - a = the radius of the cross section of ring of the torus.
  // 
  //   Derivatives:
  //   - d(x(u,v)/du = -(c+a*cos(v))*sin(u)
  //   - d(x(u,v)/dv = -a*sin(v))*cos(u)
  //   - d(y(u,v)/du = (c+a*cos(v))*cos(u)
  //   - d(y(u,v)/dv = -a*sin(v))*sin(u)
  //   - d(z(u,v)/du = 0
  //   - d(z(u,v)/dv = a*cos(v)
  // 
  //   Let Du = (dx/du, dy/du, dz/du)
  //   
  //   Let Dv = (dx/dv, dy/dv, dz/dv)
  //   
  //   Then the normal n = Du X Dv
  //   
  //   - c > a corresponds to the ring torus.
  //   - c = a corresponds to a horn torus which is tangent to itself at the point (0, 0, 0).
  //   - c < a corresponds to a self-intersecting spindle torus.
  //
  // This function performs the mapping fn(u,v)->(x,y,x), returning it
  // as Pt. It also returns the partial derivatives Du and Dv.
  // Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)
  //</pre>
  virtual void Evaluate(double u[3], double Pt[3], double Du[9]);

  // Description:
  // Calculate a user defined scalar using one or all of u,Pt,Du.
  //
  // u[3] are the parameters with Pt being the the Cartesian point, 
  // Du[9] are the derivatives of this point with respect to u and v.
  // Pt, Du are obtained from Evaluate().
  //
  // This function is only called if the ScalarMode has the value
  // vtkParametricFunctionSource::SCALAR_FUNCTION_DEFINED
  //
  // If the user does not need to calculate a scalar, then the 
  // instantiated function should return zero. 
  //
  virtual double EvaluateScalar(double u[3], double Pt[3], double Du[9]);

protected:
  vtkParametricTorus();
  ~vtkParametricTorus();

  // Variables
  double RingRadius;
  double CrossSectionRadius;

private:
  vtkParametricTorus(const vtkParametricTorus&);  // Not implemented.
  void operator=(const vtkParametricTorus&);  // Not implemented.
};

#endif
