/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricRoman.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParametricRoman - Generate Steiner's Roman Surface.
// .SECTION Description
// vtkParametricRoman generates Steiner's Roman Surface.
//
// For further information about this surface, please consult the 
// technical description "Parametric surfaces" in http://www.vtk.org/documents.php 
// in the "VTK Technical Documents" section in the VTk.org web pages.
//
// .SECTION Thanks
// Andrew Maclean a.maclean@cas.edu.au for 
// creating and contributing the class.
//
#ifndef __vtkParametricRoman_h
#define __vtkParametricRoman_h

#include "vtkParametricFunction.h"

class VTK_COMMON_EXPORT vtkParametricRoman : public vtkParametricFunction
{

public:
  vtkTypeMacro(vtkParametricRoman,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description
  // Return the parametric dimension of the class.
  virtual int GetDimension() {return 2;}

  // Description:
  // Construct Steiner's Roman Surface with the following parameters:
  // MinimumU = 0, MaximumU = Pi,
  // MinimumV = 0, MaximumV = Pi, 
  // JoinU = 1, JoinV = 1,
  // TwistU = 1, TwistV = 0; 
  // ClockwiseOrdering = 1, 
  // DerivativesAvailable = 1,
  // Radius = 1
  static vtkParametricRoman *New();

  // Description:
  // Set/Get the radius.
  vtkSetMacro(Radius,double);
  vtkGetMacro(Radius,double);

  // Description:
  // Steiner's Roman Surface
  //
  // This function performs the mapping \f$f(u,v) \rightarrow (x,y,x)\f$, returning it
  // as Pt. It also returns the partial derivatives Du and Dv.
  // \f$Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)\f$ .
  // Then the normal is \f$N = Du X Dv\f$ .
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
  vtkParametricRoman();
  ~vtkParametricRoman();

  // Variables
  double Radius;

private:
  vtkParametricRoman(const vtkParametricRoman&);  // Not implemented.
  void operator=(const vtkParametricRoman&);  // Not implemented.
};

#endif
