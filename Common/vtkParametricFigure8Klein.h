/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricFigure8Klein.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParametricFigure8Klein - generate a figure-8 Klein bottle
// .SECTION Description
// vtkParametricFigure8Klein generates a figure-8 Klein bottle.  A Klein bottle
// is a closed surface with no interior and only one surface.  It is
// unrealisable in 3 dimensions without intersecting surfaces.  It can be
// realised in 4 dimensions by considering the map G:R^2->R^4 given by:
//
// - G(u,v) = ((r*cos(v)+a)*cos(u),(r*cos(v)+a)*sin(u),r*sin(v)*cos(u/2),r*sin(v)*sin(u/2))
//
// This representation of the immersion in R^3 is formed by taking two Mobius
// strips and joining them along their boundaries, this is the so called
// "Figure-8 Klein Bottle"

// .SECTION Thanks
// Andrew Maclean a.maclean@cas.edu.au for creating and contributing the
// class.
//
#ifndef __vtkParametricFigure8Klein_h
#define __vtkParametricFigure8Klein_h

#include "vtkParametricFunction.h"

class VTK_COMMON_EXPORT vtkParametricFigure8Klein : public vtkParametricFunction
{
public:
  vtkTypeRevisionMacro(vtkParametricFigure8Klein,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Construct a figure-8 Klein Bottle with the following parameters:
  // MinimumU = -Pi, MaximumU = Pi, 
  // MinimumV = -Pi, MaximumV = Pi, 
  // JoinU = 1, JoinV = 1, 
  // TwistU = 0, TwistV = 0,
  // ClockwiseOrdering = 1, 
  // DerivativesAvailable = 1, 
  // Radius = 1
  static vtkParametricFigure8Klein *New();

  // Description:
  // Set/Get the radius of the bottle.
  vtkSetMacro(Radius,double);
  vtkGetMacro(Radius,double);

  // Description
  // Return the parametric dimension of the class.
  virtual int GetDimension() {return 2;}

  // Description:
  // A Klein bottle is a closed surface with no interior and only one
  // surface.  It is unrealisable in 3 dimensions without intersecting
  // surfaces.  It can be realised in 4 dimensions by considering the map
  // G:R^2->R^4 given by:
  // <pre>
  //   - G(u,v) = ((r*cos(v)+a)*cos(u),(r*cos(v)+a)*sin(u),r*sin(v)*cos(u/2),r*sin(v)*sin(u/2))
  // </pre>
  // This representation of the immersion in R^3 is formed by taking two 
  // Mobius strips and joining them along their boundaries, this the so 
  // called "Figure-8 Klein Bottle"
  // 
  // The parametric form of the equations for a Klein bottle are:
  // <pre>
  // - -PI <= u <= PI, -PI <= v <= PI
  // - x(u,v) = cos(u)*(a + sin(v)*cos(u/2) - sin(2*v)*sin(u/2)/2)
  // - y(u,v) = sin(u)*(a + sin(v)*cos(u/2) - sin(2*v)*sin(u/2)/2)
  // - z(u,v) = sin(u/2)*sin(v) + cos(u/2)*sin(2*v)/2
  // </pre>
  //
  // Derivatives are:
  // <pre>
  // - d(x(u,v))/du = -Y(u,v)-cos(u)*(2*sin(v)*sin(u/2)+sin(2*v)*cos(u/2))/4
  // - d(x(u,v))/dv = cos(u)*(cos(v)*cos(u/2)-cos(2*v)*sin(u/2))
  // - d(y(u,v))/du = X(u,v)-sin(u)*(2*sin(v)*sin(u/2)+sin(2*v)*cos(u/2))/4
  // - d(y(u,v))/dv = sin(u)*(cos(v)*cos(u/2)-cos(2*v)*sin(u/2))
  // - d(z(u,v))/du = cos(u/2)*sin(v)/2-sin(u/2)*sin(2*v)/4
  // - d(z(u,v))/dv = sin(u/2)*cos(v)/2+cos(u/2)*cos(2*v)
  // </pre>
  //
  // Let Du = (dx/du, dy/du, dz/du), and let Dv = (dx/dv, dy/dv, dz/dv). Then
  // the normal n is Du X Dv.
  //
  // This function performs the mapping fn(u,v)->(x,y,x), returning it as
  // Pt. It also returns the partial derivatives Du and Dv.  Pt = (x, y, z),
  // Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)
  virtual void Evaluate(double uvw[3], double Pt[3], double Duvw[9]);

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
  virtual double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]);

protected:
  vtkParametricFigure8Klein();
  ~vtkParametricFigure8Klein();

  // Variables
  double Radius;

private:
  vtkParametricFigure8Klein(const vtkParametricFigure8Klein&);  // Not implemented.
  void operator=(const vtkParametricFigure8Klein&);  // Not implemented.

};

#endif
