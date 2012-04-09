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
// .NAME vtkParametricFigure8Klein - Generate a figure-8 Klein bottle.
// .SECTION Description
// vtkParametricFigure8Klein generates a figure-8 Klein bottle.  A Klein bottle
// is a closed surface with no interior and only one surface.  It is
// unrealisable in 3 dimensions without intersecting surfaces.  It can be
// realised in 4 dimensions by considering the map \f$F:R^2 \rightarrow R^4\f$  given by:
//
// - \f$f(u,v) = ((r*cos(v)+a)*cos(u),(r*cos(v)+a)*sin(u),r*sin(v)*cos(u/2),r*sin(v)*sin(u/2))\f$
//
// This representation of the immersion in \f$R^3\f$ is formed by taking two Mobius
// strips and joining them along their boundaries, this is the so called
// "Figure-8 Klein Bottle"
//
// For further information about this surface, please consult the 
// technical description "Parametric surfaces" in http://www.vtk.org/documents.php 
// in the "VTK Technical Documents" section in the VTk.org web pages.
//
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
  vtkTypeMacro(vtkParametricFigure8Klein,vtkParametricFunction);
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
  // A Figure-8 Klein bottle.  
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
  vtkParametricFigure8Klein();
  ~vtkParametricFigure8Klein();

  // Variables
  double Radius;

private:
  vtkParametricFigure8Klein(const vtkParametricFigure8Klein&);  // Not implemented.
  void operator=(const vtkParametricFigure8Klein&);  // Not implemented.

};

#endif
