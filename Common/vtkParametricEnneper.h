/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricEnneper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParametricEnneper - Generate Enneper's surface.
// .SECTION Description
// vtkParametricEnneper generates Enneper's surface.
// Enneper's surface is a a self-intersecting minimal surface
// posessing constant negative Gaussian curvature
// .SECTION Thanks
// Andrew Maclean a.maclean@cas.edu.au for 
// creating and contributing the class.
//
#ifndef __vtkParametricEnneper_h
#define __vtkParametricEnneper_h

#include "vtkParametricFunction.h"

class VTK_COMMON_EXPORT vtkParametricEnneper : public vtkParametricFunction
{
public:

  vtkTypeRevisionMacro(vtkParametricEnneper,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Construct Enneper's surface with the following parameters:
  // MinimumU = -2, MaximumU = 2,
  // MinimumV = -2, MaximumV = 2, 
  // JoinU = 0, JoinV = 0,
  // TwistU = 0, TwistV = 0,
  // ClockwiseOrdering = 1, 
  // DerivativesAvailable = 1
  static vtkParametricEnneper *New();

  // Description
  // Return the parametric dimension of the class.
  virtual int GetDimension() {return 2;}

  // Description:
  // Enneper's surface is an example of  a self-intersecting minimal 
  // surface. Enneper's surface is a well-known minimal surface. Though 
  // it has a fairly uncomplicated parameterization , it is somewhat 
  // hard to visualize because of its self-intersections. The plot suggests 
  // the self-intersections exhibited by the surface, but the plot range 
  // has been kept small enough that the structure of the surface's center 
  // is also visible. 
  //
  // Note that the self-intersection curves are subsets of the planes 
  // y = 0 and x = 0. The surface is a special case of the more general Enneper's 
  // surface of degree n. These surfaces tend to be even more complicated and 
  // difficult to visualize.
  //
  // A parametric representation of Enneper's surface
  // Define:
  // -  X(u,v) = u-u^3/3+u*v^2
  // -  Y(u,v) = v-v^3/3+v*u^2
  // -  Z(u,v) = u^2-v^2
  //
  // Then
  // - S(u,v) = (X(u,v),Y(u,v),Z(u,v)) defines the surface. 
  //
  // The derivatives are given by:
  // - d(X(u,v)/du = 1-u^2+v^2
  // - d(X(u,v)/dv = 2*u*v
  // - d(Y(u,v)/du = 2*u*v
  // - d(Y(u,v)/dv = 1-v^2+u^2
  // - d(Z(u,v)/du = 2*u
  // - d(Z(u,v)/dv = -2*v
  //
  // Let Du = (dy/du, dy/du, dy/du)
  //
  // Let Dv = (dx/dv, dy/dv, dz/dv)
  //
  // Then the normal n = Du X Dv
  //
  // This function performs the mapping fn(u,v)->(x,y,x), returning it
  // as Pt. It also returns the partial derivatives Du and Dv.
  // Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)
  void Evaluate(double uvw[3], double Pt[3], double Duvw[9]);

  // Description:
  // Calculate a user defined scalar using one or all of uvw,Pt,Duvw.
  //
  // uv are the parameters with Pt being the the cartesian point, 
  // Duvw are the derivatives of this point with respect to u, v and w.
  // Pt, Duvw are obtained from Evaluate().
  //
  // This function is only called if the ScalarMode has the value
  // vtkParametricTriangulator::userDefined
  //
  // If the user does not need to calculate a scalar, then the 
  // instantiated function should return zero. 
  //
  double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]);

protected:
  vtkParametricEnneper();
  ~vtkParametricEnneper();

private:
  vtkParametricEnneper(const vtkParametricEnneper&);  // Not implemented.
  void operator=(const vtkParametricEnneper&);  // Not implemented.
};

#endif
