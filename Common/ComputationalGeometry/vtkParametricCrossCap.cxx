/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricCrossCap.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParametricCrossCap.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkParametricCrossCap);

//----------------------------------------------------------------------------
vtkParametricCrossCap::vtkParametricCrossCap()
{
  // Preset triangulation parameters
  this->MinimumU = 0;
  this->MinimumV = 0;
  this->MaximumU = vtkMath::Pi();
  this->MaximumV = vtkMath::Pi();

  this->JoinU = 1;
  this->JoinV = 1;
  this->TwistU = 1;
  this->TwistV = 1;
  this->ClockwiseOrdering = 1;
  this->DerivativesAvailable = 1;
}

//----------------------------------------------------------------------------
vtkParametricCrossCap::~vtkParametricCrossCap()
{
}

//----------------------------------------------------------------------------
void vtkParametricCrossCap::Evaluate(double uvw[3], double Pt[3], double Duvw[9])
{

  double u = uvw[0];
  double v = uvw[1];
  double *Du = Duvw;
  double *Dv = Duvw + 3;

  double cu = cos(u);
  double su = sin(u);
  double cv = cos(v);
  double c2v = cos(2 * v);
  double sv = sin(v);
  double s2v = sin(2 * v);

  // The point
  Pt[0] = cu * s2v;
  Pt[1] = su * s2v;
  Pt[2] = cv * cv - cu * cu * sv * sv;

  //The derivatives are:
  Du[0] = -Pt[1];
  Du[1] = Pt[0];
  Du[2] = 2 * cu * su * sv * sv;
  Dv[0] = 2 * cu * c2v;
  Dv[1] = 2 * su * c2v;
  Dv[2] = -2 * cv * sv * (1 + cu * cu);
}

//----------------------------------------------------------------------------
double vtkParametricCrossCap::EvaluateScalar(double *, double *, double *)
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkParametricCrossCap::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
