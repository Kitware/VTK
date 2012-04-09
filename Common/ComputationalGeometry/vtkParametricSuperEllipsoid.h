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
// .NAME vtkParametricSuperEllipsoid - Generate a superellipsoid.
// .SECTION Description
// vtkParametricSuperEllipsoid generates a superellipsoid.  A superellipsoid
// is a versatile primitive that is controlled by two parameters n1 and
// n2. As special cases it can represent a sphere, square box, and closed
// cylindrical can.
//
// For further information about this surface, please consult the
// technical description "Parametric surfaces" in http://www.vtk.org/documents.php
// in the "VTK Technical Documents" section in the VTk.org web pages.
//
// Also see: http://astronomy.swin.edu.au/~pbourke/surfaces/
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

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkParametricFunction.h"

class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkParametricSuperEllipsoid : public vtkParametricFunction
{
public:
  vtkTypeMacro(vtkParametricSuperEllipsoid,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct a superellipsoid with the following parameters:
  // MinimumU = 0, MaximumU = 2*Pi,
  // MinimumV = 0, MaximumV = Pi,
  // JoinU = 1, JoinV = 0,
  // TwistU = 0, TwistV = 0,
  // ClockwiseOrdering = 1,
  // DerivativesAvailable = 0,
  // N1 = 1, N2 = 1, XRadius = 1, YRadius = 1,
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
  // A superellipsoid.
  //
  // This function performs the mapping \f$f(u,v) \rightarrow (x,y,x)\f$, returning it
  // as Pt. It also returns the partial derivatives Du and Dv.
  // \f$Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)\f$ .
  // Then the normal is \f$N = Du X Dv\f$ .
  virtual void Evaluate(double uvw[3], double Pt[3], double Duvw[9]);

  // Description:
  // Calculate a user defined scalar using one or all of uvw, Pt, Duvw.
  //
  // uvw are the parameters with Pt being the the cartesian point,
  // Duvw are the derivatives of this point with respect to u, v and w.
  // Pt, Duvw are obtained from Evaluate().
  //
  // This function is only called if the ScalarMode has the value
  // vtkParametricFunctionSource::SCALAR_FUNCTION_DEFINED
  //
  // If the user does not need to calculate a scalar, then the
  // instantiated function should return zero.
  //
  virtual double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]);

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
