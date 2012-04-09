/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricBoy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParametricBoy - Generate Boy's surface.
// .SECTION Description
// vtkParametricBoy generates Boy's surface.
// This is a Model of the projective plane without singularities.
// It was found by Werner Boy on assignment from David Hilbert.
//
// For further information about this surface, please consult the
// technical description "Parametric surfaces" in http://www.vtk.org/documents.php
// in the "VTK Technical Documents" section in the VTk.org web pages.
//
// .SECTION Thanks
// Andrew Maclean a.maclean@cas.edu.au for
// creating and contributing the class.
//
#ifndef __vtkParametricBoy_h
#define __vtkParametricBoy_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkParametricFunction.h"

class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkParametricBoy : public vtkParametricFunction
{
public:

  vtkTypeMacro(vtkParametricBoy,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct Boy's surface with the following parameters:
  // MinimumU = 0, MaximumU = Pi,
  // MinimumV = 0, MaximumV = Pi,
  // JoinU = 1, JoinV = 1,
  // TwistU = 1, TwistV = 1;
  // ClockwiseOrdering = 1,
  // DerivativesAvailable = 1,
  // ZScale = 0.125.
  static vtkParametricBoy *New();

  // Description
  // Return the parametric dimension of the class.
  virtual int GetDimension() {return 2;}

  // Description:
  // Set/Get the scale factor for the z-coordinate.
  // Default = 1/8, giving a nice shape.
  vtkSetMacro(ZScale,double);
  vtkGetMacro(ZScale,double);

  // Description:
  // Boy's surface.
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
  vtkParametricBoy();
  ~vtkParametricBoy();

  // Variables
  double ZScale;

private:
  vtkParametricBoy(const vtkParametricBoy&);  // Not implemented.
  void operator=(const vtkParametricBoy&);  // Not implemented.
};

#endif
