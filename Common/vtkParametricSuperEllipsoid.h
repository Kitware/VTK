/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricSuperEllipsoid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParametricSuperEllipsoid - generate a superellipsoid
// .SECTION Description
// vtkParametricSuperEllipsoid generates a superellipsoid.  A superellipsoid
// is a versatile primitive that is controlled by two parameters n1 and
// n2. As special cases it can represent a sphere, square box, and closed
// cylindrical can.
//
// For more information see: http://astronomy.swin.edu.au/~pbourke/surfaces/
//
// .SECTION Caveats
// Care needs to be taken specifying the bounds correctly. You may need to 
// carefully adjust MinimumU, MinimumV, MaximumU, MaximumV.
//
// .SECTION Thanks
// Andrew Maclean a.maclean@cas.edu.au for creating and contributing the
// class.
//
#ifndef __vtkParametricSuperEllipsoid_h
#define __vtkParametricSuperEllipsoid_h

#include "vtkParametricFunction.h"

class VTK_COMMON_EXPORT vtkParametricSuperEllipsoid : public vtkParametricFunction
{
public:
  vtkTypeRevisionMacro(vtkParametricSuperEllipsoid,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Construct a superellipsoid with the following parameters: PointsU = 50,
  // PointsV = 50, MinimumU = 0, MaximumU = 2*Pi, MinimumV = 0, MaximumV =
  // 2*Pi, JoinU = 1, JoinV = 1, TwistU = 0, TwistV = 0; Ordering = 0,
  // DerivativesSupplied = 0, N1 = 1, N2 = 1, XRadius = 1, YRadius = 1,
  // ZRadius = 1, a sphere in this case.
  static vtkParametricSuperEllipsoid *New();

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
  // Set/Get the "squareness" parameter in the z axis.  Default = 1.
  vtkSetMacro(N1,double);
  vtkGetMacro(N1,double);

  // Description:
  //  Set/Get the "squareness" parameter in the x-y plane. Default = 1.
  vtkSetMacro(N2,double);
  vtkGetMacro(N2,double);

  // Description:
  // The parametric equations for a superellipsoid are:
  // <pre>
  // - x = rx*cos(u)^n1*cos(v)^n2
  // - y = ry*cos(u)^n1*sin(v)^n2
  // - z = rz*sin(u)^n1
  // 
  // where:
  // - -pi/2 <= u < pi/2, -pi <= v < pi
  // - 0 <= n1,n2 < infinity
  // - rx, ry, rx are scaling factors 
  // ( 0 < rx, ry, rz < infinity )
  // 
  // Derivatives:
  // - dx/du = -rx*cos(u)^n1*n1*sin(u)/cos(u)*cos(v)^n2;
  // - dy/du = -ry*cos(u)^n1*n1*sin(u)/cos(u)*sin(v)^n2;
  // - dz/du = rz*sin(u)^n1*n1*cos(u)/sin(u);
  // - dx/dv = -rx*cos(u)^n1*cos(v)^n2*n2*sin(v)/cos(v);
  // - dy/dv = ry*cos(u)^n1*sin(v)^n2*n2*cos(v)/sin(v);
  // - dz/dv = 0;
  // </pre>
  // Let Du = (dx/du, dy/du, dz/du). Let Dv = (dx/dv, dy/dv, dz/dv). Then the normal n = Du X Dv.
  // 
  // Note that rx, ry, and rz are the scale factors for each axis 
  // (axis intercept). The total shape lies at the centre of a box 
  // of dimension 2rx, 2ry, 2rz. The power n1 acts as the "squareness"
  // parameter in the z axis and n2 the "squareness" in the x-y plane.
  // 
  // In practise, the absolute values of the trigonometric functions
  // are used when raising to a power. This removes any complex values
  // but can introduce unusual representations of the surface e.g. in 
  // the case of a sphere, the poles can be joined internally.
  //
  // Note that there are are some numerical issues with both very 
  // small or very large values of of n1 and n2.
  //
  // Here is a table showing some interesting special cases 
  // for parameters n1 and n2:
  // 
  // <div align="center">
  // <center>
  // <table border="0" cellspacing="1" style="border-collapse: collapse" bordercolor="#111111" id="AutoNumber1">
  //   <tr>
  //   <td><b>n1</b></td>
  //   <td><b>n2</b></td>
  //   <td><b>rx,ry,rz</b></td>
  //   <td><b>Description</b></td>
  //   </tr>
  //   <tr>
  //   <td align="center">0</td>
  //   <td align="center">0</td>
  //   <td align="center">&nbsp;</td>
  //   <td align="center">&nbsp;</td>
  //   </tr>
  //   <tr>
  //   <td align="center">0</td>
  //   <td align="center">0</td>
  //   <td align="center">rx=ry=rz</td>
  //   <td align="center">cube</td>
  //   </tr>
  //   <tr>
  //   <td align="center">&lt;1</td>
  //   <td align="center">&lt;1</td>
  //   <td align="center">&nbsp;</td>
  //   <td align="center">cuboid</td>
  //   </tr>
  //   <tr>
  //   <td align="center">&lt;1</td>
  //   <td align="center">1</td>
  //   <td align="center">&nbsp;</td>
  //   <td align="center">pillow shaped</td>
  //   </tr>
  //   <tr>
  //   <td align="center">1</td>
  //   <td align="center">&lt;1</td>
  //   <td align="center">&nbsp;</td>
  //   <td align="center">cylindical</td>
  //   </tr>
  //   <tr>
  //   <td align="center">1</td>
  //   <td align="center">1</td>
  //   <td align="center">&nbsp;</td>
  //   <td align="center">ellipsoid</td>
  //   </tr>
  //   <tr>
  //   <td align="center">1</td>
  //   <td align="center">1</td>
  //   <td align="center">rx=ry=rz</td>
  //   <td align="center">sphere</td>
  //   </tr>
  //   <tr>
  //   <td align="center">2</td>
  //   <td align="center">2</td>
  //   <td align="center">rx=ry=rz</td>
  //   <td align="center">octahedron</td>
  //   </tr>
  //   <tr>
  //   <td align="center" colspan="2">n1 or n2 &gt; 2</td>
  //   <td align="center">&nbsp;</td>
  //   <td align="center">pinched</td>
  //   </tr>
  //   <tr>
  //   <td align="center" colspan="2">n1 or n2 = 2</td>
  //   <td align="center">&nbsp;</td>
  //   <td align="center">bevelled</td>
  //   </tr>
  //   <tr>
  //   <td align="center">&#8734;</td>
  //   <td align="center">&#8734;</td>
  //   <td align="center">&nbsp;</td>
  //   <td align="center">3D plus</td>
  //   </tr>
  // </table>
  // </center>
  // For more information see: http://astronomy.swin.edu.au/~pbourke/surfaces/.
  //
  // This function performs the mapping fn(u,v)->(x,y,x), returning it
  // as Pt. It also returns the partial derivatives Du and Dv.
  // Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)
  void Evaluate(double u[3], double Pt[3], double Du[9]);

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
  double EvaluateScalar(double u[3], double Pt[3], double Du[9]);

protected:
  vtkParametricSuperEllipsoid();
  ~vtkParametricSuperEllipsoid();

  // Variables
  double XRadius;
  double YRadius;
  double ZRadius;
  double N1;
  double N2;

private:
  vtkParametricSuperEllipsoid(const vtkParametricSuperEllipsoid&);  // Not implemented.
  void operator=(const vtkParametricSuperEllipsoid&);  // Not implemented.

  // Description:
  // Calculate sign(x)*(abs(x)^n).
  double Power ( double x, double n );

};

#endif
