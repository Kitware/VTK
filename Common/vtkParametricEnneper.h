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
//
// For further information about this surface, please consult the 
// technical description "Parametric surfaces" in http://www.vtk.org/documents.php 
// in the "VTK Technical Documents" section in the VTk.org web pages.
//
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

  vtkTypeMacro(vtkParametricEnneper,vtkParametricFunction);
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
  // Enneper's surface.
  //
  // This function performs the mapping \f$f(u,v) \rightarrow (x,y,x)\f$, returning it
  // as Pt. It also returns the partial derivatives Du and Dv.
  // \f$Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)\f$ .
  // Then the normal is \f$N = Du X Dv\f$ .
  virtual void Evaluate(double uvw[3], double Pt[3], double Duvw[9]);

  // Description:
  // Calculate a user defined scalar using one or all of uvw, Pt, Duvw.
  //
  // uv are the parameters with Pt being the the cartesian point, 
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
  vtkParametricEnneper();
  ~vtkParametricEnneper();

private:
  vtkParametricEnneper(const vtkParametricEnneper&);  // Not implemented.
  void operator=(const vtkParametricEnneper&);  // Not implemented.
};

#endif
