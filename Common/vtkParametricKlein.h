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
// .NAME vtkParametricKlein - a "classical" representation of a Klein bottle
// .SECTION Description
// vtkParametricKlein is a "classical" representation of a Klein
// bottle.  A Klein bottle is a closed surface with no interior and only one
// surface.  It is unrealisable in 3 dimensions without intersecting
// surfaces.  It can be realised in 4 dimensions by considering the map
// G:R^2->R^4 given by:
// <pre>
// - G(u,v) = ((r*cos(v)+a)*cos(u),(r*cos(v)+a)*sin(u),r*sin(v)*cos(u/2),r*sin(v)*sin(u/2))
// </pre>
// The classical representation of the immersion in R^3 is returned by this function.
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
  vtkTypeRevisionMacro(vtkParametricKlein,vtkParametricFunction);
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
  // A Klein bottle is a closed surface with no interior and only one surface.
  // It is unrealisable in 3 dimensions without intersecting surfaces. 
  // It can be realised in 4 dimensions by considering 
  // the map G:R^2->R^4 given by:
  // 
  // - G(u,v) = ((r*cos(v)+a)*cos(u),(r*cos(v)+a)*sin(u),r*sin(v)*cos(u/2),r*sin(v)*sin(u/2))
  // 
  // The classical representation of the immersion in R^3 is returned 
  // by this function.
  //<pre>
  // - F(u,v) = - (-2/15*cos(u)*(3*cos(v)+5*sin(u)*cos(v)*cos(u)-30*sin(u)-
  // 60*sin(u)*cos(u)^6+90*sin(u)*cos(u)^4),-1/15*sin(u)*(80*cos(v)*cos(u)^7*sin(u)+
  // 48*cos(v)*cos(u)^6-80*cos(v)*cos(u)^5*sin(u)-48*cos(v)*cos(u)^4-5*cos(v)*cos(u)^3*sin(u)-
  // 3*cos(v)*cos(u)^2+5*sin(u)*cos(v)*cos(u)+3*cos(v)-60*sin(u)),2/15*sin(v)*(3+5*sin(u)*cos(u)))
  //</pre>
  // Thanks to Robert Israel, israel@math.ubc.ca http://www.math.ubc.ca/~israel
  // for this parametrisation.
  // 
  // The parametric form of the equations for a Klein bottle are:
  // <pre>
  // For 0 <= u <= PI, 0 <= v <= 2PI
  // x(u,v) = -2/15*cos(u)*(3*cos(v)+5*sin(u)*cos(v)*cos(u)-30*sin(u)-
  //          60*sin(u)*cos(u)^6+90*sin(u)*cos(u)^4)
  // y(u,v) = -1/15*sin(u)*(80*cos(v)*cos(u)^7*sin(u)+48*cos(v)*cos(u)^6-
  //          80*cos(v)*cos(u)^5*sin(u)-48*cos(v)*cos(u)^4-5*cos(v)*cos(u)^3*sin(u)-
  //          3*cos(v)*cos(u)^2+5*sin(u)*cos(v)*cos(u)+3*cos(v)-60*sin(u))
  // z(u,v) = 2/15*sin(v)*(3+5*sin(u)*cos(u))
  // </pre>
  // Derivatives are:
  // <pre>
  // - d(x(u,v))/du = 2/15*sin(u)*(3*cos(v)+5*sin(u)*cos(v)*cos(u)-30*sin(u)-
  //                  60*sin(u)*cos(u)^6+90*sin(u)*cos(u)^4)-2/15*cos(u)*(5*cos(v)*cos(u)^2-
  //                  5*sin(u)^2*cos(v)-30*cos(u)-60*cos(u)^7+360*sin(u)^2*cos(u)^5+90*cos(u)^5-
  //                  360*sin(u)^2*cos(u)^3)
  // - d(x(u,v))/dv = -2/15*cos(u)*(-3*sin(v)-5*sin(u)*sin(v)*cos(u))
  // - d(y(u,v))/du = -1/15*cos(u)*(80*cos(v)*cos(u)^7*sin(u)+48*cos(v)*cos(u)^6-
  //                  80*cos(v)*cos(u)^5*sin(u)-48*cos(v)*cos(u)^4-5*cos(v)*cos(u)^3*sin(u)-
  //                  3*cos(v)*cos(u)^2+5*sin(u)*cos(v)*cos(u)+3*cos(v)-60*sin(u))-
  //                  1/15*sin(u)*(-560*cos(v)*cos(u)^6*sin(u)^2+80*cos(v)*cos(u)^8-
  //                  288*cos(v)*cos(u)^5*sin(u)+400*cos(v)*cos(u)^4*sin(u)^2-
  //                  80*cos(v)*cos(u)^6+192*cos(v)*cos(u)^3*sin(u)+15*sin(u)^2*cos(v)*cos(u)^2-
  //                  5*cos(v)*cos(u)^4+6*sin(u)*cos(v)*cos(u)+5*cos(v)*cos(u)^2-
  //                  5*sin(u)^2*cos(v)-60*cos(u))
  // - d(y(u,v))/dv = -1/15*sin(u)*(-80*sin(v)*cos(u)^7*sin(u)-48*sin(v)*cos(u)^6+
  //                  80*sin(v)*cos(u)^5*sin(u)+48*sin(v)*cos(u)^4+5*sin(v)*cos(u)^3*sin(u)+
  //                  3*sin(v)*cos(u)^2-5*sin(u)*sin(v)*cos(u)-3*sin(v))
  // - d(z(u,v))/du = 2/15*sin(v)*(5*cos(u)^2-5*sin(u)^2)
  // - d(z(u,v))/dv = 2/15*cos(v)*(3+5*sin(u)*cos(u))
  // </pre>
  // 
  // Let Du = (dx/du, dy/du, dz/du)
  // 
  // Let Dv = (dx/dv, dy/dv, dz/dv)
  // 
  // Then the normal n = Du X Dv.
  //
  // This function performs the mapping Evaluate(u)->(x), returning it
  // as Pt. It also returns the partial derivatives Du and Dv.
  // Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv).
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
  vtkParametricKlein();
  ~vtkParametricKlein();

private:
  vtkParametricKlein(const vtkParametricKlein&);  // Not implemented.
  void operator=(const vtkParametricKlein&);  // Not implemented.
};

#endif
