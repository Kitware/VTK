/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricBoy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParametricBoy.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkParametricBoy);

//----------------------------------------------------------------------------
vtkParametricBoy::vtkParametricBoy()
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

  this->ZScale = 0.125;
}

//----------------------------------------------------------------------------
vtkParametricBoy::~vtkParametricBoy()
{
}

//----------------------------------------------------------------------------
void vtkParametricBoy::Evaluate(double uvw[3], double Pt[3], double Duvw[9])
{

  double u = uvw[0];
  double v = uvw[1];
  double *Du = Duvw;
  double *Dv = Duvw + 3;

  double cu = cos(u);
  double su = sin(u);
  double sv = sin(v);

  double X = cos(u)*sin(v);
  double Y = sin(u)*sin(v);
  double Z = cos(v);

  double X2 = X * X;
  double X3 = X2 * X;
  double X4 = X3 * X;
  double Y2 = Y * Y;
  double Y3 = Y2 * Y;
  double Y4 = Y3 * Y;
  double Z2 = Z * Z;
  double Z3 = Z2 * Z;
  double Z4 = Z3 * Z;

  // The point
  Pt[0] = 1.0/2.0*(2*X2-Y2-Z2+2.0*Y*Z*(Y2-Z2)+
    Z*X*(X2-Z2)+X*Y*(Y2-X2));
  Pt[1] = sqrt(3.0)/2.0*(Y2-Z2+(Z*X*(Z2-X2)+
    X*Y*(Y2-X2)));
  Pt[2] = this->ZScale*(X+Y+Z)*((X+Y+Z)*(X+Y+Z)*(X+Y+Z)+
    4.0*(Y-X)*(Z-Y)*(X-Z));

  //The derivatives are:
  Du[0] = -1.0/2.0*X4-Z3*X+3.0*Y2*X2-3.0/2.0*Z*X2*Y+3.0*Z*X*Y2-
    3.0*Y*X-1.0/2.0*Y4+1.0/2.0*Z3*Y;
  Dv[0] = (3.0/2.0*Z2*X2+2*Z*X-1.0/2.0*Z4)*cu+
    (-2.0*Z*X3+2*Z*X*Y2+3*Z2*Y2-Z*Y-Z4)*su+
    (-1.0/2.0*X3+3.0/2.0*Z2*X-Y3+3.0*Z2*Y+Z)*sv;
  Du[1] = -1.0/2.0*sqrt(3.0)*X4+3.0*sqrt(3.0)*Y2*X2+
    3.0/2.0*sqrt(3.0)*Z*X2*Y+sqrt(3.0)*Y*X-
    1.0/2.0*sqrt(3.0)*Y4-1.0/2.0*sqrt(3.0)*Z3*Y;
  Dv[1] = (-3.0/2.0*sqrt(3.0)*Z2*X2+1.0/2.0*sqrt(3.0)*Z4)*cu+
    (-2.0*sqrt(3.0)*Z*X3+2.0*sqrt(3.0)*Z*Y2*X+sqrt(3.0)*Z*Y)*su+
    (1.0/2.0*sqrt(3.0)*X3-3.0/2.0*sqrt(3.0)*Z2*X+sqrt(3.0)*Z)*sv;
  Du[2] = X4+3/2*Z*X3+3/2*Z2*X2+X3*Y-3*X2*Y2+
        3*Z*X2*Y-Y3*X-3/2*Z*Y3-3/2*Z2*Y2-Z3*Y;
  Dv[2] = (1/2*Z*X3+3/2*Z3*X+Z4)*cu+(4*Z*X3+3*Z*X2*Y+
        9/2*Z2*X2+9/2*Z2*X*Y+3*Z3*X+1/2*Z*Y3+3*Z2*Y2+
        3/2*Z3*Y)*su+(-3/2*X2*Y-3/2*Z*X2-3/2*X*Y2-
        3*Z*X*Y-3*Z2*X-Y3-3/2*Z*Y2-1/2*Z3)*sv;
}

//----------------------------------------------------------------------------
double vtkParametricBoy::EvaluateScalar(double *, double *, double *)
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkParametricBoy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ZScale: " << this->ZScale << "\n";

}
