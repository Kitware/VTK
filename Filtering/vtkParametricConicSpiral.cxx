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

vtkCxxRevisionMacro(vtkParametricConicSpiral, "1.1");
vtkStandardNewMacro(vtkParametricConicSpiral);

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

  // Nautilus
  //this->A = 0.2;
  //this->B = 0.1;
  //this->C = 0;
  //this->N = 2;
  // Spiral
  this->A = 0.2;
  this->B = 1;
  this->C = 0.1;
  this->N = 2;
}

vtkParametricConicSpiral::~vtkParametricConicSpiral()
{
}

void vtkParametricConicSpiral::Evaluate(double U[3], double Pt[3], double DU[9])
{
  double u = U[0];
  double v = U[1];
  double *Du = DU;
  double *Dv = DU + 3;

  // A parametric representation of a conic spiral surface
  // Define:
  // -  X(u,v) = a*(1-v/(2*pi))*cos(n*v)*(1+cos(u))+c*cos(n*v)
  // -  Y(u,v) = a*(1-v/(2*pi))*sin(n*v)*(1+cos(u))+c*sin(n*v)
  // -  Z(u,v) = b*v/(2*pi)+a*(1-v/(2*pi))*sin(u)
  //
  // Where:  a=0.2,b=1,c=0.1,n=2,u=0..2*pi},v=0..2*pi
  //
  // Then
  // - S(u,v) = (X(u,v),Y(u,v),Z(u,v)) defines the surface. 
  //
  // The derivatives are given by:
  // - d(X(u,v)/du = -a*(1-1/2*v/pi)*cos(n*v)*sin(u)
  // - d(X(u,v)/dv = -1/2*a/pi*cos(n*v)*(1+cos(u))-a*(1-1/2*v/pi)*sin(n*v)*n*(1+cos(u))-c*sin(n*v)*n
  // - d(Y(u,v)/du = -a*(1-1/2*v/pi)*sin(n*v)*sin(u)
  // - d(Y(u,v)/dv = -1/2*a/Pi*sin(n*v)*(1+cos(u))+a*(1-1/2*v/pi)*cos(n*v)*n*(1+cos(u))+c*cos(n*v)*n
  // - d(Z(u,v)/du = a*(1-1/2*v/pi)*cos(u)
  // - d(Z(u,v)/dv = 1/2*b/pi-1/2*a/pi*sin(u)
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

double vtkParametricConicSpiral::EvaluateScalar(double *, double *, double *)
{
  return 0;
}

void vtkParametricConicSpiral::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "A: " << this->A << "\n";
  os << indent << "B: " << this->B << "\n";
  os << indent << "C: " << this->C << "\n";
  os << indent << "N: " << this->N << "\n";
}
