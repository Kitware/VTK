/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricPseudosphere.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParametricPseudosphere.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkParametricPseudosphere);
//----------------------------------------------------------------------------//
vtkParametricPseudosphere::vtkParametricPseudosphere()
{
  // Preset triangulation parameters
  this->MinimumU = -5.0;
  this->MaximumU = 5.0;
  this->MinimumV = -vtkMath::Pi();
  this->MaximumV = vtkMath::Pi();

  this->JoinU = 0;
  this->JoinV = 1;
  this->TwistU = 0;
  this->TwistV = 0;
  this->ClockwiseOrdering = 0;
  this->DerivativesAvailable = 1;
}

//----------------------------------------------------------------------------//
vtkParametricPseudosphere::~vtkParametricPseudosphere()
{
}

//----------------------------------------------------------------------------//
void vtkParametricPseudosphere::Evaluate(double uvw[3], double Pt[3],
    double Duvw[9])
{
  // Copy the parameters out of the vector, for the sake of convenience.
  double u = uvw[0];
  double v = uvw[1];

  // We're only going to need the u and v partial derivatives.
  // The w partial derivatives are not needed.
  double *Du = Duvw;
  double *Dv = Duvw + 3;

  // Instead of a bunch of calls to the trig library,
  // just call it once and store the results.
  double cosv   = cos(v);
  double sinv   = sin(v);
  double sechu  = 1. / cosh(u);
  double tanhu  = tanh(u);

  // Location of the point. This parametrization was taken from:
  // http://mathworld.wolfram.com/Pseudosphere.html
  Pt[0] = sechu * cosv;
  Pt[1] = sechu * sinv;
  Pt[2] = u - tanhu;

  // The derivative with respect to u:
  Du[0] = -sechu * tanhu * cosv;
  Du[1] = -sechu * tanhu * sinv;
  Du[2] = 1. - sechu * sechu;

  // The derivative with respect to v:
  Dv[0] = -sechu * sinv;
  Dv[1] = sechu * cosv;
  Dv[2] = 0.;
}

//----------------------------------------------------------------------------//
double vtkParametricPseudosphere::EvaluateScalar(double *, double *,
    double *)
{
  return 0;
}

//----------------------------------------------------------------------------//
void vtkParametricPseudosphere::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
