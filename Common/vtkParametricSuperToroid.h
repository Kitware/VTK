/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricSuperToroid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParametricSuperToroid - generate a supertoroid
// .SECTION Description
// vtkParametricSuperToroid generates a supertoroid.  Essentially a
// supertoroid is a torus with the sine and cosine terms raised to a power.
// A supertoroid is a versatile primitive that is controlled by four
// parameters r0, r1, n1 and n2. r0, r1 determine the type of torus whilst
// the value of n1 determines the shape of the torus ring and n2 determines
// the shape of the cross section of the ring. It is the different values of
// these powers which give rise to a family of 3D shapes that are all
// basically toroidal in shape.
//
// For more information see: http://astronomy.swin.edu.au/~pbourke/surfaces/.
//
// .SECTION Caveats
// Care needs to be taken specifying the bounds correctly. You may need to 
// carefully adjust MinimumU, MinimumV, MaximumU, MaximumV.
//
// .SECTION Thanks
// Andrew Maclean a.maclean@cas.edu.au for creating and contributing the
// class.
//
#ifndef __vtkParametricSuperToroid_h
#define __vtkParametricSuperToroid_h

#include "vtkParametricFunction.h"

class VTK_COMMON_EXPORT vtkParametricSuperToroid : public vtkParametricFunction
{
public:
  vtkTypeRevisionMacro(vtkParametricSuperToroid,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Construct a supertoroid with the following parameters: 
  // MinimumU = 0, MaximumU = 2*Pi, 
  // MinimumV = 0, MaximumV = 2*Pi, 
  // JoinU = 1, JoinV = 1,
  // TwistU = 0, TwistV = 0, 
  // ClockwiseOrdering = 1, 
  // DerivativesAvailable = 0,
  // RingRadius = 1, CrossSectionRadius = 0.5, 
  // N1 = 1, N2 = 1, XRadius = 1,
  // YRadius = 1, ZRadius = 1, a torus in this case.
  static vtkParametricSuperToroid *New();

  // Description
  // Return the parametric dimension of the class.
  virtual int GetDimension() {return 2;}

  // Description:
  // Set/Get the radius from the center to the middle of the ring of the
  // supertoroid.  Default = 1.
  vtkSetMacro(RingRadius,double);
  vtkGetMacro(RingRadius,double);

  // Description:
  // Set/Get the radius of the cross section of ring of the supertoroid.
  // Default = 0.5.
  vtkSetMacro(CrossSectionRadius,double);
  vtkGetMacro(CrossSectionRadius,double);

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
  // Set/Get the shape of the torus ring.  Default = 1.
  vtkSetMacro(N1,double);
  vtkGetMacro(N1,double);

  // Description:
  //  Set/Get the shape of the cross section of the ring. Default = 1.
  vtkSetMacro(N2,double);
  vtkGetMacro(N2,double);

  // Description:
  // <pre>
  // The parametric equations for a supertoroid are:
  // - x = cos(u)^n1*(r0+r1*cos(v)^n2);
  // - y = sin(u)^n1*(r0+r1*cos(v)^n2);
  // - z = r1*sin(v)^n2
  // 
  // where:
  // - 0 <= u <= pi, 0 <= v <= pi
  // - 0 <= n1,n2 < infinity
  // - rx, ry, rx are scaling factors ( 0 < rx, ry, rz < infinity )
  // - r0 = The radius from the center to the middle of the ring of the torus.
  // - r1 = The radius of the cross section of ring of the torus.
  // 
  // Derivatives:
  // - dx/du = -rx * cos(u)^n1*n1*sin(u)/cos(u)*(r0+r1*cos(v)^n2);
  // - dy/du = ry * sin(u)^n1*n1*cos(u)/sin(u)*(r0+r1*cos(v)^n2);
  // - dz/du = 0;
  // - dx/dv = -rx * cos(u)^n1*r1*cos(v)^n2*n2*sin(v)/cos(v);
  // - dy/dv = -ry * sin(u)^n1*r1*cos(v)^n2*n2*sin(v)/cos(v);
  // - dz/dv = rz * r1*sin(v)^n2*n2*cos(v)/sin(v);
  // 
  // Note that rx, ry, and rz are the scale factors for each axis (axis
  // intercept).  The power n1 controls the shape of the torus ring and n2
  // controls the shape of the cross section of the ring.
  // 
  // In practise, the absolute values of the trigonometric functions
  // are used when raising to a power. This removes any complex values
  // but may introduce unusual representations of the surface in some cases.
  //
  // Note that there are are some numerical issues with both very 
  // small or very large values of of n1 and n2.
  //
  // Let Du = (dx/du, dy/du, dz/du). Let Dv = (dx/dv, dy/dv, dz/dv). Then the
  // normal n = Du X Dv.
  // 
  // - r0 > r1 corresponds to the ring torus.
  // - r0 = r1 corresponds to a horn torus which is tangent to itself at the point (0, 0, 0).
  // - r0 < r1 corresponds to a self-intersecting spindle torus.
  //
  // For more information see: http://astronomy.swin.edu.au/~pbourke/surfaces/.
  //
  // This function performs the mapping fn(u,v)->(x,y,x), returning it
  // as Pt. It also returns the partial derivatives Du and Dv.
  // Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)
  // </pre>
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
  vtkParametricSuperToroid();
  ~vtkParametricSuperToroid();

  // Variables
  double RingRadius;
  double CrossSectionRadius;
  double XRadius;
  double YRadius;
  double ZRadius;
  double N1;
  double N2;

private:
  vtkParametricSuperToroid(const vtkParametricSuperToroid&);  // Not implemented.
  void operator=(const vtkParametricSuperToroid&);  // Not implemented.

  // Description:
  // Calculate sign(x)*(abs(x)^n).
  double Power ( double x, double n );

};

#endif

