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
// .NAME vtkParametricEllipsoid - Generate an ellipsoid.
// .SECTION Description
// vtkParametricEllipsoid generates an ellipsoid.
// If all the radii are the same, we have a sphere.
// An oblate spheroid occurs if RadiusX = RadiusY > RadiusZ.
// Here the Z-axis forms the symmetry axis. To a first
// approximation, this is the shape of the earth.
// A prolate spheroid occurs if RadiusX = RadiusY < RadiusZ.
//
// For further information about this surface, please consult the
// technical description "Parametric surfaces" in http://www.vtk.org/documents.php
// in the "VTK Technical Documents" section in the VTk.org web pages.
//
// .SECTION Thanks
// Andrew Maclean andrew.amaclean@gmail.com for creating and contributing the
// class.
//
#ifndef vtkParametricEllipsoid_h
#define vtkParametricEllipsoid_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkParametricFunction.h"

class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkParametricEllipsoid : public vtkParametricFunction
{
public:
  vtkTypeMacro(vtkParametricEllipsoid,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct an ellipsoid with the following parameters:
  // MinimumU = 0, MaximumU = 2*Pi,
  // MinimumV = 0, MaximumV = Pi,
  // JoinU = 1, JoinV = 0,
  // TwistU = 0, TwistV = 0,
  // ClockwiseOrdering = 1,
  // DerivativesAvailable = 1,
  // XRadius = 1, YRadius = 1,
  // ZRadius = 1, a sphere in this case.
  static vtkParametricEllipsoid *New();

  // Description
  // Return the parametric dimension of the class.
  virtual int GetDimension() {return 2;}

  // Description:
  // Set/Get the scaling factor for the x-axis. Default is 1.
  vtkSetMacro(XRadius,double);
  vtkGetMacro(XRadius,double);

  // Description:
  // Set/Get the scaling factor for the y-axis. Default is 1.
  vtkSetMacro(YRadius,double);
  vtkGetMacro(YRadius,double);

  // Description:
  // Set/Get the scaling factor for the z-axis. Default is 1.
  vtkSetMacro(ZRadius,double);
  vtkGetMacro(ZRadius,double);

  // Description:
  // An ellipsoid.
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
