/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricTorus.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParametricTorus.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkParametricTorus);

//----------------------------------------------------------------------------
vtkParametricTorus::vtkParametricTorus() :
  RingRadius(1.0), CrossSectionRadius(0.5)
{
  this->MinimumU = 0;
  this->MinimumV = 0;
  this->MaximumU = 2 * vtkMath::Pi();
  this->MaximumV = 2 * vtkMath::Pi();

  this->JoinU = 1;
  this->JoinV = 1;
  this->TwistU = 0;
  this->TwistV = 0;
  this->ClockwiseOrdering = 1;
  this->DerivativesAvailable = 1;
}

//----------------------------------------------------------------------------
vtkParametricTorus::~vtkParametricTorus()
{
}

//----------------------------------------------------------------------------
void vtkParametricTorus::Evaluate(double uvw[3], double Pt[3], double Duvw[9])
{
  double u = uvw[0];
  double v = uvw[1];
  double *Du = Duvw;
  double *Dv = Duvw + 3;

  double cu = cos(u);
  double su = sin(u);
  double cv = cos(v);
  double sv = sin(v);
  double t = this->RingRadius + this->CrossSectionRadius * cv;

  // The point
  Pt[0] = t * cu;
  Pt[1] = t * su;
  Pt[2] = this->CrossSectionRadius * sv;

  //The derivatives are:
  Du[0] = -t * su;
  Du[1] = t * cu;
  Du[2] = 0;
  Dv[0] = -this->CrossSectionRadius * sv * cu;
  Dv[1] = -this->CrossSectionRadius * sv * su;
  Dv[2] = this->CrossSectionRadius * cv;
}

//----------------------------------------------------------------------------
double vtkParametricTorus::EvaluateScalar(double* vtkNotUsed(uv[3]),
                                          double* vtkNotUsed(Pt[3]),
                                          double* vtkNotUsed(Duv[9]))
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkParametricTorus::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Ring Radius: " << this->RingRadius << "\n";
  os << indent << "Cross-Sectional Radius: " << this->CrossSectionRadius << "\n";
}
