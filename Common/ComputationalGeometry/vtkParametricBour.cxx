/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricBour.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParametricBour.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkParametricBour);
//----------------------------------------------------------------------------//
vtkParametricBour::vtkParametricBour()
{
  // Preset triangulation parameters
  this->MinimumU = 0.;
  this->MaximumU = 1.0;
  this->MinimumV = 0.;
  this->MaximumV = 4.*vtkMath::Pi();

  this->JoinU = 0;
  this->JoinV = 0;
  this->TwistU = 0;
  this->TwistV = 0;
  this->ClockwiseOrdering = 0;
  this->DerivativesAvailable = 1;
}

//----------------------------------------------------------------------------//
vtkParametricBour::~vtkParametricBour()
{
}

//----------------------------------------------------------------------------//
void vtkParametricBour::Evaluate(double uvw[3], double Pt[3],
                                 double Duvw[9])
{
  // Copy the parameters out of the vector, for the sake of convenience.
  double u = uvw[0];
  double v = uvw[1];

  // We're only going to need the u and v partial derivatives.
  // The w partial derivatives are not needed.
  double *Du = Duvw;
  double *Dv = Duvw + 3;

  // Location of the point. This parametrization was taken from:
  // https://en.wikipedia.org/wiki/Bour%27s_minimal_surface
  Pt[0] = u * cos(v) - u * u * cos(2.*v) / 2.;
  Pt[1] = -u * sin(v) * (u * cos(v) + 1.);
  Pt[2] = 4. / 3.*pow(u, 1.5) * cos(1.5 * v);

  // The derivative with respect to u:
  Du[0] = cos(v) - u * cos(2 * v);
  Du[1] = -sin(v) * (1. + 2.*u * cos(v));
  Du[2] = 2 * sqrt(u) * cos(1.5 * v);

  // The derivative with respect to v:
  Dv[0] = u * (2.*u * cos(v) - 1.) * sin(v);
  Dv[1] = -u * (cos(v) + u * cos(2.*v));
  Dv[2] = -2.*pow(u, 1.5) * sin(1.5 * v);
}

//----------------------------------------------------------------------------//
double vtkParametricBour::EvaluateScalar(double *, double *, double *)
{
  return 0;
}

//----------------------------------------------------------------------------//
void vtkParametricBour::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
