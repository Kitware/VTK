/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricConicSpiral.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParametricConicSpiral - Generate conic spiral surfaces that resemble sea-shells.
// .SECTION Description
// vtkParametricConicSpiral generates conic spiral surfaces. These can resemble sea shells, or
// may look like a torus "eating" its own tail.
//
// For further information about this surface, please consult the
// technical description "Parametric surfaces" in http://www.vtk.org/documents.php
// in the "VTK Technical Documents" section in the VTk.org web pages.
//
// .SECTION Thanks
// Andrew Maclean andrew.amaclean@gmail.com for creating and contributing the
// class.
//
#ifndef vtkParametricConicSpiral_h
#define vtkParametricConicSpiral_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkParametricFunction.h"

class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkParametricConicSpiral : public vtkParametricFunction
{
public:
  vtkTypeMacro(vtkParametricConicSpiral,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct a conic spiral surface with the following parameters:
  // MinimumU = 0, MaximumU = 2Pi,
  // MinimumV = 0, MaximumV = 2Pi,
  // JoinU = 0, JoinV = 0,
  // TwistU = 0, TwistV = 0,
  // ClockwiseOrdering = 1,
  // DerivativesAvailable = 1,
  // A = 0.2, B = 1.0, C = 0.1, N = 2.
  static vtkParametricConicSpiral *New();

  // Description
  // Return the parametric dimension of the class.
  virtual int GetDimension() {return 2;}

   // Description:
  // Set/Get the scale factor.
  // Default = 0.2
  vtkSetMacro(A,double);
  vtkGetMacro(A,double);

  // Description:
  // Set/Get the A function coefficient.
  // See the definition in Parametric surfaces referred to above.
  // Default is 1.
  vtkSetMacro(B,double);
  vtkGetMacro(B,double);

  // Description:
  // Set/Get the B function coefficient.
  // See the definition in Parametric surfaces referred to above.
  // Default is 0.1.
  vtkSetMacro(C,double);
  vtkGetMacro(C,double);

  // Description:
  // Set/Get the C function coefficient.
  // See the definition in Parametric surfaces referred to above.
  // Default is 2.
  vtkSetMacro(N,double);
  vtkGetMacro(N,double);

  // Description:
  // A conic spiral surface.
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
  virtual double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]);

protected:
  vtkParametricConicSpiral();
  ~vtkParametricConicSpiral();

  // Variables
  double A;
  double B;
  double C;
  double N;

private:
  vtkParametricConicSpiral(const vtkParametricConicSpiral&);  // Not implemented.
  void operator=(const vtkParametricConicSpiral&);  // Not implemented.
};

#endif
