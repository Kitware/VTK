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
// .NAME vtkParametricTorus - Generate a torus.
// .SECTION Description
// vtkParametricTorus generates a torus.
//
// For further information about this surface, please consult the 
// technical description "Parametric surfaces" in http://www.vtk.org/documents.php 
// in the "VTK Technical Documents" section in the VTk.org web pages.
//
// .SECTION Thanks
// Andrew Maclean a.maclean@cas.edu.au for 
// creating and contributing the class.
//
#ifndef __vtkParametricTorus_h
#define __vtkParametricTorus_h

#include "vtkParametricFunction.h"

class VTK_COMMON_EXPORT vtkParametricTorus : public vtkParametricFunction
{

public:
  vtkTypeMacro(vtkParametricTorus,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Construct a torus with the following parameters:
  // MinimumU = 0, MaximumU = 2*Pi,
  // MinimumV = 0, MaximumV = 2*Pi, 
  // JoinU = 1, JoinV = 1,
  // TwistU = 0, TwistV = 0, 
  // ClockwiseOrdering = 1, 
  // DerivativesAvailable = 1,
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
  // A torus.
  //
  // This function performs the mapping \f$f(u,v) \rightarrow (x,y,x)\f$, returning it
  // as Pt. It also returns the partial derivatives Du and Dv.
  // \f$Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)\f$.
  // Then the normal is \f$N = Du X Dv\f$.
  virtual void Evaluate(double uvw[3], double Pt[3], double Duvw[9]);

  // Description:
  // Calculate a user defined scalar using one or all of uvw, Pt, Duvw.
  //
  // uvw are the parameters with Pt being the the Cartesian point, 
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
