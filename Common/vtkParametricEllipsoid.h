/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricEllipsoid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParametricEllipsoid - generate a ellipsoid
// .SECTION Description
// vtkParametricEllipsoid generates a ellipsoid.
// If all the radii are the same, we have a sphere.
// An oblate spheroid occurs if RadiusX = RadiusY > RadiusZ. 
// Here the Z-axis forms the symmetry axis. To a first
// approximation, this is the shape of the earth.
// A prolate spheroid occurs if RadiusX = RadiusY < RadiusZ.
//
// .SECTION Thanks
// Andrew Maclean a.maclean@cas.edu.au for creating and contributing the
// class.
//
#ifndef __vtkParametricEllipsoid_h
#define __vtkParametricEllipsoid_h

#include "vtkParametricFunction.h"

class VTK_COMMON_EXPORT vtkParametricEllipsoid : public vtkParametricFunction
{
public:
  vtkTypeRevisionMacro(vtkParametricEllipsoid,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Construct an ellipsoid with the following parameters:
  // MinimumU = 0, MaximumU = 2*Pi, 
  // MinimumV = 0, MaximumV = Pi, 
  // JoinU = 1, JoinV = 0, 
  // TwistU = 0, TwistV = 0, 
  // ClockwiseOrdering = 0,
  // DerivativesAvailable = 1, 
  // XRadius = 1, YRadius = 1,
  // ZRadius = 1, a sphere in this case.
  static vtkParametricEllipsoid *New();

  // Description
  // Return the parametric dimension of the class.
  virtual int GetDimension() {return 2;}

  // Description:
  // Set/Get the scaling factor for the x-axis. Default = 1.
  vtkSetMacro(XRadius,double);
  vtkGetMacro(XRadius,double);

  // Description:
  // Set/Get the scaling factor for the y-axis. Default = 1.
  vtkSetMacro(YRadius,double);
  vtkGetMacro(YRadius,double);

  // Description:
  // Set/Get the scaling factor for the z-axis. Default = 1.
  vtkSetMacro(ZRadius,double);
  vtkGetMacro(ZRadius,double);

  // Description:
  // The parametric equations for a ellipsoid are:
  // <pre>
  // - x = rx*sin(v)*cos(u)
  // - y = ry*sin(v)*sin(u)
  // - z = rz*cos(v)
  // 
  // where:
  // - 0 <= u < pi, 0 <= v < pi/2 
  // - rx, ry, rx are scaling factors 
  // ( 0 < rx, ry, rz < infinity )
  // 
  // Derivatives:
  // - dx/du = -rx*sin(v)*sin(u);
  // - dy/du = ry*sin(v)*cos(u);
  // - dz/du = 0;
  // - dx/dv = rx*cos(v)*cos(u);
  // - dy/dv = ry*cos(v)*sin(u);
  // - dz/dv = -rz*sin(v);
  // </pre>
  // Let Du = (dx/du, dy/du, dz/du). Let Dv = (dx/dv, dy/dv, dz/dv). Then the normal n = Du X Dv.
  // 
  // Note that rx, ry, and rz are the scale factors for each axis. 
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
  // vtkParametricFunction::userDefined
  //
  // If the user does not need to calculate a scalar, then the 
  // instantiated function should return zero. 
  //
  double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]);

protected:
  vtkParametricEllipsoid();
  ~vtkParametricEllipsoid();

  // Variables
  double XRadius;
  double YRadius;
  double ZRadius;
  double N1;
  double N2;

private:
  vtkParametricEllipsoid(const vtkParametricEllipsoid&);  // Not implemented.
  void operator=(const vtkParametricEllipsoid&);  // Not implemented.

};

#endif
