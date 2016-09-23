/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricCatalanMinimal.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParametricCatalanMinimal.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkParametricCatalanMinimal);
//----------------------------------------------------------------------------//
vtkParametricCatalanMinimal::vtkParametricCatalanMinimal()
{
  // Preset triangulation parameters
  this->MinimumU = -4.*vtkMath::Pi();
  this->MinimumV = -1.5;
  this->MaximumU =  4.*vtkMath::Pi();
  this->MaximumV = 1.5;

  this->JoinU = 0;
  this->JoinV = 0;
  this->TwistU = 0;
  this->TwistV = 0;
  this->ClockwiseOrdering = 1;
  this->DerivativesAvailable = 1;
}

//----------------------------------------------------------------------------//
vtkParametricCatalanMinimal::~vtkParametricCatalanMinimal()
{
}

//----------------------------------------------------------------------------//
void vtkParametricCatalanMinimal::Evaluate(double uvw[3], double Pt[3], double Duvw[9])
{
  // Copy the parameters out of the vector, for the sake of convenience.
  double u = uvw[0];
  double v = uvw[1];

  // We're only going to need the u and v partial derivatives.
  // The w partial derivatives are not needed.
  double *Du = Duvw;
  double *Dv = Duvw + 3;

  // Location of the point. This parametrization was taken from:
  // https://www.math.hmc.edu/~gu/curves_and_surfaces/surfaces/catalan.html
  Pt[0] = u  - cosh(v)*sin(u);
  Pt[1] = 1. - cos(u)*cosh(v);
  Pt[2] = 4.*sin(u/2.)*sinh(v/2.);

  // The derivative with respect to u:
  Du[0] = 1. - cosh(v)*cos(u);
  Du[1] = cosh(v)*sin(u);
  Du[2] = 2.*cos(u/2.)*sinh(v/2.);

  // The derivative with respect to v:
  Dv[0] = sin(u)*sinh(v);
  Dv[1] = -cos(u)*sinh(v);
  Dv[2] = 2.*sin(u/2.)*cosh(v/2.);
}

//----------------------------------------------------------------------------//
double vtkParametricCatalanMinimal::EvaluateScalar(double *, double *, double *)
{
  return 0;
}

//----------------------------------------------------------------------------//
void vtkParametricCatalanMinimal::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
