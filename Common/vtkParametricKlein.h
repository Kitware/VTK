/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricKlein.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParametricKlein - Generates a "classical" representation of a Klein bottle.
// .SECTION Description
// vtkParametricKlein generates a "classical" representation of a Klein
// bottle.  A Klein bottle is a closed surface with no interior and only one
// surface.  It is unrealisable in 3 dimensions without intersecting
// surfaces.  It can be
// realised in 4 dimensions by considering the map \f$F:R^2 \rightarrow R^4\f$  given by:
//
// - \f$f(u,v) = ((r*cos(v)+a)*cos(u),(r*cos(v)+a)*sin(u),r*sin(v)*cos(u/2),r*sin(v)*sin(u/2))\f$
//
// The classical representation of the immersion in \f$R^3\f$ is returned by this function.
//
//
// For further information about this surface, please consult the 
// technical description "Parametric surfaces" in http://www.vtk.org/documents.php 
// in the "VTK Technical Documents" section in the VTk.org web pages.
//
// .SECTION Thanks
// Andrew Maclean a.maclean@cas.edu.au for creating and contributing the
// class.
//
#ifndef __vtkParametricKlein_h
#define __vtkParametricKlein_h

#include "vtkParametricFunction.h"

class VTK_COMMON_EXPORT vtkParametricKlein : public vtkParametricFunction
{
public:
  vtkTypeMacro(vtkParametricKlein,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Construct a Klein Bottle with the following parameters:
  // MinimumU = 0, MaximumU = 2*Pi,
  // MinimumV = -Pi, MaximumV = Pi, 
  // JoinU = 0, JoinV = 1,
  // TwistU = 0, TwistV = 0,
  // ClockwiseOrdering = 1, 
  // DerivativesAvailable = 1,
  static vtkParametricKlein *New();  //! Initialise the parameters for the Klein bottle
  
  // Description
  // Return the parametric dimension of the class.
  virtual int GetDimension() {return 2;}

  // Description:
  // A Klein bottle.
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
  vtkParametricKlein();
  ~vtkParametricKlein();

private:
  vtkParametricKlein(const vtkParametricKlein&);  // Not implemented.
  void operator=(const vtkParametricKlein&);  // Not implemented.
};

#endif
