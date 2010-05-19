/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricConicSpiral.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParametricConicSpiral.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkParametricConicSpiral);

//----------------------------------------------------------------------------
vtkParametricConicSpiral::vtkParametricConicSpiral()
{
  // Preset triangulation parameters
  this->MinimumU = 0;
  this->MinimumV = 0;
  this->MaximumU = 2.0*vtkMath::Pi();
  this->MaximumV = 2.0*vtkMath::Pi();

  this->JoinU = 0;
  this->JoinV = 0;
  this->TwistU = 0;
  this->TwistV = 0;
  this->ClockwiseOrdering = 1;
  this->DerivativesAvailable = 1;

  // Conic Spiral
  this->A = 0.2;
  this->B = 1;
  this->C = 0.1;
  this->N = 2;
}

//----------------------------------------------------------------------------
vtkParametricConicSpiral::~vtkParametricConicSpiral()
{
}

//----------------------------------------------------------------------------
void vtkParametricConicSpiral::Evaluate(double uvw[3], double Pt[3], double Duvw[9])
{
  double u = uvw[0];
  double v = uvw[1];
  double *Du = Duvw;
  double *Dv = Duvw + 3;

  double inv2pi = 1.0/(2.0*vtkMath::Pi());

  double cnv = cos(this->N*v);
  double snv = sin(this->N*v);
  double cu = cos(u);
  double su = sin(u);

  // The point
  Pt[0] = this->A*(1-v*inv2pi)*cnv*(1+cu)+this->C*cnv;
  Pt[1] = this->A*(1-v*inv2pi)*snv*(1+cu)+this->C*snv;
  Pt[2] = this->B*v*inv2pi+this->A*(1-v*inv2pi)*su;

  //The derivatives are:
  Du[0] = -this->A*(1-v*inv2pi)*cnv*su;
  Dv[0] = -this->A*inv2pi*cnv*(1+cu)-this->A*(1-v*inv2pi)*snv*this->N*(1+cu)-this->C*snv*N;
  Du[1] = -this->A*(1-v*inv2pi)*snv*su;
  Dv[1] = -this->A*inv2pi*snv*(1+cu)+this->A*(1-v*inv2pi)*cnv*this->N*(1+cu)+C*cnv*this->N;
  Du[2] = this->A*(1-v*inv2pi)*cu;
  Dv[2] = this->B*inv2pi-this->A*inv2pi*su;
}

//----------------------------------------------------------------------------
double vtkParametricConicSpiral::EvaluateScalar(double *, double *, double *)
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkParametricConicSpiral::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "A: " << this->A << "\n";
  os << indent << "B: " << this->B << "\n";
  os << indent << "C: " << this->C << "\n";
  os << indent << "N: " << this->N << "\n";
}
