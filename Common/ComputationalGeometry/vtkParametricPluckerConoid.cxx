/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricPluckerConoid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParametricPluckerConoid.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkParametricPluckerConoid);
//----------------------------------------------------------------------------//
vtkParametricPluckerConoid::vtkParametricPluckerConoid() :
  N(2)
{
  // Preset triangulation parameters
  this->MinimumU = 0.;
  this->MinimumV = 0.;
  this->MaximumU = 3.;
  this->MaximumV = 2.*vtkMath::Pi();

  this->JoinU = 0;
  this->JoinV = 0;
  this->TwistU = 0;
  this->TwistV = 0;
  this->ClockwiseOrdering = 1;
  this->DerivativesAvailable = 1;
}

//----------------------------------------------------------------------------//
vtkParametricPluckerConoid::~vtkParametricPluckerConoid()
{
}

//----------------------------------------------------------------------------//
void vtkParametricPluckerConoid::Evaluate(double uvw[3], double Pt[3], double Duvw[9])
{
  // Copy the parameters out of the vector, for the sake of convenience.
  double u = uvw[0];
  double v = uvw[1];

  // We're only going to need the u and v partial derivatives.
  // The w partial derivatives are not needed.
  double *Du = Duvw;
  double *Dv = Duvw + 3;

  // Location of the point, this parametrization was take from:
  // https://en.wikipedia.org/wiki/Plücker%27s_conoid
  Pt[0] = u*cos(v);
  Pt[1] = u*sin(v);
  Pt[2] = sin(this->N * v);

  // The derivative with respect to u:
  Du[0] = cos(v);
  Du[1] = sin(v);
  Du[2] = 0.;

  // The derivative with respect to v:
  Dv[0] = -u*sin(v);
  Dv[1] =  u*cos(v);
  Dv[2] = this->N * cos(this->N * v);
}

//----------------------------------------------------------------------------//
double vtkParametricPluckerConoid::EvaluateScalar(double *, double *, double *)
{
  return 0;
}

//----------------------------------------------------------------------------//
void vtkParametricPluckerConoid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
